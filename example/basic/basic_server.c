#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>

#include "../../src/error.h"
#include "../../src/server.h"
#include "../../src/message.h"
#include "../../src/request_context.h"
#include "../../src/url.h"
#include "../../src/logger.h"
#include "database.h"

static unsigned char g_json_buffer[DEFAULT_ARENA_SIZE];
static Arena g_json_arena;

static UserTable g_user_table;

void post__user(RequestContext* request_context) {
    char content_type[MAX_HEADER_FIELD_LEN];
    get_header_value(request_context->req.headers, request_context->req.headers_count, "Content-Type", content_type);
    if (strcmp("application/json", content_type) != 0) {
        ERROR("Content-Type is not 'application/json'. (Got %s)", content_type);
        HttpResponse_bad_request(&request_context->res);
        return;

    }

    User user;
    user.age = -1;
    user.first_name[0] = 0;
    user.last_name[0] = 0;
    
    char* payload = request_context->req.body;
    JsonObjectView* json = (JsonObjectView*)Arena_json_value_view(&g_json_arena, payload);

    if (json == NULL || json->value.type != JSON_OBJ) {
        ERROR("Unexpected JSON content. (%s)", request_context->req.body);
        HttpResponse_bad_request(&request_context->res);
        return;
    }

    for (size_t i=0; i<json->size; i++) {
        char* key = payload + json->members[i]->key.start;
        size_t key_size = json->members[i]->key.end - json->members[i]->key.start;
        if (strncmp("first_name", key, key_size) == 0) {
            if (json->members[i]->value->type != JSON_STR) {
                ERROR("Invalid first_name type.");
                HttpResponse_bad_request(&request_context->res);
                return;
            }

            char* value = payload + ((JsonNativeView*)json->members[i]->value)->window.start;
            size_t value_size = ((JsonNativeView*)json->members[i]->value)->window.end - ((JsonNativeView*)json->members[i]->value)->window.start;
            if (strnlen(value, value_size) > MAX_NAME_LEN) {
                ERROR("first_name too long.");
                HttpResponse_bad_request(&request_context->res);
                return;
            }

            snprintf(user.first_name, value_size + 1, "%s", value);
        } else if (strncmp("last_name", key, key_size) == 0) {
            if (json->members[i]->value->type != JSON_STR) {
                ERROR("Invalid last_name type.");
                HttpResponse_bad_request(&request_context->res);
                return;
            }

            char* value = payload + ((JsonNativeView*)json->members[i]->value)->window.start;
            size_t value_size = ((JsonNativeView*)json->members[i]->value)->window.end - ((JsonNativeView*)json->members[i]->value)->window.start;
            if (strnlen(value, value_size) > MAX_NAME_LEN) {
                ERROR("last_name too long.");
                HttpResponse_bad_request(&request_context->res);
                return;
            }

            snprintf(user.last_name, value_size + 1, "%s", value);
        } else if (strncmp("age", key, key_size) == 0) {
            if (json->members[i]->value->type != JSON_INT) {
                ERROR("Invalid age type.");
                HttpResponse_bad_request(&request_context->res);
                return;
            }

            user.age = atol(payload + ((JsonNativeView*)json->members[i]->value)->window.start);
            if (user.age < 0) {
                ERROR("age cannot be negative.");
                HttpResponse_bad_request(&request_context->res);
                return;
            }
        } 
    }

    if (UserTable_add(&g_user_table, user) < 0) {
        ERROR("Failed to add to database.");
        HttpResponse_bad_request(&request_context->res);
        return;
    }

    HttpResponse_text(&request_context->res, "OK");
}

void get__user(RequestContext* request_context) {
    char first_name[MAX_ENDPOINT_VARIABLE_VALUE_LEN + 1];
    char last_name[MAX_ENDPOINT_VARIABLE_VALUE_LEN + 1];

    INFO("Reading first_name ...");
    if (RequestContext_get_params_value(request_context, "first_name", first_name) != SUCCESS) {
        ERROR("Failed to read first_name.");
        HttpResponse_bad_request(&request_context->res);
        return;
    }

    INFO("Reading last_name ...");
    if (RequestContext_get_params_value(request_context, "last_name", last_name) != SUCCESS) {
        ERROR("Failed to read last_name.");
        HttpResponse_bad_request(&request_context->res);
        return;
    }

    INFO("Fetching from db ...");
    User user = UserTable_get_user_by_name(&g_user_table, first_name, last_name);
    if (user.id == (uint16_t)-1) {
        ERROR("User '%s %s' not found.", first_name, last_name);
        HttpResponse_bad_request(&request_context->res);
        return;
    }

    INFO("Constructing JSON data ...");
    JsonValue* json = JOBJ(&g_json_arena, 3,
        JMEM(&g_json_arena, "first_name", JSTR(&g_json_arena, user.first_name)),
        JMEM(&g_json_arena, "last_name", JSTR(&g_json_arena, user.last_name)),
        JMEM(&g_json_arena, "age", JINT(&g_json_arena, user.age))
    );

    INFO("Constructing response ...");
    HttpResponse_json(&request_context->res, json);
}

