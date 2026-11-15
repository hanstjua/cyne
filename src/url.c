#include <ctype.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "logger.h"
#include "url.h"

RuleTokenType c_to_rule_token_type(char c) {
    switch (c) {
        case 's': return STRING_TOK;
        case 'p': return PATH_TOK;
        case 'i': return INT_TOK;
        case 'f': return FLOAT_TOK;
        default: return STRING_TOK;
    }
}

void get_token_name(RuleToken* t, char* raw_token, size_t len) {
    char type = strchr(raw_token, ':') == raw_token + 2 ? raw_token[1] : 0; 
    size_t name_index = type == 0 ? 1 : 3;
    size_t name_len = type == 0 ? len - 2 : len - 4;
    strncpy(t->name, raw_token + name_index, name_len);
    t->name[len] = 0;
}

typedef struct ParserNode {
    RuleToken token;
    size_t index, start, end;
} ParserNode;

typedef struct ParserNodeStack {
    ParserNode nodes[MAX_ENDPOINT_VARIABLE_COUNT];
    size_t size;
} ParserNodeStack;

void pop(ParserNodeStack* stack, ParserNode* out_node) {
    if (stack->size > 0) {
        *out_node = stack->nodes[stack->size - 1];
        stack->size--;
    }
}

CyneResult push(ParserNodeStack* stack, ParserNode node) {
    if (stack->size == MAX_ENDPOINT_VARIABLE_COUNT) {
        // TODO: figure out how to handle
        // No need to handle now as impossible to push more nodes than max var limit
    } else {
        stack->nodes[stack->size] = node;
        stack->size++;
    }
    return SUCCESS;
}

CyneResult tokenize_rule(char* rule, RuleToken out_tokens[], size_t* out_size) {
    *out_size = 0;
    size_t raw_token_len = 0;
    RuleToken token;
    size_t cur_rule_index = 0;
    char* next_var_ptr = NULL;

    while (*rule != 0) {
        if (*rule == '<') {
            if (*out_size == MAX_ENDPOINT_VARIABLE_COUNT) {
                return TOO_MANY_ENDPOINT_VARIABLES;
            }

            raw_token_len = strchr(rule, '>') - rule + 1;
            if (raw_token_len > MAX_ENDPOINT_VARIABLE_NAME_LEN) {
                return ENDPOINT_VARIABLE_NAME_TOO_LONG;
            }

            get_token_name(&token, rule, raw_token_len);
            token.type = c_to_rule_token_type(*(rule + 1));
        } else {
            next_var_ptr = strchr(rule, '<');
            raw_token_len = next_var_ptr != NULL ? next_var_ptr - rule : strlen(rule);
            token.rule_index.start = cur_rule_index;
            token.rule_index.end = cur_rule_index+raw_token_len;
            token.type = LITERAL_TOK;
        }

        out_tokens[*out_size] = token;
        *out_size += 1;
        rule += raw_token_len;
        cur_rule_index += raw_token_len;
    }

    return SUCCESS;
}

void match_token(RuleToken token, char* rule, char* endpoint, size_t endpoint_len, bool* out_match, bool* out_match_still_possible) {
    switch (token.type) {
        case STRING_TOK:
        if (memchr(endpoint, '/', endpoint_len) != NULL) {
            *out_match = false;
            *out_match_still_possible = false;
        } else {
            *out_match = true;
        }
        break;

        case PATH_TOK:
        *out_match = true;
        break;

        case INT_TOK:
        for (size_t i=0; i<endpoint_len; i++) {
            if (!isnumber(endpoint[i])) {
                *out_match = false;
                *out_match_still_possible = false;
                return;
            }
        }
        *out_match = true;
        break;

        case FLOAT_TOK:
        for (size_t i=0; i<endpoint_len; i++) {
            if (!isnumber(endpoint[i]) && endpoint[i] != '.') {
                *out_match = false;
                *out_match_still_possible = false;
                return;
            }
        }
        *out_match = true;
        break;

        case LITERAL_TOK:
        if (endpoint_len <= (token.rule_index.end - token.rule_index.start)) {
            for (size_t i=token.rule_index.start; i<token.rule_index.end; i++) {
                if (rule[i] != endpoint[i - token.rule_index.start]) {
                    *out_match = false;
                    *out_match_still_possible = i == endpoint_len;
                    return;
                }
            }
            *out_match = endpoint_len == (token.rule_index.end - token.rule_index.start);
            *out_match_still_possible = true;

        } else {
            *out_match = false;
            *out_match_still_possible = false;
        }
        break;

        default:
        *out_match = false;
    }
}

