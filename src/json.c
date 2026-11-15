#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "configs.h"
#include "json.h"
#include "logger.h"

#define MAX_PARSER_VALUE_VIEW_STACK_SIZE JSON_PARSER_MAX_VALUE_VIEW_MEMORY / sizeof(JsonValueView*)

typedef enum JsonParserMode {
    JSON_PARSER_ARR_ITEM,
    JSON_PARSER_ARR_ITEM_END,
    JSON_PARSER_ARR_END,
    JSON_PARSER_OBJ_MEMBER,
    JSON_PARSER_OBJ_MEMBER_END,
    JSON_PARSER_OBJ_END,
} JsonParserMode;

typedef struct JsonParserState {
    JsonParserMode mode;
    JsonValueWindow key;
    uint32_t index;
    uint32_t json_pos;
} JsonParserState;

typedef struct StateStack {
    JsonParserState buffer[MAX_JSON_DEPTH];
    uint32_t size;
} StateStack;

void StateStack_push(StateStack* s, JsonParserState state) {
    if (s->size >= MAX_JSON_DEPTH) {
        // TODO: handle overflow
    }
    s->buffer[s->size++] = state;
}

JsonParserState StateStack_pop(StateStack* s) {
    if (s->size == 0) {
        // TODO: handle empty stack
    }
    return s->buffer[--s->size];
}

typedef union ViewStackItem {
    JsonValueView* v;
    JsonObjectMemberView* m;
} ViewStackItem;

typedef struct ViewStack {
    ViewStackItem buffer[MAX_PARSER_VALUE_VIEW_STACK_SIZE];
    uint32_t size;
} ViewStack;

void ViewStack_push_value(ViewStack* s, JsonValueView* v) {
    if (s->size >= MAX_PARSER_VALUE_VIEW_STACK_SIZE) {
        // TODO: handle overflow
    }
    s->buffer[s->size++].v = v;
}

void ViewStack_push_member(ViewStack* s, JsonObjectMemberView* m) {
    if (s->size >= MAX_PARSER_VALUE_VIEW_STACK_SIZE) {
        // TODO: handle overflow
    }
    s->buffer[s->size++].m = m;
}

ViewStackItem ViewStack_pop(ViewStack* s) {
    if (s->size == 0) {
        // TODO: handle empty stack
    }
    return s->buffer[--s->size];
}

JsonValue* Arena_json_str(Arena* a, char* str) {
    uint32_t str_size = strlen(str) + 1;
    if (str_size + a->offset >= a->size) {
        return NULL;
    }

    char* value_ptr = (char*)a->buffer + a->offset; 
    sprintf(value_ptr, "%s", str);
    a->offset += strlen(str) + 1;

    size_t aligned_offset;
    if (Arena_align_up(a, sizeof(void*), &aligned_offset) != SUCCESS) {
        return NULL;
    }

    uint32_t new_offset = aligned_offset + sizeof(void*) + sizeof(JsonValueType);
    if (new_offset >= a->size) {
        return NULL;
    }

    JsonValue* j = (void*)(a->buffer + aligned_offset);
    j->type = JSON_STR;
    j->s = value_ptr;

    a->offset = new_offset;

    return j;
}

JsonValue* Arena_json_int(Arena* a, long i) {
    size_t aligned_offset;
    if (Arena_align_up(a, sizeof(void*), &aligned_offset) != SUCCESS) {
        return NULL;
    }

    uint32_t new_offset = aligned_offset + sizeof(void*) + sizeof(JsonValueType);
    if (new_offset >= a->size) {
        return NULL;
    }

    JsonValue* j = (void*)(a->buffer + aligned_offset);
    j->type = JSON_INT;
    j->i = i;

    a->offset = new_offset;

    return j;
}

JsonValue* Arena_json_float(Arena* a, double f) {
    size_t aligned_offset;
    if (Arena_align_up(a, sizeof(void*), &aligned_offset) != SUCCESS) {
        return NULL;
    }

    uint32_t new_offset = aligned_offset + sizeof(void*) + sizeof(JsonValueType);
    if (new_offset >= a->size) {
        return NULL;
    }

    JsonValue* j = (void*)(a->buffer + aligned_offset);
    j->type = JSON_FLOAT;
    j->f = f;

    a->offset = new_offset;

    return j;
}

