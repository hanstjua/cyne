#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../libs/unity/src/unity.h"
#include "../src/json.h"

#define BUFFER_SIZE 1024

#define TEST_ASSERT_EQUAL_OBJ_MEMBER_VIEW_KEY_TYPE_VALUE_MESSAGE(expected_key, expected_type, expected_value, obj_view, index, key_message, type_message, value_message) \
TEST_ASSERT_EQUAL_STRING_LEN_MESSAGE((expected_key), json + (obj_view)->members[(index)]->key.start, (obj_view)->members[(index)]->key.end - (obj_view)->members[(index)]->key.start, (key_message)); \
TEST_ASSERT_EQUAL_MESSAGE((expected_type), (obj_view)->members[(index)]->value->type, (type_message)); \
TEST_ASSERT_EQUAL_STRING_LEN_MESSAGE((expected_value), json + ((JsonNativeView*)((obj_view)->members[(index)]->value))->window.start, ((JsonNativeView*)((obj_view)->members[(index)]->value))->window.end - ((JsonNativeView*)((obj_view)->members[(index)]->value))->window.start, (value_message));

#define TEST_ASSERT_EQUAL_OBJ_MEMBER_VIEW_KEY_TYPE_MESSAGE(expected_key, expected_type, obj_view, index, key_message, type_message) \
TEST_ASSERT_EQUAL_STRING_LEN_MESSAGE((expected_key), json + (obj_view)->members[(index)]->key.start, (obj_view)->members[(index)]->key.end - (obj_view)->members[(index)]->key.start, (key_message)); \
TEST_ASSERT_EQUAL_MESSAGE((expected_type), (obj_view)->members[(index)]->value->type, (type_message)); \

static unsigned char g_test_buffer[BUFFER_SIZE];
static Arena g_test_arena;

void setUp(void) {
    Arena_init(&g_test_arena, g_test_buffer, BUFFER_SIZE);
}

void tearDown(void) {
    memset(g_test_buffer, 0, BUFFER_SIZE);
}

void test_JSTR() {
    JsonValue* result = JSTR(&g_test_arena, "test");

    TEST_ASSERT_NOT_NULL_MESSAGE(result, "Allocation failed.");
    TEST_ASSERT_EQUAL_MESSAGE(JSON_STR, result->type, "Type mismatch.");
    TEST_ASSERT_EQUAL_STRING_MESSAGE("test", result->s, "String mismatch.");
}

void test_JSTR_should_return_NULL_when_there_is_insufficient_space_in_arena() {
    char* test_data[] = {
        "test",
        "this is quite a long string."
    };

    unsigned char buffer[10];
    Arena arena;
    Arena_init(&arena, buffer, 10);

    for (int i=0; i<2; i++) {
        printf("(%d) Data: %s\n", i+1, test_data[i]);
        TEST_ASSERT_NULL_MESSAGE(JSTR(&arena, test_data[i]), "Result not NULL.");
    }
}

void test_JINT() {
    JsonValue* result = JINT(&g_test_arena, 1234);

    TEST_ASSERT_NOT_NULL_MESSAGE(result, "Allocation failed.");
    TEST_ASSERT_EQUAL_MESSAGE(JSON_INT, result->type, "Type mismatch.");
    TEST_ASSERT_EQUAL_MESSAGE(1234, result->i, "Value mismatch.");
}

void test_JINT_should_return_NULL_when_there_is_insufficient_space_in_arena() {
    unsigned char buffer[10];
    Arena arena;
    Arena_init(&arena, buffer, 10);

    TEST_ASSERT_NULL_MESSAGE(JINT(&arena, 1234), "Result not NULL.");
}

void test_JFLT() {
    JsonValue* result = JFLT(&g_test_arena, 12.34);

    TEST_ASSERT_NOT_NULL_MESSAGE(result, "Allocation failed.");
    TEST_ASSERT_EQUAL_MESSAGE(JSON_FLOAT, result->type, "Type mismatch.");
    TEST_ASSERT_FLOAT_WITHIN_MESSAGE(0.00001, 12.34, result->f, "Value mismatch.");
}