EndpointVariableType rule_token_type_to_endpoint_var_type(RuleTokenType r) {
    switch (r) {
        case STRING_TOK: return STRING;
        case PATH_TOK: return PATH;
        case INT_TOK: return INT;
        case FLOAT_TOK: return FLOAT;
        default: return UNKNOWN_VAR_TYPE;
    }
}

// Use 'N' since the name "strnlen" may already exist on some platforms
size_t strNlen(char* s, size_t max) {
    char* endptr = memchr(s, 0, max);
    return endptr == NULL ? 0 : endptr - s;
}

CyneResult copy_to_out_vars(size_t tokens_count, RuleToken tokens[], size_t token_match_indices[], size_t vars_count, size_t value_len, char* endpoint, EndpointVariable out_vars[], bool* out_match) {
    for (size_t i=0; i<tokens_count; i++) {
        if (tokens[i].type != LITERAL_TOK) {
            out_vars[vars_count].type = rule_token_type_to_endpoint_var_type(tokens[i].type);
            strcpy(out_vars[vars_count].name, tokens[i].name);
            value_len = i == (tokens_count - 1) ? strlen(endpoint + token_match_indices[i]) : token_match_indices[i+1] - token_match_indices[i];
            switch (tokens[i].type) {
                case STRING_TOK:
                if (value_len > MAX_ENDPOINT_VARIABLE_VALUE_LEN) {
                    *out_match = false;
                    out_vars[0].type = UNKNOWN_VAR_TYPE;
                    return ENDPOINT_VARIABLE_VALUE_TOO_LONG;
                }
                strncpy(out_vars[vars_count].s, (endpoint + token_match_indices[i]), value_len);
                out_vars[vars_count].s[value_len] = 0;
                break;

                case PATH_TOK:
                if (value_len > MAX_ENDPOINT_VARIABLE_VALUE_LEN) {
                    *out_match = false;
                    out_vars[0].type = UNKNOWN_VAR_TYPE;
                    return ENDPOINT_VARIABLE_VALUE_TOO_LONG;
                }
                strncpy(out_vars[vars_count].p, (endpoint + token_match_indices[i]), value_len);
                out_vars[vars_count].p[value_len] = 0;
                break;

                case INT_TOK:
                // TODO: How to check if value overflows int32_t
                out_vars[vars_count].i = strtol(endpoint + token_match_indices[i], NULL, 10);
                break;

                case FLOAT_TOK:
                // TODO: How to check if value overflows double?
                out_vars[vars_count].f = strtod(endpoint + token_match_indices[i], NULL);

                default:
                break;
            }
            vars_count++;
        }
    }
    
    if (vars_count < MAX_ENDPOINT_VARIABLE_COUNT) {
        out_vars[vars_count].type = UNKNOWN_VAR_TYPE;  // set 'terminating' endpoint vars
    }

    return SUCCESS;
}

