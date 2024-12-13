#include "webserver.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include <unistd.h>
#include <sys/stat.h>

#include "stb_ds.h"

#include "http.h"
#include "error.h"


static int file_exists(char * filename) {
    struct stat path_stat;

    if (stat(filename, &path_stat) == -1) {
        return 0;
    }

    if (!S_ISREG(path_stat.st_mode)) {
        return 0;
    }

    FILE * f = fopen(filename, "r");

    if (f == NULL) {
        return 0;
    }

    fclose(f);

    return 1;
}

static const char * file_mimetype(char * filepath) {
    char * extension = strrchr(filepath, '.');

    if (extension == NULL) {
        return "application/octet-stream";
    }

    extension++;

    if (strcmp(extension, "html") == 0 || strcmp(extension, "htm") == 0) {
        return "text/html";
    }
    if (strcmp(extension, "css") == 0) {
        return "text/css";
    }
    if (strcmp(extension, "js") == 0) {
        return "application/javascript";
    }
    if (strcmp(extension, "json") == 0) {
        return "application/json";
    }
    if (strcmp(extension, "jpeg") == 0 || strcmp(extension, "jpg") == 0) {
        return "image/jpg";
    }
    if (strcmp(extension, "txt") == 0) {
        return "text/plain";
    }
    if (strcmp(extension, "gif") == 0) {
        return "image/gif";
    }
    if (strcmp(extension, "png") == 0) {
        return "image/png";
    }
    if (strcmp(extension, "pdf") == 0) {
        return "application/pdf";
    }

    return "application/octet-stream";
}

struct request_t {
    enum HTTP_METHOD method;
    char * path;
    char * content;
    struct headers_t { char * key; char * value; } * headers;
};

struct response_t {
    char * content;
    int content_length;
    const char * code_message;
    const char * mimetype;
    int code;
};

static struct response_t get(struct request_t request);
static struct response_t post(struct request_t request);
static struct response_t any(struct request_t request);

struct request_t read_request(int sockfd) {
    struct request_t request = {
        .method = HTTP_METHOD_UNKNOWN,
        .path = NULL,
        .content = NULL,
        .headers = NULL
    };

    shdefault(request.headers, "0");

    const int BUFFER_SIZE = 1024 * 1024;
    char BUFFER[BUFFER_SIZE];

    memset(BUFFER, 0, BUFFER_SIZE);

    int index = 0;

    int index_starting_header = 0;

    int state = 0;

    while (1) {
        int n = read(sockfd, &BUFFER[index], 1);

        if (n == 0) {
            break;
        }

        if (n < 0) {
            error("ERROR reading from socket");
        }

        if (index >= 2) {
            if (state == 0) {
                /* Reading Request Line */
                if (strcmp(&BUFFER[index - 1], "\r\n") == 0) {
                    state = 1;

                    BUFFER[index + 1] = '\0';

                    char * start_path = strchr(BUFFER, ' ');
                    char * end_path = strchr(start_path + 1, ' ');

                    request.method = http_match_method(BUFFER, index);
                    request.path = (char *) malloc(sizeof(char) * (end_path - start_path));

                    memcpy(request.path, start_path + 1, (end_path - start_path) - 1);

                    request.path[(end_path - start_path) - 1] = '\0';

                    index_starting_header = index + 1;
                }
            } else if (state == 1) {
                /* Reading Headers */
                if (strcmp(&BUFFER[index - 3], "\r\n\r\n") == 0) {
                    break;
                }

                if (strcmp(&BUFFER[index - 1], "\r\n") == 0) {
                    BUFFER[index + 1] = '\0';

                    char * end_header_name = strchr(&BUFFER[index_starting_header], ':');
                    char * start_header_value = end_header_name + 2;
                    char * end_header_value = strchr(start_header_value, '\r');

                    int header_name_sz = end_header_name - &BUFFER[index_starting_header];
                    int header_value_sz = end_header_value - start_header_value;

                    char * header_name = (char *) malloc(sizeof(char) * header_name_sz + 1);
                    char * header_value = (char *) malloc(sizeof(char) * header_value_sz + 1);

                    header_name[header_name_sz] = '\0';
                    header_value[header_value_sz] = '\0';

                    memcpy(header_name, &BUFFER[index_starting_header], header_name_sz);
                    memcpy(header_value, start_header_value, header_value_sz);

                    char *s = header_name;
                    while (*s) {
                        *s = toupper((unsigned char) *s);
                        s++;
                    }

                    shput(request.headers, header_name, header_value);

                    index_starting_header = index + 1;
                }
            }
        }

        index = index + 1;
    }

