
#ifndef _HTTPSERVER_H_
#define _HTTPSERVER_H_

#include "picohttpparser.h"
#include <WiFiEspAT.h>


#define HTTP_METHOD_GET               "GET"
#define HTTP_METHOD_POST              "POST"
#define HTTP_METHOD_PUT               "PUT"
#define HTTP_METHOD_PATCH             "PATCH"
#define HTTP_METHOD_DELETE            "DELETE"
#define HTTP_HEADER_CONTENT_LENGTH    "Content-Length"
#define HTTP_HEADER_CONTENT_TYPE      "Content-Type"
#define HTTP_HEADER_CONNECTION        "Connection"
#define HTTP_HEADER_TRANSFER_ENCODING "Transfer-Encoding"
#define HTTP_HEADER_USER_AGENT        "User-Agent"
#define HTTP_HEADER_VALUE_CHUNKED     "chunked"

typedef struct phr_header Header;
typedef struct phr_header Param;

class Request {

public:
  // Request();

  char buf[1024];
  char *method;
  char *path;
  int minor_version;
  Header headers[20];
  Param params[20];
  size_t buflen = 0;
  size_t prevbuflen = 0;
  size_t method_len;
  size_t path_len;
  size_t num_headers;
  size_t num_params;
  WiFiClient client;
  bool websocket = false;

  void reset();
  int parse();
  void send(int status, const char *contentType, uint8_t *body, size_t size);
  uint8_t readBlocking();
  void websocketHandshake();
  void websocketSend(uint8_t msg_type, uint8_t *body, size_t size);
  uint8_t websocketParse();
  void websocketDisconnect();

private:

  void parseParams();
};

typedef void(*HttpRequestHandler)(Request &request);
typedef void(*WebSocketMessageHandler)(uint8_t msgType, Request &request);

class HttpServer {

public:
  void begin(WiFiServer &server, HttpRequestHandler http, WebSocketMessageHandler websocket);
  void poll();

private:
  WiFiServer server;
  Request request[5];
  HttpRequestHandler httpRequestHandler;
  WebSocketMessageHandler websocketMessageHandler;
};

#endif
