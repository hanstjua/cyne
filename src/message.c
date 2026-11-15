#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "json.h"
#include "logger.h"
#include "message.h"

typedef enum {
    METHOD,
    REQUEST_TARGET,
    HEADER_FIELD,
    HEADER_VALUE,
    BODY,
    COMPLETE,
    ERROR
} __ParserStage;

char* strnspace(char* str, size_t size) {
    char c;
    size_t i;
    for (i=0; i<size; i++) {
        c = str[i];
        if (c == 0 || isspace(c)) break;
    }
    return isspace(c) ? str + i : NULL;
}

char* strnchr(char* str, char character, size_t size) {
    char c;
    size_t i;
    for (i=0; i<size; i++) {
        c = str[i];
        if (c == 0 || c == character) break;
    }
    return c == character ? str + i : NULL;
}

HttpMethod get_method_from_buf(const char* buf, size_t count) {
    if (strncmp(buf, "GET", count) == 0) {
        return GET;
    }
    if (strncmp(buf, "POST", count) == 0) {
        return POST;
    }
    if (strncmp(buf, "PUT", count) == 0) {
        return PUT;
    }
    if (strncmp(buf, "DELETE", count) == 0) {
        return DELETE;
    }
    if (strncmp(buf, "PATCH", count) == 0) {
        return PATCH;
    }
    if (strncmp(buf, "OPTIONS", count) == 0) {
        return OPTIONS;
    }
    if (strncmp(buf, "TRACE", count) == 0) {
        return TRACE;
    }
    if (strncmp(buf, "HEAD", count) == 0) {
        return HEAD;
    }
    if (strncmp(buf, "CONNECT", count) == 0) {
        return CONNECT;
    }
    return UNKNOWN;
}

bool set_header(HttpHeader headers[], size_t* header_count, char* start_of_field, size_t field_len, char* start_of_value, size_t value_len) {
    size_t header_idx, char_index;
    bool field_found = false;
    size_t cur_value_len;

    for (header_idx=0; header_idx<*header_count; header_idx++) {
        for (char_index=0; char_index<field_len; char_index++) {
            if (headers[header_idx].field[char_index] != tolower(*(start_of_field + char_index))) {
                break;
            } else if (char_index == field_len - 1) {
                field_found = true;
            }
        }
        if (field_found) break;
    }

    if (header_idx == *header_count) { // new field detected
        for (char_index=0; char_index<field_len; char_index++) {
            headers[header_idx].field[char_index] = tolower(*(start_of_field + char_index)); // lower case field name
        }
        headers[header_idx].field[field_len] = 0; // ensure field name is null terminated
        strncpy(headers[header_idx].value, start_of_value, value_len);
        headers[header_idx].value[value_len] = 0; // ensure value is null terminated
        *header_count += 1;
    } else {
        cur_value_len = strlen(headers[header_idx].value);
        headers[header_idx].value[cur_value_len] = ',';
        strncpy(headers[header_idx].value + cur_value_len + 1, start_of_value, value_len);
        headers[header_idx].value[value_len + cur_value_len + 1] = 0; // ensure value is null terminated
    }

    return true;
}

static inline void parse_method(const char* buf, HttpMethod* method, char** end_of_method, __ParserStage* stage, CyneResult* ret) {
    *end_of_method = strchr(buf, ' ');
    if (*end_of_method != NULL) {
        *method = get_method_from_buf(buf, *end_of_method - buf);
        if (*method == UNKNOWN) {
            *stage = ERROR;
            *ret = METHOD_PARSE_FAILED;
        } else {
            *stage = REQUEST_TARGET;
        }
    }
}