void test_JFLT_should_return_NULL_when_there_is_insufficient_space_in_arena() {
    unsigned char buffer[10];
    Arena arena;
    Arena_init(&arena, buffer, 10);

    TEST_ASSERT_NULL_MESSAGE(JFLT(&arena, 12.34), "Result not NULL.");
}

void test_JBOL() {
    JsonValue* result = JBOL(&g_test_arena, true);

    TEST_ASSERT_NOT_NULL_MESSAGE(result, "Allocation failed.");
    TEST_ASSERT_EQUAL_MESSAGE(JSON_TRUE, result->type, "Type mismatch.");
}

void test_JBOL_should_return_NULL_when_there_is_insufficient_space_in_arena() {
    unsigned char buffer[10];
    Arena arena;
    Arena_init(&arena, buffer, 10);

    TEST_ASSERT_NULL_MESSAGE(JFLT(&arena, 12.34), "Result not NULL.");
}

void test_JARR() {
    JsonValue* result = JARR(
        &g_test_arena,
        4,
        JSTR(&g_test_arena, "This is string."),
        JINT(&g_test_arena, 1234),
        JFLT(&g_test_arena, 12.34),
        JBOL(&g_test_arena, true)
    );

    TEST_ASSERT_NOT_NULL_MESSAGE(result, "Allocation failed.");
    TEST_ASSERT_EQUAL_MESSAGE(JSON_ARR, result->type, "Type mismatch.");
    TEST_ASSERT_EQUAL_MESSAGE(4, result->a->size, "Value mismatch.");
    TEST_ASSERT_EQUAL_STRING_MESSAGE("This is string.", result->a->values[0]->s, "String mismatch.");
    TEST_ASSERT_EQUAL_MESSAGE(1234, result->a->values[1]->i, "Integer mismatch.");
    TEST_ASSERT_FLOAT_WITHIN_MESSAGE(0.00001, 12.34, result->a->values[2]->f, "Float mismatch.");
    TEST_ASSERT_EQUAL_MESSAGE(JSON_TRUE, result->a->values[3]->type, "Boolean mismatch.");
}

void test_JARR_should_return_NULL_when_there_is_insufficient_space_in_arena() {
    unsigned char buffer[10];
    Arena arena;
    Arena_init(&arena, buffer, 10);

    TEST_ASSERT_NULL_MESSAGE(JARR(&arena, 2, JINT(&arena, 1234), JFLT(&arena, 12.34)), "Result not NULL.");
}

void test_JARR_should_return_NULL_when_there_is_a_NULL_pointer_in_values() {
    TEST_ASSERT_NULL_MESSAGE(JARR(&g_test_arena, 2, JINT(&g_test_arena, 1234), NULL), "Result not NULL.");
}