CyneResult parse_endpoint(char* endpoint, char* rule, bool* out_match, EndpointVariable out_vars[]) {
    ParserNodeStack stack;
    RuleToken tokens[MAX_ENDPOINT_VARIABLE_COUNT];
    size_t tokens_count;
    ParserNode cur_node;
    size_t endpoint_len;
    size_t rule_len;
    bool match = false;
    bool match_still_possible = false;
    size_t vars_count = 0;
    size_t token_match_indices[MAX_ENDPOINT_VARIABLE_COUNT];
    size_t value_len = 0;
    CyneResult ret = SUCCESS;

    // Set 'terminating' endpoint variable
    out_vars[0].type = UNKNOWN_VAR_TYPE;

    // Set match to false by default
    *out_match = false;

    rule_len = strNlen(rule, MAX_ENDPOINT_RULE_LEN);
    if (rule_len == 0) {
        if (*rule != 0) {  // endpoint Rule too long
            return ENDPOINT_RULE_TOO_LONG;
        } else {
            return EMPTY_ENDPOINT_RULE;
        }
    }

    endpoint_len = strNlen(endpoint, MAX_ENDPOINT_LEN);
    if (endpoint_len == 0) {
        if (*endpoint != 0) {  // endpoint too long
            return ENDPOINT_TOO_LONG;
        } else {
            return EMPTY_ENDPOINT;
        }
    }

    ret = tokenize_rule(rule, tokens, &tokens_count);
    if (ret != SUCCESS) {
        return ret;
    }

    stack.size = 0;
    ret = push(&stack, (ParserNode){ .token=tokens[0], .index=0, .start=0, .end=1 });
    if (ret != SUCCESS) {
        return ret;
    }

    *out_match = false;
    for (uint64_t i=0; i<MAX_ENDPOINT_MATCHING_ATTEMPTS_PER_RULE; i++) {
        pop(&stack, &cur_node);
        match_token(cur_node.token, rule, endpoint + cur_node.start, cur_node.end - cur_node.start, &match, &match_still_possible);

        if (match) {
            token_match_indices[cur_node.index] = cur_node.start;

            if (cur_node.index == tokens_count - 1 && cur_node.end == endpoint_len) {
                *out_match = true;
                ret = SUCCESS;
            }
            
            if (cur_node.end < endpoint_len) {
                ret = push(&stack, (ParserNode){
                    .token=cur_node.token,
                    .index=cur_node.index,
                    .start=cur_node.start,
                    .end=cur_node.end + 1
                });
                if (ret != SUCCESS) {
                    return ret;
                }
                if (cur_node.index < rule_len - 1) {
                    ret = push(&stack, (ParserNode){
                        .token=tokens[cur_node.index + 1],
                        .index=cur_node.index + 1,
                        .start=cur_node.end,
                        .end=cur_node.end + 1
                    });
                    if (ret != SUCCESS) {
                        return ret;
                    }
                }
            }
        } else if (cur_node.end < endpoint_len && match_still_possible) {
            ret = push(&stack, (ParserNode){
                .token=cur_node.token,
                .index=cur_node.index,
                .start=cur_node.start,
                .end=cur_node.end + 1
            });
            if (ret != SUCCESS) {
                return ret;
            }
        }

        if (stack.size == 0 || *out_match) {
            break;
        } else if (i == MAX_ENDPOINT_MATCHING_ATTEMPTS_PER_RULE - 1) {
            ret = MAX_ENDPOINT_MATCHING_ATTEMPTS_EXCEEDED;
            break;
        }
    }

    if (*out_match) {
        ret = copy_to_out_vars(
            tokens_count,
            tokens,
            token_match_indices,
            vars_count,
            value_len,
            endpoint,
            out_vars,
            out_match
        );
    }

    return ret;
}

typedef enum ParamParserState {
    PARAMPS_NAME,
    PARAMPS_VALUE,
    PARAMPS_ERROR,
    PARAMPS_COMPLETE
} ParamParserState;

char url_decode(char* code) {
    return (char)strtol(code, NULL, 16);
}

int get_param_index(QueryParam params[], size_t params_count, char* name) {
    for (size_t i=0; i<params_count; i++) {
        if (strcmp(params[i].name, name) == 0){
            return i;
        } 
    }
    return -1;
}