void parse_request_target(char** start_of_target, char* end_of_method, char** end_of_target, __ParserStage* stage, CyneResult* ret, char* request_target, char** start_of_field) {
    *start_of_target = end_of_method + 1;
    *end_of_target = strchr(*start_of_target, ' ');
    if (*end_of_target == NULL || *start_of_target == *end_of_target) {
        *stage = ERROR;
        *ret = REQUEST_TARGET_PARSE_FAILED;
    } else if ((*end_of_target - *start_of_target) > MAX_REQUEST_TARGET_LEN) {
        *stage = ERROR;
        *ret = REQUEST_TARGET_TOO_LONG;
    } else {
        memset(request_target, 0, MAX_REQUEST_TARGET_LEN + 1);
        strncpy(request_target, *start_of_target, *end_of_target - *start_of_target);

        *start_of_field = strstr(*end_of_target, "\r\n");
        if (*start_of_field != NULL) {
            *stage = HEADER_FIELD;
            *start_of_field += 2;
        } else {
            *stage = ERROR;
            *ret = START_LINE_CRLF_NOT_FOUND;
        }
    }
}

void parse_header_value(
    char** start_of_value,
    char** end_of_header_line,
    char** end_of_value, 
    HttpHeader* headers,
    size_t* header_count,
    char** start_of_field,
    char* end_of_field,
    size_t* body_len,
    char** start_of_body,
    __ParserStage* stage,
    CyneResult* ret
) {
    while (isspace(*(++(*start_of_value))));
    *end_of_header_line = strstr(*start_of_value, "\r\n");

    if (*end_of_header_line != NULL) {
        // remove trailing whitespace(s)
        *end_of_value = strnchr(*start_of_value, ' ', *end_of_header_line - *start_of_value);
        *end_of_value = *end_of_value == NULL ? *end_of_header_line : *end_of_value;

        set_header(headers, header_count, *start_of_field, end_of_field - *start_of_field, *start_of_value, *end_of_value - *start_of_value);
        
        if (strcmp(headers[*header_count-1].field, "content-length") == 0) {
            *body_len = atoi(headers[*header_count-1].value);
            DEBUG("content-length detected %zu", (long)*body_len);
        }

        *start_of_field = *end_of_header_line + 2;
        if (strncmp(*start_of_field, "\r\n", 2) == 0) {
            *start_of_body = *start_of_field + 2;
            *stage = BODY;
        } else {
            *stage = HEADER_FIELD;
        }
    } else {
        *stage = ERROR;
        *ret = HEADER_CRLF_NOT_FOUND;
    }
}

void parse_header_field(size_t* header_count, char* start_of_field, char** end_of_field, __ParserStage* stage, CyneResult* ret, char** start_of_value) {
    if (*header_count < MAX_HEADER_COUNT) {
        *end_of_field = strchr(start_of_field, ':');
        if (*end_of_field != NULL) {
            if(strnspace(start_of_field, *end_of_field - start_of_field) != NULL) {
                *stage = ERROR;
                *ret = HEADER_FIELD_CONTAINS_WHITESPACE;
            } else {
                *stage = HEADER_VALUE;
                *start_of_value = *end_of_field;
            }
        } else {
            *stage = ERROR;
            *ret = HEADER_FIELD_DELIMITER_NOT_FOUND;
        }
    } else {
        *stage = ERROR;
        *ret = HEADER_MAX_COUNT_EXCEEDED;
    }
}

bool has_field(HttpHeader* headers, size_t header_count, char* field) {
    bool found = false;
    size_t field_len = strlen(field), i = 0;
    while (i<header_count && !found) {
        for (size_t j=0; j<field_len; j++) {
            if (headers[i].field[j] != *(field + j)) break;
            else if (j == field_len - 1) {
                found = true;
            }
        }
        i++;
    }

    return found;
}

bool aredigits(char* s, size_t size) {
    bool ret = true;
    for (size_t i=0; i<size; i++) {
        ret &= isdigit(s[i]);
        if (!ret) break;
    }
    return ret;
}