void test_JOBJ() {
    int member_count = 6;
    JsonValue* null = JNUL(&g_test_arena); 
    JsonValue* result = JOBJ(&g_test_arena, member_count,
        JMEM(&g_test_arena, "string", JSTR(&g_test_arena, "This is string.")),
        JMEM(&g_test_arena, "integer", JINT(&g_test_arena, 1234)),
        JMEM(&g_test_arena, "float", JFLT(&g_test_arena, 12.34)),
        JMEM(&g_test_arena, "boolean", JBOL(&g_test_arena, true)),
        JMEM(&g_test_arena, "null", null),
        JMEM(&g_test_arena, "array", JARR(&g_test_arena, 2,
            JINT(&g_test_arena, 1234),
            null
            )
        )
    );

    TEST_ASSERT_NOT_NULL_MESSAGE(result, "Allocation failed.");
    TEST_ASSERT_EQUAL_MESSAGE(JSON_OBJ, result->type, "Type mismatch.");
    TEST_ASSERT_EQUAL_MESSAGE(member_count, result->o->size, "Value mismatch.");
    TEST_ASSERT_EQUAL_STRING_MESSAGE("string", result->o->members[0]->key, "Name mismatch.");
    TEST_ASSERT_EQUAL_STRING_MESSAGE("integer", result->o->members[1]->key, "Name mismatch.");
    TEST_ASSERT_EQUAL_STRING_MESSAGE("float", result->o->members[2]->key, "Name mismatch.");
    TEST_ASSERT_EQUAL_STRING_MESSAGE("boolean", result->o->members[3]->key, "Name mismatch.");
    TEST_ASSERT_EQUAL_STRING_MESSAGE("null", result->o->members[4]->key, "Name mismatch.");
    TEST_ASSERT_EQUAL_STRING_MESSAGE("array", result->o->members[5]->key, "Name mismatch.");
    TEST_ASSERT_EQUAL_MESSAGE(JSON_STR, result->o->members[0]->value->type, "Type mismatch.");
    TEST_ASSERT_EQUAL_STRING_MESSAGE("This is string.", result->o->members[0]->value->s, "Value mismatch.");
    TEST_ASSERT_EQUAL_MESSAGE(JSON_INT, result->o->members[1]->value->type, "Type mismatch.");
    TEST_ASSERT_EQUAL_MESSAGE(1234, result->o->members[1]->value->i, "Value mismatch.");
    TEST_ASSERT_EQUAL_MESSAGE(JSON_FLOAT, result->o->members[2]->value->type, "Type mismatch.");
    TEST_ASSERT_FLOAT_WITHIN_MESSAGE(0.00001, 12.34, result->o->members[2]->value->f, "Value mismatch.");
    TEST_ASSERT_EQUAL_MESSAGE(JSON_TRUE, result->o->members[3]->value->type, "Type mismatch.");
    TEST_ASSERT_EQUAL_MESSAGE(JSON_NULL, result->o->members[4]->value->type, "Type mismatch.");
    TEST_ASSERT_EQUAL_MESSAGE(JSON_ARR, result->o->members[5]->value->type, "Type mismatch.");
    TEST_ASSERT_EQUAL_MESSAGE(2, result->o->members[5]->value->a->size, "Array size mismatch.");
}

void test_JOBJ_should_return_NULL_when_there_is_insufficient_space_in_arena() {
    unsigned char buffer[10];
    Arena arena;
    Arena_init(&arena, buffer, 10);

    TEST_ASSERT_NULL_MESSAGE(JOBJ(&arena, 1, JMEM(&arena, "key", JSTR(&arena, "value"))), "Result not NULL.");
}

void test_JOBJ_should_return_NULL_when_there_is_a_NULL_pointer_in_values() {
    TEST_ASSERT_NULL_MESSAGE(JOBJ(&g_test_arena, 1, JMEM(&g_test_arena, "key", NULL)), "Result not NULL.");
}

void test_JsonValue_serialize() {
    JsonValue* null = JNUL(&g_test_arena); 
    JsonValue* data = JOBJ(&g_test_arena, 6,
        JMEM(&g_test_arena, "string", JSTR(&g_test_arena, "This is string.")),
        JMEM(&g_test_arena, "integer", JINT(&g_test_arena, 1234)),
        JMEM(&g_test_arena, "float", JFLT(&g_test_arena, 12.34)),
        JMEM(&g_test_arena, "boolean", JBOL(&g_test_arena, true)),
        JMEM(&g_test_arena, "null", null),
        JMEM(&g_test_arena, "array", JARR(&g_test_arena, 2,
                JINT(&g_test_arena, 1234),
                null
            )
        )
    );
    char* expected = "{\"string\": \"This is string.\", \"integer\": 1234, \"float\": 12.340000, \"boolean\": true, \"null\": null, \"array\": [1234, null]}";
    char actual[256];

    TEST_ASSERT_EQUAL_MESSAGE(SUCCESS, JsonValue_serialize(data, 256, actual), "Result mismatch.");
    TEST_ASSERT_EQUAL_STRING_MESSAGE(expected, actual, "Serialized string mismatch.");
}