void put__user(RequestContext* request_context) {
    INFO("Checking content type ...");
    char content_type[MAX_HEADER_FIELD_LEN];
    get_header_value(request_context->req.headers, request_context->req.headers_count, "Content-Type", content_type);
    if (strcmp("application/json", content_type) != 0) {
        ERROR("Content-Type is not 'application/json'. (Got %s)", content_type);
        HttpResponse_bad_request(&request_context->res);
        return;

    }

    User user;
    user.id = (uint16_t)-1;
    user.age = -1;
    user.first_name[0] = 0;
    user.last_name[0] = 0;
    
    char* payload = request_context->req.body;
    JsonObjectView* json = (JsonObjectView*)Arena_json_value_view(&g_json_arena, payload);

    if (json->value.type != JSON_OBJ) {
        ERROR("Unexpected JSON content. (%s)", request_context->req.body);
        HttpResponse_bad_request(&request_context->res);
        return;
    }

    for (size_t i=0; i<json->size; i++) {
        char* key = payload + json->members[i]->key.start;
        size_t key_size = json->members[i]->key.end - json->members[i]->key.start;
        if (strncmp("first_name", key, key_size) == 0) {
            if (json->members[i]->value->type != JSON_STR) {
                ERROR("Invalid first_name type.");
                HttpResponse_bad_request(&request_context->res);
                return;
            }

            char* value = payload + ((JsonNativeView*)json->members[i]->value)->window.start;
            size_t value_size = ((JsonNativeView*)json->members[i]->value)->window.end - ((JsonNativeView*)json->members[i]->value)->window.start;
            if (strnlen(value, value_size) > MAX_NAME_LEN) {
                ERROR("first_name too long.");
                HttpResponse_bad_request(&request_context->res);
                return;
            }

            snprintf(user.first_name, value_size + 1, "%s", value);
        } else if (strncmp("last_name", key, key_size) == 0) {
            if (json->members[i]->value->type != JSON_STR) {
                ERROR("Invalid last_name type.");
                HttpResponse_bad_request(&request_context->res);
                return;
            }

            char* value = payload + ((JsonNativeView*)json->members[i]->value)->window.start;
            size_t value_size = ((JsonNativeView*)json->members[i]->value)->window.end - ((JsonNativeView*)json->members[i]->value)->window.start;
            if (strnlen(value, value_size) > MAX_NAME_LEN) {
                ERROR("last_name too long.");
                HttpResponse_bad_request(&request_context->res);
                return;
            }

            snprintf(user.last_name, value_size + 1, "%s", value);
        } else if (strncmp("age", key, key_size) == 0) {
            if (json->members[i]->value->type != JSON_INT) {
                ERROR("Invalid age type.");
                HttpResponse_bad_request(&request_context->res);
                return;
            }

            user.age = atol(payload + ((JsonNativeView*)json->members[i]->value)->window.start);
            if (user.age < 0) {
                ERROR("age cannot be negative.");
                HttpResponse_bad_request(&request_context->res);
                return;
            }
        } 
    }

    if (UserTable_update(&g_user_table, user) < 0) {
        ERROR("Failed to update user in database.");
        HttpResponse_bad_request(&request_context->res);
        return;
    }

    HttpResponse_text(&request_context->res, "OK");
}