void parse_one_body_chunk(char** start_of_body, __ParserStage* stage, CyneResult* ret, char* body, size_t* read) {
    char* size_end = strstr(*start_of_body, "\r\n");
    char raw_chunk_size[16] = "";
    size_t chunk_size = 0;
    
    if (size_end != NULL && size_end - *start_of_body <= 15) {
        raw_chunk_size[0] = 0;
        strncpy(raw_chunk_size, *start_of_body, size_end - *start_of_body);
        raw_chunk_size[size_end - *start_of_body] = 0;

        chunk_size = atoi(raw_chunk_size);
        *start_of_body = size_end + 2;

        if (chunk_size == 0) {
            if (aredigits(raw_chunk_size, strlen(raw_chunk_size))) *stage = COMPLETE;
            else {
                *stage = ERROR;
                *ret = BODY_CHUNKS_PARSE_FAILED;
            }
        } else if (strncmp(*start_of_body + chunk_size, "\r\n", 2) != 0) {
            *stage = ERROR;
            *ret = BODY_CHUNKS_PARSE_FAILED;
        } else if (*read + chunk_size > MAX_BODY_LEN) {
            *stage = ERROR;
            *ret = BODY_TOO_LONG;
        } else {
            strncpy(body + *read, *start_of_body, chunk_size);
            *(body + *read + chunk_size + 1) = 0;
            *read += chunk_size;
            *start_of_body += chunk_size + 2;
        }

    } else if (size_end != NULL && size_end - *start_of_body > 15) {
        *stage = ERROR;
        *ret = BODY_CHUNK_TOO_LONG;
    } else {
        *stage = ERROR;
        *ret = BODY_CHUNKS_PARSE_FAILED;
    }
}

void parse_chunked_body(__ParserStage* stage, char* start_of_body, CyneResult* ret, size_t* body_len, char* body) {
    size_t read = 0;
    while (*stage != COMPLETE && *stage != ERROR) {
        parse_one_body_chunk(&start_of_body, stage, ret, body, &read);
    }
    *body_len = strlen(body);
}

void parse_body(char* start_of_body, size_t* body_len, char* body, __ParserStage* stage, CyneResult* ret, HttpHeader* headers, size_t header_count) {
    if (strlen(start_of_body) > MAX_BODY_LEN) { // Body too long
        *stage = ERROR;
        *ret = BODY_TOO_LONG;
    } else if (*body_len > 0 && strlen(start_of_body) != *body_len) {  // Incorrect body length
        ERROR("Body size mismatch: expected %zu actual %zu", *body_len, strlen(start_of_body));
        *stage = ERROR;
        *ret = BODY_LEN_MISMATCH;
    } else if (*start_of_body == 0) {  // Body is empty
        *body = 0;
        *stage = COMPLETE;
    } else if (*body_len == 0 && has_field(headers, header_count, "transfer-encoding")) { // TODO: find a way to check that transfer-encoding is chunked
        parse_chunked_body(stage, start_of_body, ret, body_len, body);
    } else {
        strcpy(body, start_of_body);
        *stage = COMPLETE;
    }
}

CyneResult HttpRequest_from_str(const char* buf, HttpRequest* out) {
    char* end_of_method, *start_of_target, *end_of_target, *start_of_field, *end_of_field, *start_of_value, *end_of_header_line, *end_of_value, *start_of_body;
    char request_target[MAX_REQUEST_TARGET_LEN + 1], body[MAX_BODY_LEN + 1];
    size_t header_count = 0, body_len = 0;
    HttpMethod method = UNKNOWN;
    HttpHeader headers[MAX_HEADER_COUNT];
    CyneResult ret = SUCCESS;
    __ParserStage stage = METHOD;
    DEBUG("Request:\n%s", buf);
    while (stage != COMPLETE && stage != ERROR) {
        switch (stage) {
            case METHOD:
            DEBUG("Method.");
            parse_method(buf, &method, &end_of_method, &stage, &ret);
            break;

            case REQUEST_TARGET:
            DEBUG("Request target.");
            parse_request_target(&start_of_target, end_of_method, &end_of_target, &stage, &ret, request_target, &start_of_field);
            break;

            case HEADER_FIELD:
            DEBUG("Header field.");
            parse_header_field(&header_count, start_of_field, &end_of_field, &stage, &ret, &start_of_value);
            break;

            case HEADER_VALUE:
            DEBUG("Header value.");
            parse_header_value( &start_of_value, &end_of_header_line, &end_of_value, headers, &header_count, &start_of_field, end_of_field, &body_len, &start_of_body, &stage, &ret);
            break;

            case BODY:
            DEBUG("Body.");
            parse_body(start_of_body, &body_len, body, &stage, &ret, headers, header_count);
            break;

            case COMPLETE:
            case ERROR:
            stage = COMPLETE;
        }
    }
    
    DEBUG("Out of loop.");

    if (stage != ERROR) {
        out->method = method;
        strcpy(out->request_target, request_target);
        strcpy(out->body, body);
        out->content_length = strlen(body);
        for(size_t i=0; i<header_count; i++) {
            out->headers[i] = headers[i];
            if (strcmp("content-length", out->headers[i].field) == 0) {
                out->content_length = atol(out->headers[i].value);
            }
        }
        out->headers_count = header_count;
    }

    DEBUG("Exitting ...");

    return ret;
}