void test_JsonValue_serialize_should_return_OVERFLOW_when_buffer_is_too_small() {
    JsonValue* data = JOBJ(&g_test_arena, 1,
        JMEM(&g_test_arena, "string", JSTR(&g_test_arena, "This is string.")),
    );
    char actual[12];

    TEST_ASSERT_EQUAL_MESSAGE(JSON_BUFFER_OVERFLOW, JsonValue_serialize(data, 12, actual), "Result mismatch.");
}

void test_Arena_json_value_view() {
    char* json = "{\"string\": \"This is string.\", \"integer\": 1234, \"float\": 12.34, \"boolean\": true, \"null\": null, \"array\": [\"abcd\", 1234, null], \"object\": {\"inner\": null}}";
    JsonObjectView* result = (JsonObjectView*)Arena_json_value_view(&g_test_arena, json);

    TEST_ASSERT_NOT_NULL_MESSAGE(result, "Result is NULL.");
    TEST_ASSERT_EQUAL_MESSAGE(JSON_OBJ, ((JsonValueView*)result)->type, "Type mismatch.");
    TEST_ASSERT_EQUAL_MESSAGE(7, result->size, "root object size mismatch.");
    TEST_ASSERT_EQUAL_OBJ_MEMBER_VIEW_KEY_TYPE_VALUE_MESSAGE("string", JSON_STR, "This is string.", result, 0, "string key mismatch.", "string type mismatch.", "string value mismatch.");
    TEST_ASSERT_EQUAL_OBJ_MEMBER_VIEW_KEY_TYPE_VALUE_MESSAGE("integer", JSON_INT, "1234", result, 1, "integer key mismatch.", "integer type mismatch.", "integer value mismatch.");
    TEST_ASSERT_EQUAL_OBJ_MEMBER_VIEW_KEY_TYPE_VALUE_MESSAGE("float", JSON_FLOAT, "12.34", result, 2, "float key mismatch.", "float type mismatch.", "float value mismatch.");
    TEST_ASSERT_EQUAL_OBJ_MEMBER_VIEW_KEY_TYPE_MESSAGE("boolean", JSON_TRUE, result, 3, "boolean key mismatch.", "boolean type mismatch.");
    TEST_ASSERT_EQUAL_OBJ_MEMBER_VIEW_KEY_TYPE_MESSAGE("null", JSON_NULL, result, 4, "null key mismatch.", "null type mismatch.");

    JsonArrayView* array = ((JsonArrayView*)(result->members[5]->value));
    TEST_ASSERT_EQUAL_OBJ_MEMBER_VIEW_KEY_TYPE_MESSAGE("array", JSON_ARR, result, 5, "array key mismatch.", "array type mismatch.");
    TEST_ASSERT_EQUAL_MESSAGE(3, array->size, "array size mismatch.");

    JsonNativeView* nat_view = NULL;
    nat_view = ((JsonNativeView*)array->views[0]);
    TEST_ASSERT_EQUAL_MESSAGE(JSON_STR, nat_view->value.type, "array: string type mismatch.");
    TEST_ASSERT_EQUAL_STRING_LEN_MESSAGE("abcd", json + nat_view->window.start, nat_view->window.end - nat_view->window.start, "array: string mismatch.");
    nat_view = ((JsonNativeView*)array->views[1]);
    TEST_ASSERT_EQUAL_MESSAGE(JSON_INT, nat_view->value.type, "array: integer type mismatch.");
    TEST_ASSERT_EQUAL_STRING_LEN_MESSAGE("1234", json + nat_view->window.start, nat_view->window.end - nat_view->window.start, "array: integer mismatch.");
    nat_view = ((JsonNativeView*)array->views[2]);
    TEST_ASSERT_EQUAL_MESSAGE(JSON_NULL, nat_view->value.type, "array: null type mismatch.");

    JsonObjectView* object = ((JsonObjectView*)result->members[6]->value);
    TEST_ASSERT_EQUAL_MESSAGE(JSON_OBJ, ((JsonValueView*)object)->type, "Type mismatch.");
    TEST_ASSERT_EQUAL_MESSAGE(1, object->size, "inner object size mismatch.");
    TEST_ASSERT_EQUAL_OBJ_MEMBER_VIEW_KEY_TYPE_MESSAGE("inner", JSON_NULL, object, 0, "inner key mismatch.", "inner type mismatch.");

}

