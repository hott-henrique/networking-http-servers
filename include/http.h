#ifndef HTTP_H

#define HTTP_H

enum HTTP_METHOD {
    HTTP_METHOD_GET,
    HTTP_METHOD_HEAD,
    HTTP_METHOD_POST,
    HTTP_METHOD_PUT,
    HTTP_METHOD_DELETE,
    HTTP_METHOD_CONNECT,
    HTTP_METHOD_TRACE,
    HTTP_METHOD_PATCH,
    HTTP_METHOD_OPTIONS,
    HTTP_METHOD_UNKNOWN
};

enum HTTP_METHOD http_match_method(char * buffer, int buffer_size);
int http_method_string_length(enum HTTP_METHOD method);
const char * http_method_to_string(enum HTTP_METHOD method);

#endif