JsonValue* Arena_json_bool(Arena* a, bool b) {
    size_t aligned_offset;
    if (Arena_align_up(a, sizeof(void*), &aligned_offset) != SUCCESS) {
        return NULL;
    }

    uint32_t new_offset = aligned_offset + sizeof(void*) + sizeof(JsonValueType);
    if (new_offset >= a->size) {
        return NULL;
    }

    JsonValue* j = (void*)(a->buffer + aligned_offset);
    j->type = b ? JSON_TRUE : JSON_FALSE;

    a->offset = new_offset;

    return j;
}

JsonValue* Arena_json_null(Arena* a) {
    size_t aligned_offset;
    if (Arena_align_up(a, sizeof(void*), &aligned_offset) != SUCCESS) {
        return NULL;
    }

    uint32_t new_offset = aligned_offset + sizeof(void*) + sizeof(JsonValueType);
    if (new_offset >= a->size) {
        return NULL;
    }

    JsonValue* j = (void*)(a->buffer + aligned_offset);
    j->type = JSON_NULL;

    a->offset = new_offset;

    return j;

}

JsonValue* Arena_json_arr(Arena* a, JsonValue* values[], uint32_t size) {
    size_t aligned_offset;
    if (Arena_align_up(a, sizeof(void*), &aligned_offset) != SUCCESS) {
        return NULL;
    }

    uint32_t json_value_offset = aligned_offset + sizeof(JsonValue);
    uint32_t json_array_offset = json_value_offset + sizeof(JsonArray);
    uint32_t values_offset = json_array_offset + sizeof(JsonValue*) * size;
    if (values_offset >= a->size || json_array_offset >= a->size || json_value_offset >= a->size) {
        return NULL;
    }

    JsonValue* j = (void*)(a->buffer + aligned_offset);
    j->type = JSON_ARR;
    j->a = (void*)(a->buffer + json_value_offset);

    JsonArray* jarr = j->a;
    jarr->size = size;
    jarr->values = (JsonValue**)(a->buffer + json_array_offset);

    for (uint32_t i=0; i<size; i++) {
        if (values[i] == NULL) {
            return NULL;
        }
        *((JsonValue**)(a->buffer + json_array_offset) + i) = values[i];
    }

    a->offset = values_offset;

    return j;
}

JsonValue* Arena_json_obj(Arena* a, JsonObjectMember* members[], uint32_t size) {
    size_t aligned_offset;
    if (Arena_align_up(a, sizeof(void*), &aligned_offset) != SUCCESS) {
        return NULL;
    }

    uint32_t json_value_offset = aligned_offset + sizeof(JsonValue);
    uint32_t json_obj_offset = json_value_offset + sizeof(JsonObject);
    uint32_t values_offset = json_obj_offset + sizeof(JsonValue*) * size;
    if (values_offset >= a->size|| json_obj_offset >= a->size || json_value_offset >= a->size) {
        return NULL;
    }

    JsonValue* j = (void*)(a->buffer + aligned_offset);
    j->type = JSON_OBJ;
    j->o = (void*)(a->buffer + json_value_offset);

    JsonObject* jobj = j->o;
    jobj->size = size;
    jobj->members = (JsonObjectMember**)(a->buffer + json_obj_offset);

    for (uint32_t i=0; i<size; i++) {
        if (members[i] == NULL) {
            return NULL;
        }
        *((JsonObjectMember**)(a->buffer + json_obj_offset) + i) = members[i];
    }

    a->offset = values_offset;

    return j;
}

JsonObjectMember* Arena_json_obj_member(Arena* a, char* key, JsonValue* value) {
    uint32_t key_len = strlen(key) + 1;
    if (a->offset + key_len >= a->size) {
        return NULL;
    }
    sprintf((char*)(a->buffer + a->offset), "%s", key);
    char* key_ptr = (char*)a->buffer + a->offset;
    a->offset += key_len;

    size_t aligned_offset;
    if (Arena_align_up(a, sizeof(void*), &aligned_offset) != SUCCESS || value == NULL) {
        return NULL;
    }

    uint32_t json_member_offset = aligned_offset + sizeof(JsonObjectMember);
    if (json_member_offset >= a->size) {
        return NULL;
    }

    JsonObjectMember* j = (void*)(a->buffer + aligned_offset);
    j->key = key_ptr;
    j->value = value;

    a->offset = json_member_offset;

    return j;
}