void test_Arena_json_value_view_should_return_json_value_view_with_no_member_when_input_is_an_empty_object() {
    char* json = "{}";
    JsonObjectView* result = (JsonObjectView*)Arena_json_value_view(&g_test_arena, json);

    TEST_ASSERT_NOT_NULL_MESSAGE(result, "Result is NULL.");
    TEST_ASSERT_EQUAL_MESSAGE(JSON_OBJ, ((JsonValueView*)result)->type, "Type mismatch.");
    TEST_ASSERT_EQUAL_MESSAGE(0, result->size, "root object size mismatch.");

}

void test_Arena_json_value_view_should_return_NULL_when_input_is_invalid_json() {
    size_t SCENARIO_COUNT = 3;
    char* invalid_json[SCENARIO_COUNT];
    invalid_json[0] = "{{\"string\": \"too many opening brace.\"}";
    invalid_json[1] = "{\"string\": \"too many closing brace.\"}}";
    invalid_json[2] = "{\"string\": \"wrong closing brace.\"]";
    char error_message[1024];

    for (size_t i=0; i<SCENARIO_COUNT; i++) {
        JsonObjectView* result = (JsonObjectView*)Arena_json_value_view(&g_test_arena, invalid_json[i]);

        sprintf(error_message, "%s (%s)", "Result is not NULL.", invalid_json[i]);
        TEST_ASSERT_NULL_MESSAGE(result, error_message);
    }

}

int main() {
    UNITY_BEGIN();

    RUN_TEST(test_JSTR);
    RUN_TEST(test_JSTR_should_return_NULL_when_there_is_insufficient_space_in_arena);
    RUN_TEST(test_JINT);
    RUN_TEST(test_JINT_should_return_NULL_when_there_is_insufficient_space_in_arena);
    RUN_TEST(test_JFLT);
    RUN_TEST(test_JFLT_should_return_NULL_when_there_is_insufficient_space_in_arena);
    RUN_TEST(test_JBOL);
    RUN_TEST(test_JBOL_should_return_NULL_when_there_is_insufficient_space_in_arena);
    RUN_TEST(test_JARR);
    RUN_TEST(test_JARR_should_return_NULL_when_there_is_insufficient_space_in_arena);
    RUN_TEST(test_JARR_should_return_NULL_when_there_is_a_NULL_pointer_in_values);
    RUN_TEST(test_JOBJ);
    RUN_TEST(test_JOBJ_should_return_NULL_when_there_is_insufficient_space_in_arena);
    RUN_TEST(test_JOBJ_should_return_NULL_when_there_is_a_NULL_pointer_in_values);

    RUN_TEST(test_JsonValue_serialize);
    RUN_TEST(test_JsonValue_serialize_should_return_OVERFLOW_when_buffer_is_too_small);

    RUN_TEST(test_Arena_json_value_view);
    RUN_TEST(test_Arena_json_value_view_should_return_json_value_view_with_no_member_when_input_is_an_empty_object);
    RUN_TEST(test_Arena_json_value_view_should_return_NULL_when_input_is_invalid_json);

    return UNITY_END();
}