CyneResult str_to_start_line(char* buf, HttpMethod* out_method, char* out_target, size_t* out_read) {
    char* end_of_method, *start_of_target, *end_of_target, *start_of_field;
    __ParserStage stage;
    CyneResult ret = SUCCESS;
    HttpMethod method = UNKNOWN;
    char target[MAX_REQUEST_TARGET_LEN + 1] = "";
    size_t read = 0;

    parse_method(buf, &method, &end_of_method, &stage, &ret);
    if (ret != SUCCESS) return ret;

    parse_request_target(&start_of_target, end_of_method, &end_of_target, &stage, &ret, target, &start_of_field);
    if (ret != SUCCESS) return ret;

    read = start_of_field - buf;
    *out_method = method;
    strcpy(out_target, target);
    *out_read = read;

    return ret;
}

CyneResult str_to_headers(char* buf, HttpHeader* out_headers, size_t* out_header_count, size_t* out_read) {
    size_t header_count = 0, body_len;
    char* start_of_field = buf, *end_of_field, *start_of_value, *end_of_header_line, *end_of_value, *start_of_body;
    __ParserStage stage = HEADER_FIELD;
    HttpHeader headers[MAX_HEADER_COUNT];
    CyneResult ret = SUCCESS;
    
    while (ret == SUCCESS && (stage == HEADER_FIELD || stage == HEADER_VALUE)) {
        switch (stage) {
            case HEADER_FIELD:
            parse_header_field(&header_count, start_of_field, &end_of_field, &stage, &ret, &start_of_value);
            break;
            case HEADER_VALUE:
            parse_header_value(
                &start_of_value,
                &end_of_header_line,
                &end_of_value, 
                headers,
                &header_count,
                &start_of_field,
                end_of_field,
                &body_len,
                &start_of_body,
                &stage,
                &ret
            );
            break;
            default:
            break;
        }
    }

    if (stage != ERROR) {
        for (size_t i=0; i<header_count; i++) {
            strcpy(out_headers[i].field, headers[i].field);
            strcpy(out_headers[i].value, headers[i].value);
        }
        *out_header_count = header_count;
        *out_read = start_of_body - buf;
    }

    return ret;
}

CyneResult str_to_body(char* buf, size_t content_length, char* out_body) {
    size_t body_len = content_length;
    __ParserStage stage;
    CyneResult ret = SUCCESS;

    parse_body(buf, &body_len, out_body, &stage, &ret, NULL, 0);

    return ret;
}

CyneResult str_to_body_chunked(char* buf, char* out_body, size_t* out_read) {
    char* start_of_body = buf;
    __ParserStage stage;
    CyneResult ret = SUCCESS;
    *out_read = 0;

    parse_one_body_chunk(&start_of_body, &stage, &ret, out_body, out_read);

    return ret;
}

void get_header_value(HttpHeader* headers, size_t header_count, char* key, char* out_value) {
    char lower_key[MAX_HEADER_FIELD_LEN + 1] = "";
    size_t key_len = strlen(key);

    if (key_len > MAX_HEADER_FIELD_LEN) {
        return;
    }

    for (size_t i=0; i<key_len; i++) {
        lower_key[i] = tolower(*(key + i));
    }

    for (size_t i=0; i<header_count; i++) {
        if (strlen(headers[i].field) == key_len) {
            bool same = true; 
            for (size_t j=0; j<key_len; j++) {
                same = tolower(headers[i].field[j]) == lower_key[j];
                if (!same) {
                    break;
                }
            }

            if (same) {
                strcpy(out_value, headers[i].value);
                break;
            }
        } 
    }
}