typedef struct JsonSerializerStackItem {
    const JsonValue* value;
    uint32_t index;
} JsonSerializerStackItem;

CyneResult JsonValue_serialize(const JsonValue* j, uint32_t buffer_size, char* out_buffer) {
    JsonSerializerStackItem stack[MAX_JSON_DEPTH], current_item;
    uint32_t stack_size = 0, current_item_size = 0;
    CyneResult result = SUCCESS;
    char* buffer_start = out_buffer;
    char temp[1];

    stack[0].value = j;
    stack[0].index = 0;
    stack_size = 1;

    while (result == SUCCESS && stack_size > 0) {
        current_item = stack[stack_size-- - 1];

        switch (current_item.value->type) {
            case JSON_STR:
            if (buffer_size - (uint32_t)(out_buffer - buffer_start) < (strlen(current_item.value->s) + 3)) {
                return JSON_BUFFER_OVERFLOW;
            }
            out_buffer += sprintf(out_buffer, "\"%s\"", current_item.value->s);
            break;
            
            case JSON_INT:
            if (buffer_size - (out_buffer - buffer_start) < (snprintf(temp, 1, "%ld", current_item.value->i) + 1)) {
                return JSON_BUFFER_OVERFLOW;
            }
            out_buffer += sprintf(out_buffer, "%ld", current_item.value->i);
            break;
            
            case JSON_FLOAT:
            if (buffer_size - (out_buffer - buffer_start) < (snprintf(temp, 1, "%lf", current_item.value->f) + 1)) {
                return JSON_BUFFER_OVERFLOW;
            }
            out_buffer += sprintf(out_buffer, "%lf", current_item.value->f);
            break;
            
            case JSON_TRUE:
            if (buffer_size - (out_buffer - buffer_start) < 5) {
                return JSON_BUFFER_OVERFLOW;
            }
            out_buffer += sprintf(out_buffer, "%s", "true");
            break;

            case JSON_FALSE:
            if (buffer_size - (out_buffer - buffer_start) < 6) {
                return JSON_BUFFER_OVERFLOW;
            }
            out_buffer += sprintf(out_buffer, "%s", "false");
            break;

            case JSON_NULL:
            if (buffer_size - (out_buffer - buffer_start) < 5) {
                return JSON_BUFFER_OVERFLOW;
            }
            out_buffer += sprintf(out_buffer, "%s", "null");
            break;
            
            default:
            current_item_size = current_item.value->type == JSON_OBJ ? current_item.value->o->size : current_item.value->a->size;

            if (current_item.index == current_item_size) {
                if (buffer_size - (out_buffer - buffer_start) < 2) {
                    return JSON_BUFFER_OVERFLOW;
                }
                out_buffer += sprintf(out_buffer, "%s", current_item.value->type == JSON_OBJ ? "}" : "]");
            } else {
                if (current_item.index == 0) {
                    if (buffer_size - (out_buffer - buffer_start) < 2) {
                        return JSON_BUFFER_OVERFLOW;
                    }
                    out_buffer += sprintf(out_buffer, "%s", current_item.value->type == JSON_OBJ ? "{" : "[");
                } else {
                    if (buffer_size - (out_buffer - buffer_start) < 3) {
                        return JSON_BUFFER_OVERFLOW;
                    }
                    out_buffer += sprintf(out_buffer, "%s", ", ");
                }

                if (current_item.index <= current_item_size - 1) {
                    stack[stack_size].value = current_item.value;
                    stack[stack_size].index = current_item.index + 1;
                    stack_size++;

                    if (current_item.value->type == JSON_OBJ) {
                        if (buffer_size - (out_buffer - buffer_start) < (snprintf(temp, 1, "\"%s\": ", current_item.value->o->members[current_item.index]->key) + 1)) {
                            return JSON_BUFFER_OVERFLOW;
                        }
                        out_buffer += sprintf(out_buffer, "\"%s\": ", current_item.value->o->members[current_item.index]->key);
                        stack[stack_size].value = current_item.value->o->members[current_item.index]->value;
                    } else {
                        stack[stack_size].value = current_item.value->a->values[current_item.index];
                    }
                    stack[stack_size].index = 0;
                    stack_size++;

                }
            }
        }
    }

    return SUCCESS;
}

uint32_t get_json_str_end(const char* json, uint32_t str_start) {
    uint32_t str_end = -1;
    const char* cursor = json + str_start;
    while (str_end == (uint32_t)-1) {
        str_end = strchr(cursor, '"') - json;
    }

    return str_end;
}

