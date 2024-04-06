#include "Http.h"

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
