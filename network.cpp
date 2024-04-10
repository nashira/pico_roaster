#include "network.h"
#include "temperature.h"
#include "sensors.h"
#include "Http.h"
#include <WiFiEspAT.h>
#include <ArduinoJson.h>

static HttpServer server;
static JsonDocument json;
static unsigned long long capture_time;

void network_on_wifi_connect(void *data);
void network_check_status(void *data);

void handle_request(Request &req);
void websocket_handle_message(uint8_t msg_type, Request &req);

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
  task_schedule(NETWORK_CORE, 0, &network_check_status_task);
}

void network_check_status(void *data) {
  int wifiStatus = WiFi.status();
  Serial.print("network_check_status: ");
  Serial.println(wifiStatus);
  if (wifiStatus == WL_CONNECTED) {
    task_cancel(NETWORK_CORE, network_check_status_task.alarm_id);
    Serial.println(WiFi.localIP());
    Serial1.println("AT+UART_CUR=1000000,8,1,0,0");
    task_schedule(NETWORK_CORE, 100'000, &network_on_wifi_connect_task);
  }
}

void network_on_wifi_connect(void *data) {
  // Serial.println("network_on_wifi_connect");
  Serial1.begin(1'000'000);
  WiFiServer wifiServer(80);
  wifiServer.begin(5, 60);
  server.begin(wifiServer, handle_request, websocket_handle_message);
}

void network_serve_clients() {
  server.poll();
}

void handle_request(Request &req) {
  if (strncmp(req.path, "/ws", 3) == 0) {
    req.websocketHandshake();

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

void websocket_handle_message(uint8_t msg_type, Request &req) {
  if (msg_type == 0x81) {
    deserializeJson(json, req.buf, req.buflen);
    long id = json["id"];
    String c = json["c"];
    json.clear();

    // Serial.printf("%d : ", id);
    // Serial.println(c);

    if (c == "d") {
      json["id"] = id;
      json["d"]["bt"] = temperature1();
      json["d"]["et"] = temperature2();
      json["d"]["fan"] = sensors_voltage();
      json["d"]["gas"] = sensors_angle();
      json["d"]["ts"] = millis() - capture_time;
      req.buflen = serializeJson(json, req.buf);
      req.websocketSend(0x81, reinterpret_cast<uint8_t *>(req.buf), req.buflen);
    } else if (c == "reset_capture_time") {
      capture_time = millis();
    }
  }
}