JsonValueView* Arena_json_value_view_str(Arena* a, const char* json, uint32_t str_start) {
    uint32_t str_end = get_json_str_end(json, str_start);

    size_t aligned_offset;
    if (Arena_align_up(a, sizeof(void*), &aligned_offset) != SUCCESS) {
        return NULL;
    }

    uint32_t new_offset = aligned_offset + sizeof(JsonValueView) + sizeof(JsonValueWindow);
    if (new_offset >= a->size) {
        return NULL;
    }

    JsonNativeView* j = (void*)(a->buffer + aligned_offset);
    j->value.type = JSON_STR;
    j->window.start = str_start;
    j->window.end = str_end;

    a->offset = new_offset;

    return (JsonValueView*)j;
}

JsonValueView* Arena_json_value_view_int(Arena* a, const char* json, uint32_t int_start) {
    size_t aligned_offset;
    if (Arena_align_up(a, sizeof(void*), &aligned_offset) != SUCCESS) {
        return NULL;
    }

    uint32_t new_offset = aligned_offset + sizeof(JsonValueView) + sizeof(JsonValueWindow);
    if (new_offset >= a->size) {
        return NULL;
    }

    JsonNativeView* j = (void*)(a->buffer + aligned_offset);
    j->value.type = JSON_INT;
    j->window.start = int_start;
    j->window.end = strpbrk(json + int_start, ",]} \n\t\v") - json;

    a->offset = new_offset;

    return (JsonValueView*)j;
}

JsonValueView* Arena_json_value_view_float(Arena* a, const char* json, uint32_t float_start) {
    size_t aligned_offset;
    if (Arena_align_up(a, sizeof(void*), &aligned_offset) != SUCCESS) {
        return NULL;
    }

    uint32_t new_offset = aligned_offset + sizeof(JsonValueView) + sizeof(JsonValueWindow);
    if (new_offset >= a->size) {
        return NULL;
    }

    JsonNativeView* j = (void*)(a->buffer + aligned_offset);
    j->value.type = JSON_FLOAT;
    j->window.start = float_start;
    j->window.end = strpbrk(json + float_start, ",]} \n\t\v") - json;

    a->offset = new_offset;

    return (JsonValueView*)j;
}

JsonValueView* Arena_json_value_view_singleton(Arena* a, JsonValueType type) {
    size_t aligned_offset;
    if (Arena_align_up(a, sizeof(void*), &aligned_offset) != SUCCESS) {
        return NULL;
    }

    uint32_t new_offset = aligned_offset + sizeof(JsonValueView);
    if (new_offset >= a->size) {
        return NULL;
    }

    JsonValueView* j = (void*)(a->buffer + aligned_offset);
    j->type = type;

    a->offset = new_offset;

    return j;

}

JsonValueView* Arena_json_value_view_bool(Arena* a, bool value) {
    return Arena_json_value_view_singleton(a, value ? JSON_TRUE : JSON_FALSE);
}

JsonValueView* Arena_json_value_view_null(Arena* a) {
    return Arena_json_value_view_singleton(a, JSON_NULL);
}

JsonValueView* Arena_json_value_view_parse_value(Arena* a, const char* json, uint32_t start_index, JsonValueView* null, JsonValueView* true_, JsonValueView* false_) {
    uint32_t value_size = strpbrk(json + start_index, ",]} \n\t\v") - json - start_index;
    if (isdigit(json[start_index])) {
        for (uint32_t i=start_index; i < start_index + value_size; i++) {
            if (json[i] == '.' || json[i] == 'E' || json[i] == 'e') {
                return Arena_json_value_view_float(a, json, start_index);
            }
        }
        return Arena_json_value_view_int(a, json, start_index); 

    } else if (strncmp(json + start_index, "true", value_size) == 0) {
        return true_;
        
    } else if (strncmp(json + start_index, "false", value_size) == 0) {
        return false_;

    } else if (strncmp(json + start_index, "null", value_size) == 0) {
        return null;

    } else {
        ERROR("Unrecognised JSON value starting with '%c'.", json[start_index]);
        return NULL;
    }

}

