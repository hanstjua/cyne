#ifndef CYNE_JSON_H
#define CYNE_JSON_H

#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>

#include "arena.h"
#include "error.h"

typedef enum {
    JSON_STR,
    JSON_INT,
    JSON_FLOAT,
    JSON_TRUE,
    JSON_FALSE,
    JSON_OBJ,
    JSON_ARR,
    JSON_NULL
} JsonValueType;

typedef struct JsonValue JsonValue;

typedef struct JsonArray {
    JsonValue** values;
    uint32_t size;
} JsonArray;

typedef struct JsonObjectMember {
    char* key;
    JsonValue* value;
} JsonObjectMember;

typedef struct JsonObject {
    JsonObjectMember** members;
    uint32_t size;
} JsonObject;

typedef struct JsonValue {
    union {
        char* s;
        long i;
        double f;
        JsonArray* a;
        JsonObject* o;
    };
    JsonValueType type;
} JsonValue;

typedef struct JsonValueWindow {
    uint32_t start;
    uint32_t end;
} JsonValueWindow;

typedef struct JsonValueView JsonValueView;

typedef struct JsonValueView {
    JsonValueType type;
} JsonValueView;

typedef struct JsonNativeView {
    JsonValueView value;
    JsonValueWindow window;
} JsonNativeView;

typedef struct JsonArrayView {
    JsonValueView value;
    uint32_t size;
    JsonValueView** views;
} JsonArrayView;

typedef struct JsonObjectMemberView {
    JsonValueView* value;
    JsonValueWindow key;
} JsonObjectMemberView;

typedef struct JsonObjectView {
    JsonValueView value;
    uint32_t size;
    JsonObjectMemberView** members;
} JsonObjectView;

typedef struct JsonViewLookup JsonViewLookup;

typedef struct JsonViewLookupItem {
    union {
        JsonValueView* v;
        JsonViewLookup* l;
    };
    JsonValueWindow key;
    JsonValueType type;
} JsonViewLookupItem;

typedef struct JsonViewLookup {
    uint32_t size;
    JsonViewLookupItem* items;    
} JsonViewLookup;

typedef struct JsonViewLookupResult {
    union {
        JsonViewLookup* l;
        long i;
        double f;
        JsonValueWindow s;
    };
    JsonValueType type;
} JsonViewLookupResult;

#define JSTR(a, str) Arena_json_str(a, str)
JsonValue* Arena_json_str(Arena* a, char* str);

#define JINT(a, i) Arena_json_int(a, i)
JsonValue* Arena_json_int(Arena* a, long i);

#define JFLT(a, f) Arena_json_float(a, f)
JsonValue* Arena_json_float(Arena* a, double f);

#define JBOL(a, b) Arena_json_bool(a, b)
JsonValue* Arena_json_bool(Arena* a, bool b);

#define JNUL(a) Arena_json_null(a)
JsonValue* Arena_json_null(Arena* a);

#define JARR(a, size, ...) Arena_json_arr(a, (JsonValue*[]){ __VA_ARGS__ }, size)
JsonValue* Arena_json_arr(Arena* a, JsonValue* values[], uint32_t size);

#define JOBJ(a, size, ...) Arena_json_obj(a, (JsonObjectMember*[]){ __VA_ARGS__ }, size)
JsonValue* Arena_json_obj(Arena* a, JsonObjectMember* members[], uint32_t size);

#define JMEM(a, k, v) Arena_json_obj_member(a, k, v)
JsonObjectMember* Arena_json_obj_member(Arena* a, char* key, JsonValue* value);

CyneResult JsonValue_serialize(const JsonValue* j, uint32_t buffer_size, char* out_buffer);

JsonValueView* Arena_json_value_view(Arena* a, const char* json);
// TODO: CyneResult JsonObjectView_get(JsonObjectView* o, const char* key, JsonValueView* out_value);
// TODO: CyneResult JsonArrayView_get(JsonArrayView* a, size_t index, JsonValueView* out_value);
// TODO: CyneResult JsonValueWindow_str(JsonValueWindow w, const char* json, char* out_str);
// TODO: CyneResult JsonValueWindow_int(JsonValueWindow w, const char* json, long* out_int);
// TODO: CyneResult JsonValueWindow_float(JsonValueWindow w, const char* json, double* out_float);
// TODO: CyneResult JsonValueWindow_bool(JsonValueWindow w, const char* json, bool* out_bool);
// TODO: JsonViewLookup* Arena_json_view_lookup(Arena* a, JsonValueView* view);
// TODO: CyneResult JsonViewLookup_arr_get(const JsonViewLookup* j, const char* json, uint32_t index, JsonViewLookupResult* out_result);
// TODO: CyneResult JsonViewLookup_obj_get(const JsonViewLookup* j, const char* json, const char* key, JsonViewLookupResult* out_result);

/* End of header */
#endif