void strip(char* dst, char* src) {
    size_t ws_index = -1;
    strcpy(dst, src + strspn(src, " \t\v\f"));
    ws_index = strspn(dst, " \t\v\f"); 
    if (ws_index > 0) {
        dst[ws_index] = 0;
    }
}

CyneResult HttpResponse_init(HttpResponse* res) {
    res->headers_count = 0;
    res->content_length = 0;
    res->reason_phrase[0] = 0;
    res->body[0] = 0;
    res->status_code = 0;

    return SUCCESS;
}

CyneResult HttpResponse_set_header(HttpResponse* res, const char* field, const char* value) {
    size_t index = res->headers_count;
    bool found = false;

    if (strlen(field) > MAX_HEADER_FIELD_LEN) {
        return HEADER_FIELD_TOO_LONG;
    }

    if (strlen(value) > MAX_HEADER_VALUE_LEN) {
        return HEADER_VALUE_TOO_LONG;
    }

    if (strpbrk(field, " \t\n\r\v\f") != NULL) {
        return HEADER_FIELD_CONTAINS_WHITESPACE;
    }
    
    for (size_t i=0; i<res->headers_count; i++) {
        if (strcmp(field, res->headers[i].field) == 0) {
            index = i;
            found = true;
            break;
        }
    }

    if (res->headers_count == MAX_HEADER_COUNT && !found) {
        return TOO_MANY_HEADERS;
    }

    snprintf(res->headers[index].field, MAX_HEADER_FIELD_LEN + 1, "%s", field);
    snprintf(res->headers[index].value, MAX_HEADER_VALUE_LEN + 1, "%s", value);
    res->headers_count = found ? res->headers_count : res->headers_count + 1;

    return SUCCESS;
}

CyneResult HttpResponse_to_str(HttpResponse* res, char* out_str) {
    char str[MAX_RESPONSE_LEN + 1] = "";
    size_t write = 0;
    char field[MAX_HEADER_FIELD_LEN + 1] = "", value[MAX_HEADER_VALUE_LEN + 1] = "";
    CyneResult ret = SUCCESS;

    write += sprintf(str, "HTTP/1.1 %hu %s\r\n", res->status_code, res->reason_phrase);    

    if (!has_field(res->headers, res->headers_count, "content-length")) {
        write += sprintf(str + write, "Content-Length: %zu\r\n", res->content_length);
    }
    for (size_t i=0; i<res->headers_count; i++) {
        strip(field, res->headers[i].field);
        strip(value, res->headers[i].value);
        write += sprintf(str + write, "%s: %s\r\n", field, value);
    }
    
    write += sprintf(str + write, "\r\n%s", res->body);

    strcpy(out_str, str);

    return ret;
}

CyneResult HttpResponse_redirect(HttpResponse* res, const char* url, bool preserve_request) {
    HttpResponse_set_header(res, "Location", url);
    res->status_code = preserve_request ? 307 : 302;
    snprintf(res->reason_phrase, MAX_REASON_PHRASE_LEN + 1, "%s", "Found");

    return SUCCESS;
}

CyneResult HttpResponse_permanent_redirect(HttpResponse* res, const char* url, bool preserve_request) {
    HttpResponse_set_header(res, "Location", url);
    res->status_code = preserve_request ? 308 : 301;
    snprintf(res->reason_phrase, MAX_REASON_PHRASE_LEN + 1, "%s", "Found");

    return SUCCESS;
}

CyneResult HttpResponse_not_modified(HttpResponse* res) {
    res->status_code = 304;
    snprintf(res->reason_phrase, MAX_REASON_PHRASE_LEN + 1, "%s", "Not Modified");

    return SUCCESS;

}