JsonValueView* Arena_json_array_view(Arena* a, JsonValueView* values[], uint32_t size) {
    size_t aligned_offset;
    if (Arena_align_up(a, sizeof(void*), &aligned_offset) != SUCCESS) {
        return NULL;
    }

    uint32_t new_offset = aligned_offset + sizeof(JsonArrayView) + (sizeof(JsonValueView*) * size);
    if (new_offset >= a->size) {
        return NULL;
    }

    JsonArrayView* j = (void*)(a->buffer + aligned_offset);
    j->size = size;
    j->value.type = JSON_ARR;
    j->views = (JsonValueView**)a->buffer + aligned_offset + sizeof(JsonArrayView);

    JsonValueView** values_buffer = j->views; 
    for (uint32_t i=0; i<size; i++) {
        values_buffer[i] = values[i];
    }

    a->offset = new_offset;

    return (JsonValueView*)j;

}

JsonObjectMemberView* Arena_json_object_member_view(Arena* a, JsonValueWindow key, JsonValueView* value) {
    size_t aligned_offset;
    if (Arena_align_up(a, sizeof(void*), &aligned_offset) != SUCCESS) {
        return NULL;
    }

    uint32_t new_offset = aligned_offset + sizeof(JsonValueView*) + sizeof(JsonValueWindow);
    if (new_offset >= a->size) {
        return NULL;
    }

    JsonObjectMemberView* j = (void*)(a->buffer + aligned_offset);
    j->key = key;
    j->value = value;

    a->offset = new_offset;

    return (JsonObjectMemberView*)j;

}

JsonValueView* Arena_json_object_view(Arena* a, JsonObjectMemberView* members[], uint32_t size) {
    size_t aligned_offset;
    if (Arena_align_up(a, sizeof(void*), &aligned_offset) != SUCCESS) {
        return NULL;
    }

    uint32_t new_offset = aligned_offset + sizeof(JsonObjectView) + (sizeof(JsonObjectMemberView*) * size);
    if (new_offset >= a->size) {
        return NULL;
    }

    JsonObjectView* j = (void*)(a->buffer + aligned_offset);
    j->size = size;
    j->value.type = JSON_OBJ;
    j->members = size <= 0 ? NULL : (JsonObjectMemberView**)a->buffer + aligned_offset + sizeof(JsonObjectView);

    JsonObjectMemberView** members_buffer = j->members;
    for (uint32_t i=0; i<size; i++) {
        members_buffer[i] = members[i];
    }

    a->offset = new_offset;

    return (JsonValueView*)j;
}

uint32_t advance_cursor_after_value(JsonValueView* view, uint32_t cursor, const char* json) {
    switch (view->type) {
        case JSON_TRUE:
        case JSON_NULL:
        cursor += 4 + strspn(json + cursor + 4, " \n\t\v");
        break;

        case JSON_FALSE:
        cursor += 5 + strspn(json + cursor + 4, " \n\t\v");
        break;

        case JSON_STR:
        cursor = ((JsonNativeView*)view)->window.end + 1 + strspn(json + ((JsonNativeView*)view)->window.end + 1, " \n\t\v");  // skip whitespaces
        break;

        default:
        cursor = ((JsonNativeView*)view)->window.end + strspn(json + ((JsonNativeView*)view)->window.end, " \n\t\v");  // skip whitespaces
    }

    return cursor;
}

