#ifndef CYNE_URL_H
#define CYNE_URL_H

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include "configs.h"
#include "error.h"

typedef enum EndpointVariableType {
    STRING,
    PATH,
    INT,
    FLOAT,
    UNKNOWN_VAR_TYPE
} EndpointVariableType;

typedef enum RuleTokenType {
    STRING_TOK,
    PATH_TOK,
    INT_TOK,
    FLOAT_TOK,
    LITERAL_TOK,
    UNKNOWN_TOK
} RuleTokenType;

typedef struct RuleToken {
    RuleTokenType type;
    union {
        char name[MAX_ENDPOINT_VARIABLE_NAME_LEN];
        struct { size_t start, end; } rule_index;
    };
} RuleToken;

typedef struct EndpointVariable {
    char name[MAX_ENDPOINT_VARIABLE_NAME_LEN + 1];
    union {
        char s[MAX_ENDPOINT_VARIABLE_VALUE_LEN + 1];
        char p[MAX_ENDPOINT_VARIABLE_VALUE_LEN + 1];
        int32_t i;
        float f;
    };
    EndpointVariableType type;
} EndpointVariable;

typedef struct QueryParam {
    char name[MAX_QUERY_PARAM_NAME_LEN + 1];
    char value[MAX_QUERY_PARAM_VALUE_LEN + 1];
} QueryParam;

typedef enum QueryParamParserState {
    PARSE_NAME,
    PARSE_VALUE
} QueryParamParserState;

typedef struct QueryParamParser {
    QueryParam* params;
    QueryParam next_param;
    QueryParamParserState state;
    size_t params_capacity;
    size_t params_count;
} QueryParamParser;

CyneResult QueryParamParser_init(QueryParamParser* p, QueryParam params[], size_t params_capacity);
CyneResult QueryParamParser_parse(QueryParamParser* p, char* url, size_t* out_count);

CyneResult parse_endpoint(char* endpoint, char* rule, bool* out_match, EndpointVariable out_vars[]);
CyneResult tokenize_rule(char* rule, RuleToken out_tokens[], size_t* out_size);
CyneResult parse_params(char* url, QueryParam out_params[], size_t* out_count, size_t* out_consumed);

/* End of header */
#endif