CyneResult HttpResponse_bad_request(HttpResponse* res) {
    char* body = "<b>400 Bad Request</b>";
    res->status_code = 400;
    snprintf(res->reason_phrase, MAX_REASON_PHRASE_LEN + 1, "%s", "Bad Request");
    snprintf(res->body, MAX_BODY_LEN, "%s", body);
    res->content_length = strlen(body);

    return SUCCESS;

}

CyneResult HttpResponse_forbidden(HttpResponse* res) {
    char* body = "<b>403 Forbidden</b>";
    res->status_code = 403;
    snprintf(res->reason_phrase, MAX_REASON_PHRASE_LEN + 1, "%s", "Forbidden");
    snprintf(res->body, MAX_BODY_LEN, "%s", body);
    res->content_length = strlen(body);

    return SUCCESS;

}

CyneResult HttpResponse_not_found(HttpResponse* res) {
    char* body = "<b>404 Not Found</b>";
    res->status_code = 404;
    snprintf(res->reason_phrase, MAX_REASON_PHRASE_LEN + 1, "%s", "Not Found");
    snprintf(res->body, MAX_BODY_LEN, "%s", body);
    res->content_length = strlen(body);

    return SUCCESS;

}

CyneResult HttpResponse_not_allowed(HttpResponse* res, HttpMethod allowed[], uint8_t allowed_count) {
    char value[128];
    char method[16];
    char* body = "<b>405 Method Not Allowed</b>";
    CyneResult result;

    for (uint8_t i=0; i<allowed_count; i++) {
        result = HttpMethod_to_str(allowed[i], method);
        if (result != SUCCESS) {
            return result;
        }
        if (i == 0) {
            sprintf(value, "%s", method);
        } else {
            sprintf(value + strlen(value), ", %s", method);
        }
    }

    HttpResponse_set_header(res, "Allow", value);
    res->status_code = 405;
    snprintf(res->reason_phrase, MAX_REASON_PHRASE_LEN + 1, "%s", "Method Not Allowed");
    snprintf(res->body, MAX_BODY_LEN, "%s", body);
    res->content_length = strlen(body);

    return SUCCESS;

}

CyneResult HttpResponse_gone(HttpResponse* res) {
    char* body = "<b>410 Gone</b>";
    res->status_code = 410;
    snprintf(res->reason_phrase, MAX_REASON_PHRASE_LEN + 1, "%s", "Gone");
    snprintf(res->body, MAX_BODY_LEN, "%s", body);
    res->content_length = strlen(body);

    return SUCCESS;

}

CyneResult HttpResponse_server_error(HttpResponse* res) {
    char* body = "<b>500 Server Error</b>";
    res->status_code = 500;
    snprintf(res->reason_phrase, MAX_REASON_PHRASE_LEN + 1, "%s", "Server Error");
    snprintf(res->body, MAX_BODY_LEN, "%s", body);
    res->content_length = strlen(body);

    return SUCCESS;

}

CyneResult HttpResponse_text(HttpResponse* res, const char* text) {
    size_t content_length = strlen(text);
    if (content_length > MAX_BODY_LEN) {
        return BODY_TOO_LONG;
    }

    res->status_code = 200;
    snprintf(res->reason_phrase, MAX_REASON_PHRASE_LEN + 1, "%s", "OK");
    snprintf(res->body, MAX_BODY_LEN, "%s", text);
    res->content_length = content_length;

    return SUCCESS;

}

CyneResult HttpResponse_json(HttpResponse* res, const JsonValue* json) {
    CyneResult result = SUCCESS;
    result = JsonValue_serialize(json, MAX_BODY_LEN, res->body);
    if (result != SUCCESS) {
        return result == JSON_BUFFER_OVERFLOW ? BODY_TOO_LONG : result;
    }

    res->status_code = 200;
    snprintf(res->reason_phrase, MAX_REASON_PHRASE_LEN + 1, "%s", "OK");
    res->content_length = strlen(res->body);

    return SUCCESS;
}

CyneResult HttpResponse_json_str(HttpResponse* res, const char* str);