CyneResult parse_params(char* url, QueryParam out_params[], size_t* out_count, size_t* out_consumed) {
    size_t param_count = 0;
    QueryParam params[MAX_QUERY_PARAM_COUNT];
    char name[MAX_QUERY_PARAM_NAME_LEN], value[MAX_QUERY_PARAM_VALUE_LEN];
    char* name_start = strchr(url, '?');
    char* name_end;
    char* value_start;
    char* value_end;
    ParamParserState state = PARAMPS_NAME;

    name_start = name_start == NULL ? url : name_start + 1;
    if (name_start == NULL) {
        // TODO: figure out how to detect middle fragment query string
        name_start = url;
    }
    while (state != PARAMPS_ERROR && state != PARAMPS_COMPLETE) {
        switch (state) {
            case PARAMPS_NAME:
            name_end = strchr(name_start, '=');
            if ((name_end - name_start) > MAX_QUERY_PARAM_NAME_LEN) {
                return PARAM_NAME_TOO_LONG;
            }
            for (size_t i=0, j=0; i<(size_t)(name_end - name_start); j++) {
                if (*(name_start + i) == '%') {
                    name[j] = url_decode(name_start + i + 1);
                    i += 3;
                } else {
                    name[j] = *(name_start + i);
                    i++;
                }
            }
            name[name_end - name_start] = 0;
            value_start = name_end + 1;
            state = PARAMPS_VALUE;
            break;

            case PARAMPS_VALUE:
            value_end = strchr(value_start, '&');
            if (value_end == NULL) {
                value_end = strchr(value_start, 0);
            }
            if ((value_end - value_start) > MAX_QUERY_PARAM_VALUE_LEN) {
                return PARAM_VALUE_TOO_LONG;
            }
            for (size_t i=0, j=0; i<(size_t)(value_end - value_start); j++) {
                if (*(value_start + i) == '%') {
                    value[j] = url_decode(value_start + i + 1);
                    i += 3;
                } else {
                    value[j] = *(value_start + i);
                    i++;
                }
            }
            value[value_end - value_start] = 0;

            int param_index = get_param_index(params, param_count, name);
            if (param_index > -1) {
                sprintf(params[param_index].value, "%s,%s", params[param_index].value, value);
            } else {
                sprintf(params[param_count].name, "%s", name);
                sprintf(params[param_count].value, "%s", value);
                param_count++;
            }

            DEBUG("%s = %s", params[param_count-1].name, params[param_count-1].value);

            name_start = *value_end == '&' ? value_end + 1 : value_end;
            state = *name_start == 0 ? PARAMPS_COMPLETE : PARAMPS_NAME;
            break;

            default:
            break;
        }
    }

    for (size_t i=0; i<param_count; i++) {
            sprintf(out_params[i].name, "%s", params[i].name);
            sprintf(out_params[i].value, "%s", params[i].value);

    }
    *out_count = param_count;

    *out_consumed = strlen(url);

    return SUCCESS;
}

CyneResult QueryParamParser_init(QueryParamParser* p, QueryParam params[], size_t params_capacity) {
    p->params = params;
    p->state = PARSE_NAME;
    p->params_capacity = params_capacity;
    p->params_count = 0;
    p->next_param.name[0] = 0;
    p->next_param.value[0] = 0;

    for (size_t i=0; i<params_capacity; i++) {
        if (strlen(params[i].name) > 0) {
            p->params_count++;
        } else {
            break;
        }
    }

    return SUCCESS;
}

