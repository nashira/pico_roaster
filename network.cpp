#include "network.h"
#include "temperature.h"
#include "sensors.h"
#include "Http.h"
#include "src/sha1.h"
#include "src/Base64.h"
#include <WiFiEspAT.h>
#include <ArduinoJson.h>

static WiFiServer server(80);
static Request request[5];
static JsonDocument json;

void network_on_wifi_connect(void *data);
void network_check_status(void *data);
void handle_request(Request &req);
void websocket_handshake(Request &req);
void websocket_handle_message(Request &req);

static task_t network_on_wifi_connect_task = {
  .execute = network_on_wifi_connect
};

static task_t network_check_status_task = {
  .execute = network_check_status,
  .period_us = -ftous(2)
};

void network_init() {
  // Serial.println("network_init");
  Serial1.setTX(16);
  Serial1.setRX(17);
  Serial1.begin(115200);
  WiFi.init(Serial1, 22);
  WiFi.setHostname("kaldi");
  core0_schedule(0, &network_check_status_task);
}

void network_check_status(void *data) {
  int wifiStatus = WiFi.status();
  Serial.print("network_check_status: ");
  Serial.println(wifiStatus);
  if (wifiStatus == WL_CONNECTED) {
    core0_cancel(network_check_status_task.alarm_id);
    Serial.println(WiFi.localIP());
    Serial1.println("AT+UART_CUR=1000000,8,1,0,0");
    core0_schedule(100'000, &network_on_wifi_connect_task);
  }
}

void network_on_wifi_connect(void *data) {
  // Serial.println("network_on_wifi_connect");
  Serial1.begin(1'000'000);
  server.begin(5, 60);
}

void network_serve_clients() {
  if (server.status() != LISTEN) return;

  WiFiClient client = server.accept();

  if (client) {
    for (int i = 0; i < 5; i++) {
      if (!(request[i].client)) {
        Serial.print("connect client #");
        Serial.println(i);
        request[i].client = client;
        break;
      }
    }
  }

  for (int i = 0; i < 5; i++) {
    Request &r = request[i];
    if (r.client.available() > 0) {
      if (r.websocket) {
        websocket_handle_message(r);
      } else {
        int res = r.parse();
        if (res > 0) {
          handle_request(r);
          r.reset();
        } else if (res == -1) {
          Serial.println("error");
          Serial.println((int) WiFi.getLastDriverError());
          r.reset();
        }
      }
    }
  }

  for (int i = 0; i < 5; i++) {
    if (request[i].client && !(request[i].client.connected())) {
      Serial.print("disconnect client #");
      Serial.println(i);
      request[i].reset();
      request[i].websocket = false;
      request[i].client.stop();
    }
  }
}

void handle_request(Request &req) {
  if (strncmp(req.path, "/ws", 3) == 0) {
    
    websocket_handshake(req);

  } else if (req.path_len == 1 && strncmp(req.path, "/", 1) == 0) {
    
    size_t size = snprintf(req.buf, sizeof(req.buf) - 1, "t: %llu, t1: %f, t2: %f, t1dx: %f, t2dx: %f, v: %f, a: %f\n",
      time_us_64(),
      temperature1(),
      temperature2(),
      temperature1_dx(),
      temperature2_dx(),
      sensors_voltage(),
      sensors_angle()
    );
    req.send(200, "text/plain", reinterpret_cast<uint8_t *>(req.buf), size);

  } else {
    // 404
    size_t size = snprintf(req.buf, sizeof(req.buf) - 1, "not found\n");
    req.send(404, "text/plain", reinterpret_cast<uint8_t *>(req.buf), size);

  }
}

// websockets

void websocket_handshake(Request &req) {
  bool found = false;
  char key[32];
  for (int i = 0; i < req.num_headers; i++) {
    if (strncmp(req.headers[i].name, "Sec-WebSocket-Key", req.headers[i].name_len) == 0) {
      found = true;
      strncpy(key, req.headers[i].value, 32);
      if (req.headers[i].value_len < 32) {
        key[req.headers[i].value_len] = 0;
      }
      break;
    }
  }
  
  if (found) {
    Sha1.init();
    Sha1.print(key);
    Sha1.print("258EAFA5-E914-47DA-95CA-C5AB0DC85B11");
    char *hash = reinterpret_cast<char *>(Sha1.result());
    char b64[base64_enc_len(20)];
    base64_encode(b64, hash, 20);
    req.client.print("HTTP/1.1 101 Web Socket Protocol Handshake\r\n");
    req.client.print("Upgrade: websocket\r\n");
    req.client.print("Connection: Upgrade\r\n");
    req.client.print("Sec-WebSocket-Accept: ");
    req.client.print(b64);
    req.client.print("\r\n");
    req.client.print("\r\n");
    req.client.flush();
    req.websocket = true;
  } else {
    size_t size = snprintf(req.buf, sizeof(req.buf) - 1, "hello\n");
    req.send(200, "text/plain", reinterpret_cast<uint8_t *>(req.buf), size);
  }
}

void websocket_disconnect(Request &req) {
  Serial.println("disconnect ws client");
  req.client.write((uint8_t) 0x88);
  req.client.write((uint8_t) 0x02);
  req.client.write((uint8_t) 0x03);
  req.client.write((uint8_t) 0xE8);
  req.client.flush();
  req.websocket = false;
  req.reset();
}

void websocket_decode_message(Request &req) {
  uint8_t msgtype;
  unsigned int length;
  uint8_t mask[4];
  unsigned int i;

  msgtype = req.client.read();

  if (msgtype == 0x88) {
    websocket_disconnect(req);
    return;
  }

  length = req.client.read() & 0x7F;

  if (length == 0x7E) {
      length = (req.client.read() << 8) | req.client.read();
  } else if (length == 0x7F) {
    // TODO large msg
  }

  length = min(length, 1024);

  req.client.readBytes(mask, 4);
  req.client.readBytes(req.buf, length);

  for (i = 0; i < length; ++i) {
    req.buf[i] ^= mask[i % 4];
  }
  req.buflen = length;
}

void websocket_send_message(Request &req) {
  int size = req.buflen;
  // string msg type
  req.client.write(0x81);

  // TODO: large msg
  if (size > 125) {
      req.client.write(0x7E);
      req.client.write((uint8_t) (size >> 8));
      req.client.write((uint8_t) (size && 0xFF));
  } else {
      req.client.write((uint8_t) size);
  }

  req.client.write(req.buf, req.buflen);
}

void websocket_handle_message(Request &req) {
  websocket_decode_message(req);

  deserializeJson(json, req.buf, req.buflen);
  long id = json["id"];
  json.clear();

  json["id"] = id;
  json["d"]["bt"] = temperature1();
  json["d"]["et"] = temperature2();
  json["d"]["fan"] = sensors_voltage();
  json["d"]["gas"] = sensors_angle();
  json["d"]["ts"] = millis();
  req.buflen = serializeJson(json, req.buf);
  websocket_send_message(req);
}

