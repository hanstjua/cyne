#ifndef CYNE_MESSAGE_H
#define CYNE_MESSAGE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "configs.h"
#include "error.h"
#include "json.h"

#define M2S_CASE(method) case (method): sprintf(out_str, "%s", #method); break;

typedef enum HttpMethod {
    UNKNOWN,
    GET,
    POST,
    PUT,
    DELETE,
    PATCH,
    HEAD,
    CONNECT,
    OPTIONS,
    TRACE
} HttpMethod;

static inline CyneResult HttpMethod_to_str(HttpMethod method, char* out_str) {
    switch (method) {
        M2S_CASE(UNKNOWN)
        M2S_CASE(GET)
        M2S_CASE(POST)
        M2S_CASE(PUT)
        M2S_CASE(DELETE)
        M2S_CASE(PATCH)
        M2S_CASE(HEAD)
        M2S_CASE(CONNECT)
        M2S_CASE(OPTIONS)
        M2S_CASE(TRACE)
    }
     
    return SUCCESS;
}

typedef struct HttpHeader {
    char field[MAX_HEADER_FIELD_LEN + 1];
    char value[MAX_HEADER_VALUE_LEN + 1];
} HttpHeader;

typedef struct HttpRequest {
    char request_target[MAX_REQUEST_TARGET_LEN + 1];
    char body[MAX_BODY_LEN + 1];
    HttpHeader headers[MAX_HEADER_COUNT];
    size_t headers_count;
    size_t content_length;
    HttpMethod method;
} HttpRequest;

typedef struct HttpResponse {
    HttpHeader headers[MAX_HEADER_COUNT];
    size_t headers_count;
    size_t content_length;
    char reason_phrase[MAX_REASON_PHRASE_LEN + 1];
    char body[MAX_BODY_LEN + 1];
    uint16_t status_code;
} HttpResponse;

CyneResult HttpRequest_from_str(const char* buf, HttpRequest* out);
CyneResult str_to_start_line(char* buf, HttpMethod* out_method, char* out_target, size_t* out_read);
CyneResult str_to_headers(char* buf, HttpHeader* out_headers, size_t* out_header_count, size_t* out_read);
CyneResult str_to_body(char* buf, size_t content_length, char* out_body);
CyneResult str_to_body_chunked(char* buf, char* out_body, size_t* out_read);
void get_header_value(HttpHeader* headers, size_t header_count, char* key, char* out_value);

CyneResult HttpResponse_init(HttpResponse* res);
CyneResult HttpResponse_set_header(HttpResponse* res, const char* field, const char* value);
CyneResult HttpResponse_to_str(HttpResponse* res, char* out_str);
CyneResult HttpResponse_redirect(HttpResponse* res, const char* url, bool preserve_request);  // 302 or 307 (preserve request)
CyneResult HttpResponse_permanent_redirect(HttpResponse* res, const char* url, bool preserve_request);  // 301 or 308 (preserve request)
CyneResult HttpResponse_not_modified(HttpResponse* res);  // 304
CyneResult HttpResponse_bad_request(HttpResponse* res);  // 400
CyneResult HttpResponse_forbidden(HttpResponse* res);  // 403
CyneResult HttpResponse_not_found(HttpResponse* res);  // 404
CyneResult HttpResponse_not_allowed(HttpResponse* res, HttpMethod allowed[], uint8_t allowed_count);  // 405
CyneResult HttpResponse_gone(HttpResponse* res);  // 410
CyneResult HttpResponse_server_error(HttpResponse* res);  // 500
CyneResult HttpResponse_text(HttpResponse* res, const char* text);
CyneResult HttpResponse_json(HttpResponse* res, const JsonValue* json);
CyneResult HttpResponse_json_str(HttpResponse* res, const char* str);

/* End of header */
#endif