CyneResult QueryParamParser_parse(QueryParamParser* p, char* url, size_t* out_count) {
    size_t param_count = 0;
    size_t cur_name_len = 0;
    size_t cur_value_len = 0;
    QueryParam params[MAX_QUERY_PARAM_COUNT - p->params_count];
    char name[MAX_QUERY_PARAM_NAME_LEN], value[MAX_QUERY_PARAM_VALUE_LEN];
    char* name_start = strchr(url, '?');
    char* name_end = NULL;
    char* value_start = url;
    char* value_end = NULL;
    bool done = false;

    if (p->state == PARSE_VALUE && p->next_param.name[0] == 0) {
        // value is split across different chunks
        sprintf(name, "%s", p->params[p->params_count - 1].name);
        sprintf(value, "%s", p->params[p->params_count - 1].value);

    } else {
        sprintf(name, "%s", p->next_param.name);
        sprintf(value, "%s", p->next_param.value);

    }
    for (size_t i=0; i<(MAX_QUERY_PARAM_COUNT - p->params_count); i++) {
        params[i].name[0] = 0;
        params[i].value[0] = 0;
    }

    DEBUG("Parsing %s ...", url);
    while (!done) {
        switch (p->state) {
            case PARSE_NAME:
            if (name_start == NULL) {
                name_start = url;
            } else if (*name_start == '?' || *name_start == '&') {
                name_start++;
            } else if (*name_start == 0) {
                done = true;
                name[0] = 0;
                value[0] = 0;
                break;
            }

            name_end = strchr(name_start, '=');
            if (name_end == NULL) {
                name_end = strchr(name_start, 0);
            }
            
            cur_name_len = strlen(name);
            if ((cur_name_len + 1 + (name_end - name_start)) > MAX_QUERY_PARAM_NAME_LEN) {
                return PARAM_NAME_TOO_LONG;
            }

            for (size_t i=0, j=cur_name_len; i<(size_t)(name_end - name_start); j++) {
                if (*(name_start + i) == '%') {
                    name[j] = url_decode(name_start + i + 1);
                    i += 3;
                } else {
                    name[j] = *(name_start + i);
                    i++;
                }
            }
            name[cur_name_len + name_end - name_start] = 0;
            if (*name_end == 0) {
                done = true;

            } else {
                int existing_param_index = get_param_index(p->params, p->params_count, name);
                if (get_param_index(params, param_count, name) == -1 && existing_param_index > -1) {
                    sprintf(value, "%s,", p->params[existing_param_index].value);
                }

                value_start = name_end + 1;
                p->state = PARSE_VALUE;

            }

            break;

            case PARSE_VALUE:
            if (*value_start == 0) {
                done = true;
                break;
            }

            value_end = strchr(value_start, '&');
            if (value_end == NULL) {
                value_end = strchr(value_start, 0);
            } else if (*value_end == 0) {
                done = true;
                break;
            }

            if (value_end != value_start) {
                cur_value_len = strlen(value);
                if ((cur_value_len + 1 + (value_end - value_start)) > MAX_QUERY_PARAM_VALUE_LEN) {
                    return PARAM_VALUE_TOO_LONG;
                }
                for (size_t i=0, j=cur_value_len; i<(size_t)(value_end - value_start); j++) {
                    if (*(value_start + i) == '%') {
                        value[j] = url_decode(value_start + i + 1);
                        i += 3;
                    } else {
                        value[j] = *(value_start + i);
                        i++;
                    }
                }
                value[cur_value_len + value_end - value_start] = 0;

                int param_index = get_param_index(params, param_count, name);
                if (param_index > -1) {
                    sprintf(params[param_index].value, "%s,%s", params[param_index].value, value);
                } else {
                    sprintf(params[param_count].name, "%s", name);
                    sprintf(params[param_count].value, "%s", value);
                    param_count++;
                    name[0] = 0;
                    value[0] = 0;
                }

                done = *value_end == 0;
                if (done) {
                    p->state = PARSE_VALUE;
                } else {
                    p->state = PARSE_NAME;
                    name_start = value_end + 1;
                }

            } else {
                name_start = url;
                name[0] = 0;
                value[0] = 0;
                p->state = PARSE_NAME;

            }

            break;

        }

    }

    for (size_t i=0; i<param_count; i++) {
        int param_index = get_param_index(p->params, p->params_count, params[i].name); 
        if (param_index > -1) {
            sprintf(p->params[param_index].value, "%s", params[i].value);

        } else {
            sprintf(p->params[p->params_count].name, "%s", params[i].name);
            sprintf(p->params[p->params_count].value, "%s", params[i].value);
            p->params_count++;

        }
    }

    sprintf(p->next_param.name, "%s", name);
    sprintf(p->next_param.value, "%s", value);

    if (out_count != NULL) {
        *out_count = param_count;
    }

    DEBUG("Parsed %zu params.", p->params_count);

    return SUCCESS;

}