void delete__user(RequestContext* request_context) {
    char content_type[MAX_HEADER_FIELD_LEN];
    get_header_value(request_context->req.headers, request_context->req.headers_count, "Content-Type", content_type);
    if (strcmp("application/json", content_type) != 0) {
        ERROR("Content-Type is not 'application/json'. (Got %s)", content_type);
        HttpResponse_bad_request(&request_context->res);
        return;

    }

    User user;
    user.id = (uint16_t)-1;
    user.first_name[0] = 0;
    user.last_name[0] = 0;
    
    char* payload = request_context->req.body;
    JsonObjectView* json = (JsonObjectView*)Arena_json_value_view(&g_json_arena, payload);

    if (json->value.type != JSON_OBJ) {
        ERROR("Unexpected JSON content. (%s)", request_context->req.body);
        HttpResponse_bad_request(&request_context->res);
        return;
    }

    for (size_t i=0; i<json->size; i++) {
        char* key = payload + json->members[i]->key.start;
        size_t key_size = json->members[i]->key.end - json->members[i]->key.start;
        if (strncmp("first_name", key, key_size) == 0) {
            if (json->members[i]->value->type != JSON_STR) {
                ERROR("Invalid first_name type.");
                HttpResponse_bad_request(&request_context->res);
                return;
            }

            char* value = payload + ((JsonNativeView*)json->members[i]->value)->window.start;
            size_t value_size = ((JsonNativeView*)json->members[i]->value)->window.end - ((JsonNativeView*)json->members[i]->value)->window.start;
            if (strnlen(value, value_size) > MAX_NAME_LEN) {
                ERROR("first_name too long.");
                HttpResponse_bad_request(&request_context->res);
                return;
            }

            snprintf(user.first_name, value_size + 1, "%s", value);
        } else if (strncmp("last_name", key, key_size) == 0) {
            if (json->members[i]->value->type != JSON_STR) {
                ERROR("Invalid last_name type.");
                HttpResponse_bad_request(&request_context->res);
                return;
            }

            char* value = payload + ((JsonNativeView*)json->members[i]->value)->window.start;
            size_t value_size = ((JsonNativeView*)json->members[i]->value)->window.end - ((JsonNativeView*)json->members[i]->value)->window.start;
            if (strnlen(value, value_size) > MAX_NAME_LEN) {
                ERROR("last_name too long.");
                HttpResponse_bad_request(&request_context->res);
                return;
            }

            snprintf(user.last_name, value_size + 1, "%s", value);
        }
    }

    if (UserTable_delete(&g_user_table, user) < 0) {
        ERROR("Failed to update user in database.");
        HttpResponse_bad_request(&request_context->res);
        return;
    }

    HttpResponse_text(&request_context->res, "OK");
}

CyneResult before_loop() {
    CyneResult result = Arena_init(&g_json_arena, g_json_buffer, DEFAULT_ARENA_SIZE);
    if (result != SUCCESS) {
        return result;
    }

    if (UserTable_init(&g_user_table) != 0) {
        ERROR("Failed to initialize database.");
        return RUNTIME_ERROR;
    }

    return SUCCESS;
}

CyneResult handle_request(RequestContext* request_context) {
    CyneResult result = SUCCESS;
    bool route_match = false;
    EndpointVariable endpoint_vars[MAX_ENDPOINT_VARIABLE_COUNT];
    char* request_target = request_context->req.request_target;
    char endpoint[MAX_ENDPOINT_LEN]; 

    INFO("Extracting endpoint ...");
    if (strchr(request_target, '?') != NULL) {
        snprintf(endpoint, strchr(request_target, '?') - request_target + 1, "%s", request_target);
    } else {
        sprintf(endpoint, "%s", request_target);
    }

    INFO("Matching endpoint to route ...");
    bool should_route = true;
    if (parse_endpoint(endpoint, "/user", &route_match, endpoint_vars) != SUCCESS) {
        ERROR("Error while parsing endpoint '%s' with rule '%s'", endpoint, "/user");
        HttpResponse_bad_request(&request_context->res);
        should_route = false;
    }

    if (!route_match) {
        ERROR("Unrecognised endpoint %s", endpoint);
        HttpResponse_bad_request(&request_context->res);
        should_route = false;
    }

    if (should_route) {
        switch(request_context->req.method) {
            case GET:
            get__user(request_context);
            break;

            case POST:
            post__user(request_context);
            break;

            case PUT:
            put__user(request_context);
            break;

            case DELETE:
            delete__user(request_context);
            break;

            default:
            ERROR("Unsupported method.");
            HttpResponse_bad_request(&request_context->res);
        }
    }


    result = Arena_reset(&g_json_arena);

    return result;
}

CyneResult after_loop() { 
    g_user_table.row_count = 0;
    return SUCCESS;
}