    char * content_length_str = shget(request.headers, "CONTENT-LENGTH");

    int content_length = atoi(content_length_str);

    if (content_length > 0) {
        request.content = (char *) malloc(sizeof(char) * (content_length + 1));
        read(sockfd, request.content, content_length);
        request.content[content_length] = '\0';
    }

    return request;
}

void webserver(int sockfd) {
    struct request_t request = read_request(sockfd);

    char * content = "";
    if (request.content) {
        content = request.content;
    }

    printf("%s %s %s\nHeaders:\n", http_method_to_string(request.method), request.path, content);
    for (int i = 0; i < shlen(request.headers); i++) {
        printf("\t%s: %s\n", request.headers[i].key, request.headers[i].value);
    }

    struct response_t response;

    switch (request.method) {
        case HTTP_METHOD_GET:
            response = get(request);
        break;

        case HTTP_METHOD_PUT:
        case HTTP_METHOD_POST:
            response = post(request);
        break;

        default:
            response = any(request);
        break;
    }

    /* 140: FIXED CONTENT + MAX MIMETYPE SIZE + MAX INT DIGITS + CODE + CODE MESSAGE */
    char * response_header = malloc(sizeof(char) *  140);

    int header_length = snprintf(
        response_header,
        140,
        "HTTP/1.1 %d %s\r\n"
        "Content-Length: %d\r\n"
        "Content-Type: %s\r\n"
        "Connection: Closed\r\n\r\n",
        response.code,
        response.code_message,
        response.content_length,
        response.mimetype
    );

    write(sockfd, response_header, header_length);
    write(sockfd, response.content, response.content_length);

    if (request.content != NULL) {
        free(request.content);
    }

    if (request.path != NULL) {
        free(request.path);
    }

    for (int i = 0; i < shlen(request.headers); i++) {
        free(request.headers[i].key);
        free(request.headers[i].value);
    }
    shfree(request.headers);

    free(response_header);
    free(response.content);

    close(sockfd);
}

struct response_t get(struct request_t request) {
    const int MAX_PATH_LEN = 1024;
    char filepath[MAX_PATH_LEN];

    snprintf(filepath, MAX_PATH_LEN, "%s/%s", "./resources", &request.path[1]);

    int code = 200;
    char * code_message = "OK";

    if (!file_exists(filepath)) {
        code = 404;
        code_message = "NOT FOUND";
        snprintf(filepath, MAX_PATH_LEN, "%s/%s", "./resources", "404.html");
    }

    FILE * f = fopen(filepath, "rb");

    fseek(f, 0, SEEK_END);

    int fsize = ftell(f);

    fseek(f, 0, SEEK_SET);

    char * file_content = malloc(sizeof(char) * fsize);

    fread(file_content, sizeof(char), fsize, f);

    fclose(f);

    const char * mimetype = file_mimetype(filepath);

    return (struct response_t) {
        .code = code,
        .code_message = code_message,
        .content = file_content,
        .content_length = fsize,
        .mimetype = mimetype,
    };
}

struct response_t post(struct request_t request) {
    char * message = "{ 'message': 'OK' }";
    char * mem = (char *) malloc(sizeof(char) * (strlen(message) + 1));
    memcpy(mem, message, strlen(message) + 1);

    return (struct response_t) {
        .code = 200,
        .code_message = "OK",
        .content = mem,
        .content_length = strlen(message),
        .mimetype = file_mimetype(".json"),
    };
}

struct response_t any(struct request_t request) {
    char * message = "{ 'error': 'METHOD NOT IMPLEMENTED YET' }";
    char * mem = (char *) malloc(sizeof(char) * (strlen(message) + 1));
    memcpy(mem, message, strlen(message) + 1);

    return (struct response_t) {
        .code = 500,
        .code_message = "NOT IMPLEMENTED YET",
        .content = mem,
        .content_length = strlen(message),
        .mimetype = file_mimetype(".json"),
    };
}
