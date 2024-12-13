#include "http.h"

#include <string.h>


enum HTTP_METHOD http_match_method(char * buffer, int buffer_size) {
    if (strncmp(buffer, "GET", 3) == 0) {
        return HTTP_METHOD_GET;
    }
    if (strncmp(buffer, "HEAD", 4) == 0) {
        return HTTP_METHOD_HEAD;
    }
    if (strncmp(buffer, "POST", 4) == 0) {
        return HTTP_METHOD_POST;
    }
    if (strncmp(buffer, "PUT", 3) == 0) {
        return HTTP_METHOD_PUT;
    }
    if (strncmp(buffer, "DELETE", 6) == 0) {
        return HTTP_METHOD_DELETE;
    }
    if (strncmp(buffer, "CONNECT", 7) == 0) {
        return HTTP_METHOD_CONNECT;
    }
    if (strncmp(buffer, "OPTIONS", 7) == 0) {
        return HTTP_METHOD_OPTIONS;
    }
    if (strncmp(buffer, "TRACE", 5) == 0) {
        return HTTP_METHOD_TRACE;
    }
    if (strncmp(buffer, "PATCH", 5) == 0) {
        return HTTP_METHOD_PATCH;
    }

    return HTTP_METHOD_UNKNOWN;
}

int http_method_string_length(enum HTTP_METHOD method) {
    switch (method) {
        case HTTP_METHOD_GET:
            return 3;
        case HTTP_METHOD_HEAD:
            return 4;
        case HTTP_METHOD_POST:
            return 4;
        case HTTP_METHOD_PUT:
            return 3;
        case HTTP_METHOD_DELETE:
            return 6;
        case HTTP_METHOD_CONNECT:
            return 7;
        case HTTP_METHOD_TRACE:
            return 5;
        case HTTP_METHOD_PATCH:
            return 5;
        case HTTP_METHOD_OPTIONS:
            return 7;

        default:
            return 0;
    }
}

const char * http_method_to_string(enum HTTP_METHOD method) {
    switch (method) {
        case HTTP_METHOD_GET:
            return "GET";
        case HTTP_METHOD_HEAD:
            return "HEAD";
        case HTTP_METHOD_POST:
            return "POST";
        case HTTP_METHOD_PUT:
            return "PUT";
        case HTTP_METHOD_DELETE:
            return "DELETE";
        case HTTP_METHOD_CONNECT:
            return "CONNECT";
        case HTTP_METHOD_TRACE:
            return "TRACE";
        case HTTP_METHOD_PATCH:
            return "PATCH";
        case HTTP_METHOD_OPTIONS:
            return "OPTIONS";

        default:
            return 0;
    }
}
