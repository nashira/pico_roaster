#include "Http.h"
#include "src/sha1.h"
#include "src/Base64.h"

void Request::reset() {
  buflen = 0;
  prevbuflen = 0;
  method_len = 0;
  path_len = 0;
  num_headers = 0;
  num_params = 0;
}

void Request::parseParams() {
  char *buf = path;
  size_t size = path_len;
  
  const int Before = 0;
  const int Start = 1;
  const int Name = 2;
  const int Equals = 3;
  const int Value = 4;

  int state = Before;

  char *start = buf;
  char *end = buf + size;  
  int i = -1;

  num_params = 0;

  while(start < end) {
    // if (*start == '?' && state != Before) {
    //   start++;
    //   continue;
    // }

    switch(state) {
      case Before: {
        if (*start == '?') {
            state = Start;
        }
        break;
      }
      case Start: {
        if ((*start >= '0' && *start <= '9') || (*start >= 'A' && *start <= 'z')) {
          state = Name;
          i++;
          params[i].name = start;
          params[i].name_len = 0;
          params[i].value = NULL;
          params[i].value_len = 0;
        } else if (*start == '=') {
          state = Equals;
          i++;
          params[i].name = start;
          params[i].name_len = 0;
          params[i].value = NULL;
          params[i].value_len = 0;
        }
        break;
      }
      case Name: {
        if (*start == '=') {
          state = Equals;
          params[i].name_len = start - params[i].name;
        } else if (*start == '&') {
          state = Start;
          params[i].name_len = start - params[i].name;
        }
        break;
      }
      case Equals: {
        if ((*start >= '0' && *start <= '9') || (*start >= 'A' && *start <= 'z')) {
          state = Value;
          params[i].value = start;
        } else if (*start == '&') {
          state = Start;
        }
      }
      case Value: {
        if (*start == '&') {
          state = Start;
          params[i].value_len = start - params[i].value;
        }
        break;
      }
    }

    ++start;
  }
  
  switch(state) {
    case Name: {
      params[i].name_len = start - params[i].name;
      break;
    }
    case Value: {
      params[i].value_len = start - params[i].value;
      break;
    }
  }

  num_params = i+1;
}

int Request::parse() {
  size_t rret = client.read(reinterpret_cast<uint8_t *>(buf + buflen), sizeof(buf) - buflen);
  prevbuflen = buflen;
  buflen += rret;
  num_headers = sizeof(headers) / sizeof(headers[0]);
  ssize_t pret = phr_parse_request(buf, buflen, (const char **)&(method), &(method_len), (const char **)&(path), &(path_len),
              &(minor_version), headers, &(num_headers), prevbuflen);
  if (pret > 0 && path != NULL) {
    parseParams();
  }
  return pret;
}

void Request::send(int status, const char *contentType, uint8_t *body, size_t size) {
  client.print("HTTP/1.1 ");
  client.println(status);
  
  client.print("Content-Type: ");
  client.println(contentType);

  client.print("Content-Length: ");
  client.println(size);

  client.println();

  client.write(body, size);
  client.flush();
}

uint8_t Request::readBlocking() {
  char b[1];
  if (client.readBytes(b, 1) != 1) {
    return -1;
  }
  return b[0];
}

void Request::websocketHandshake() {
  bool found = false;
  char key[32];
  for (int i = 0; i < num_headers; i++) {
    if (strncmp(headers[i].name, "Sec-WebSocket-Key", headers[i].name_len) == 0) {
      found = true;
      strncpy(key, headers[i].value, 32);
      if (headers[i].value_len < 32) {
        key[headers[i].value_len] = 0;
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
    client.print("HTTP/1.1 101 Web Socket Protocol Handshake\r\n");
    client.print("Upgrade: websocket\r\n");
    client.print("Connection: Upgrade\r\n");
    client.print("Sec-WebSocket-Accept: ");
    client.print(b64);
    client.print("\r\n");
    client.print("\r\n");
    client.flush();
    websocket = true;
  } else {
    size_t size = snprintf(buf, sizeof(buf) - 1, "hello\n");
    send(200, "text/plain", reinterpret_cast<uint8_t *>(buf), size);
  }
}

void Request::websocketDisconnect() {
  Serial.println("disconnect ws client");
  websocketSend(0x88, NULL, 0);
  websocket = false;
  reset();
}

uint8_t Request::websocketParse() {
  uint8_t msgtype;
  unsigned int length;
  uint8_t mask[4];
  unsigned int i;

  msgtype = readBlocking();

  uint8_t byte2 = readBlocking();
  uint8_t mask_bit = byte2 & 0x80;

  length = byte2 & 0x7F;

  if (length == 0x7E) {
      length = (readBlocking() << 8) | readBlocking();
  } else if (length == 0x7F) {
    // TODO large msg
  }

  length = min(length, 1024);

  if (mask_bit) {
    client.readBytes(mask, 4);
  }

  client.readBytes(buf, length);

  if (mask_bit) {
    for (i = 0; i < length; ++i) {
      buf[i] ^= mask[i % 4];
    }
  }
  buflen = length;
  return msgtype;
}

void Request::websocketSend(uint8_t msg_type, uint8_t *body, size_t size) {
  
  client.write(msg_type);

  // TODO: large msg
  if (size > 125) {
      client.write(0x7E);
      client.write((uint8_t) (size >> 8));
      client.write((uint8_t) (size && 0xFF));
  } else {
      client.write((uint8_t) size);
  }

  client.write(body, size);
  client.flush();
}

void HttpServer::begin(WiFiServer &server, HttpRequestHandler http, WebSocketMessageHandler websocket) {
  this->server = server;
  this->httpRequestHandler = http;
  this->websocketMessageHandler = websocket;
}

void HttpServer::poll() {
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
        uint8_t msgType = r.websocketParse();
        switch(msgType) {
          // disconnect
          case 0x88:
            r.websocketDisconnect();
            break;
          // ping
          case 0x89:
            r.websocketSend(0x8A, reinterpret_cast<uint8_t *>(r.buf), r.buflen);
            break;
          // pong
          case 0x8A:
            break;
          // text
          case 0x81:
          // binary
          case 0x82:
            websocketMessageHandler(msgType, r);
            break;
        }
      } else {
        int res = r.parse();
        if (res > 0) {
          httpRequestHandler(r);
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