uint32_t handle_nested_item(
    StateStack* state_stack,
    ViewStack* view_stack,
    uint32_t current_cursor,
    const char* json,
    JsonParserState parser_state,
    Arena* arena,
    JsonValueView* json_true,
    JsonValueView* json_false,
    JsonValueView* json_null,
    bool is_obj_member
) {
    uint32_t cursor = current_cursor;
    JsonValueView* view = NULL;
    JsonValueWindow key = {.start=0, .end=0};
    JsonObjectMemberView* member = NULL;

    if (view_stack->size == MAX_PARSER_VALUE_VIEW_STACK_SIZE) {
        // TODO: handle parser stack overflow
    }

    if (is_obj_member) {
        cursor += strspn(json + cursor, " \n\t\v");  // skip whitespaces
        if (json[cursor] != '"') {
            // TODO: handle invalid key
        }
        key.start = ++cursor;
        key.end = get_json_str_end(json, cursor);

        cursor = key.end + 1;
        cursor += strspn(json + cursor, " \n\t\v");  // skip whitespaces
        if (json[cursor] != ':') {
            // TODO: handle : not found
        }

        cursor++;

    }

    cursor += strspn(json + cursor, " \n\t\v");  // skip whitespaces
    switch (json[cursor]) {
        case '[':
        case '{':
        StateStack_push(state_stack, (JsonParserState){.index=parser_state.index, .key=key, .json_pos=-1, .mode=is_obj_member ? JSON_PARSER_OBJ_MEMBER_END : JSON_PARSER_ARR_ITEM_END});  // json_pos to be set by the last array-parsing state
        StateStack_push(state_stack, (JsonParserState){.index=0, .key=key, .json_pos=cursor+1, .mode=json[cursor] == '{' ? JSON_PARSER_OBJ_MEMBER : JSON_PARSER_ARR_ITEM});
        break;

        default: 
        cursor += strspn(json + cursor, " \n\t\v");  // skip whitespaces
        view = json[cursor] == '"' ? Arena_json_value_view_str(arena, json, cursor + 1) : Arena_json_value_view_parse_value(arena, json, cursor, json_null, json_true, json_false);
        if (view == NULL) {
            ERROR("Unable to allocate for JSON value view in arena.");
            return (uint32_t)-1;

        }

        cursor = advance_cursor_after_value(view, cursor, json);
        cursor += strspn(json + cursor, " \n\t\v");  // skip whitespaces
        if (json[cursor] != ',' && json[cursor] != ']' && json[cursor] != '}') {
            ERROR("Invalid JSON syntax after JSON value.");
            return (uint32_t)-1;

        } else {
            if (!is_obj_member) {
                DEBUG("end of array %c", json[cursor]);
                if (json[cursor] != ',' && json[cursor] != ']') {
                    ERROR("Unexpected character '%c' in array.", json[cursor]);
                    return (uint32_t)-1;
                }

                ViewStack_push_value(view_stack, view);
                StateStack_push(state_stack, (JsonParserState){
                    .index = json[cursor] == ',' ? parser_state.index + 1 : parser_state.index,
                    .json_pos = cursor + 1,
                    .mode = json[cursor] == ',' ? JSON_PARSER_ARR_ITEM : JSON_PARSER_ARR_END
                });

            } else {
                DEBUG("end of object %c", json[cursor]);
                if (json[cursor] != ',' && json[cursor] != '}') {
                    ERROR("Unexpected character '%c' in object.", json[cursor]);
                    return (uint32_t)-1;
                }

                member = Arena_json_object_member_view(arena, key, view);
                if (member == NULL) {
                    ERROR("Unable to allocate for object member view in arena.");
                    return (uint32_t)-1;

                }
                ViewStack_push_member(view_stack, member);
                StateStack_push(state_stack, (JsonParserState){
                    .index = json[cursor] == ',' ? parser_state.index + 1 : parser_state.index,
                    .json_pos = cursor + 1,
                    .mode = json[cursor] == ',' ? JSON_PARSER_OBJ_MEMBER : JSON_PARSER_OBJ_END
                });
            }
        }
        break;
    }
    return cursor;
}

uint32_t handle_nested_item_end(StateStack* state_stack, ViewStack* view_stack, Arena* arena, uint32_t cursor, const char* json, JsonParserState parser_state, bool is_obj_member) {
    JsonValueView* view = NULL;
    JsonObjectMemberView* member = NULL;

    cursor += strspn(json + cursor, " \n\t\v");  // skip whitespaces
    if (json[cursor] != ',' && json[cursor] != ']' && json[cursor] != '}') {
        ERROR("Invalid JSON syntax after nested item.");
        return (uint32_t)-1;

    } else {
        if (!is_obj_member) {
            StateStack_push(state_stack, (JsonParserState){
                .index = json[cursor] == ',' ? parser_state.index + 1 : parser_state.index,
                .json_pos = cursor + 1,
                .mode = json[cursor] == ',' ? JSON_PARSER_ARR_ITEM : JSON_PARSER_ARR_END
            });

        } else {
            view = ViewStack_pop(view_stack).v;
            if (view == NULL) {
                ERROR("Unable to allocate JSON value view in arena.");
                return (uint32_t)-1;

            }
            member = Arena_json_object_member_view(arena, parser_state.key, view);
            if (member == NULL) {
                ERROR("Unable to allocate object member view in arena.");
                return (uint32_t)-1;

            }
            ViewStack_push_member(view_stack, member);
            StateStack_push(state_stack, (JsonParserState){
                .index = json[cursor] == ',' ? parser_state.index + 1 : parser_state.index,
                .json_pos = cursor + 1,
                .mode = json[cursor] == ',' ? JSON_PARSER_OBJ_MEMBER : JSON_PARSER_OBJ_END
            });

        }
    }

    return cursor;

}

