#include <string.h>
#include "request_context.h"

CyneResult RequestContext_get_params_value(RequestContext* r, char* name, char* out_value) {
    for (size_t i=0; i<MAX_QUERY_PARAM_COUNT; i++) {
        if (strcmp(name, r->params[i].name) == 0) {
            snprintf(out_value, MAX_QUERY_PARAM_NAME_LEN, "%s", r->params[i].value);
            return SUCCESS;
        }

        if (r->params[i].name[0] == 0) {
            break;
        }
    }

    return PARAM_NOT_FOUND;

}

CyneResult RequestContext_get_params_count(RequestContext* r, size_t* out_count) {
    size_t count = 0;
    for (size_t i=0; i<MAX_QUERY_PARAM_COUNT; i++) {
        if (r->params[i].name[0] == 0) {
            break;
        }
        count++;
    }

    *out_count = count;

    return SUCCESS;

}

CyneResult RequestContext_get_json(RequestContext* r, Arena* a, JsonValueView** out_json) {
    char content_type[MAX_HEADER_VALUE_LEN];
    get_header_value(r->req.headers, r->req.headers_count, "Content-Type", content_type);
    if (strncmp("application/json", content_type, strlen("application/json")) != 0) {
        return NOT_JSON_CONTENT;
    }

    *out_json = Arena_json_value_view(a, r->req.body);

    return SUCCESS;
}