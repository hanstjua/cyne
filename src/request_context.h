#ifndef CYNE_REQUEST_CONTEXT_H
#define CYNE_REQUEST_CONTEXT_H

#include "error.h"
#include "message.h"
#include "url.h"

typedef struct RequestContext {
    HttpRequest req;
    HttpResponse res;
    QueryParam params[MAX_QUERY_PARAM_COUNT];
    EndpointVariable vars[MAX_ENDPOINT_VARIABLE_COUNT];
} RequestContext;

CyneResult RequestContext_get_params_value(RequestContext* r, char* name, char* out_value);
CyneResult RequestContext_get_params_count(RequestContext* r, size_t* out_count);
CyneResult RequestContext_get_json(RequestContext* r, Arena* a, JsonValueView** out_json);

/* end of header */
#endif