uint8_t handle_item_end(StateStack* state_stack, ViewStack* view_stack, Arena* arena, JsonParserState parser_state, bool is_obj) {
    JsonValueView* view = is_obj ? \
    Arena_json_object_view(arena, (JsonObjectMemberView**)view_stack->buffer + view_stack->size - 1 - parser_state.index, parser_state.index + 1) : \
    Arena_json_array_view(arena, (JsonValueView**)view_stack->buffer + view_stack->size - 1 - parser_state.index, parser_state.index + 1);

    if (view == NULL) {
        ERROR("Unable to allocate object or array view in arena.");
        return (uint8_t)-1;

    }
    view_stack->size -= parser_state.index + 1;
    ViewStack_push_value(view_stack, view);
    
    if (state_stack->size > 0 && (state_stack->buffer[state_stack->size-1].mode == JSON_PARSER_ARR_ITEM_END || state_stack->buffer[state_stack->size-1].mode == JSON_PARSER_OBJ_MEMBER_END)) {
        state_stack->buffer[state_stack->size-1].json_pos = parser_state.json_pos;
    }

    return 0;
}

JsonValueView* Arena_json_value_view(Arena* a, const char* json) {
    if (strcmp("{}", json) == 0) {
        return (JsonValueView*)Arena_json_object_view(a, NULL, 0);
    }
    ViewStack view_stack;
    view_stack.size = 0;

    StateStack state_stack;
    state_stack.size = 0;

    JsonParserState current_state;

    JsonValueView* json_null = Arena_json_value_view_null(a);
    JsonValueView* json_true = Arena_json_value_view_bool(a, true);
    JsonValueView* json_false = Arena_json_value_view_bool(a, false);

    
    uint32_t cursor = strspn(json, "\n\t\v ");
    switch (json[cursor]) {
        case '"':
        return Arena_json_value_view_str(a, json, cursor + 1);
        case '[':
        case '{':
        StateStack_push(&state_stack, (JsonParserState){.index=0, .json_pos=cursor+1, .mode=json[cursor] == '[' ? JSON_PARSER_ARR_ITEM : JSON_PARSER_OBJ_MEMBER});
        break;

        default: 
        return Arena_json_value_view_parse_value(a, json, cursor, json_null, json_true, json_false);
    }

    while (state_stack.size > 0) {
        current_state = StateStack_pop(&state_stack);
        cursor = current_state.json_pos;
        switch (current_state.mode) {
            case JSON_PARSER_ARR_ITEM:
            cursor = handle_nested_item(&state_stack, &view_stack, cursor, json, current_state, a, json_true, json_false, json_null, false);
            break;

            case JSON_PARSER_ARR_ITEM_END:
            cursor = handle_nested_item_end(&state_stack, &view_stack, a, cursor, json, current_state, false);
            break;

            case JSON_PARSER_ARR_END:
            if (handle_item_end(&state_stack, &view_stack, a, current_state, false) != 0) {
                cursor = (uint32_t)-1;

            }
            break;

            case JSON_PARSER_OBJ_MEMBER:
            cursor = handle_nested_item(&state_stack, &view_stack, cursor, json, current_state, a, json_true, json_false, json_null, true);
            break;

            case JSON_PARSER_OBJ_MEMBER_END:
            cursor = handle_nested_item_end(&state_stack, &view_stack, a, cursor, json, current_state, true);
            break;

            case JSON_PARSER_OBJ_END:
            if (handle_item_end(&state_stack, &view_stack, a, current_state, true) != 0) {
                cursor = (uint32_t)-1;

            }
            break;
        }

        if (cursor == (uint32_t)-1) {
            ERROR("Failed to parse JSON.");
            return NULL;
        } else if (state_stack.size == 0 && *(json + cursor + strspn(json + cursor, "\n\t\v")) != 0) {
            ERROR("Unexpected character '%c' after end of JSON.", *(json + cursor + strspn(json + cursor, "\n\t\v")));
            return NULL;
        } 
    }

    return ViewStack_pop(&view_stack).v;
}
