#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../libs/unity/src/unity.h"

#include "../src/error.h"
#include "../src/request_context.h"

void setUp(void) {}
void tearDown(void) {}

void test_RequestContext_get_params_value() {
    // SETUP
    RequestContext r = {
        .params={
            { .name="param1", .value="value1" },
            { .name="param2", .value="value2" }
        }
    };
    char actual_value[16];

    // EXECUTE
    CyneResult result = RequestContext_get_params_value(&r, "param2", actual_value);

    // ASSERTS
    TEST_ASSERT_EQUAL_MESSAGE(SUCCESS, result, "Result mismatch.");
    TEST_ASSERT_EQUAL_STRING_MESSAGE("value2", actual_value, "Value mismatch");

}

void test_RequestContext_get_params_value_should_return_PARAM_NOT_FOUND_when_param_does_not_exist() {
    // SETUP
    RequestContext r = {
        .params={
            { .name="param1", .value="value1" },
            { .name="param2", .value="value2" }
        }
    };
    char actual_value[16];

    // EXECUTE
    CyneResult result = RequestContext_get_params_value(&r, "param3", actual_value);

    // ASSERTS
    TEST_ASSERT_EQUAL_MESSAGE(PARAM_NOT_FOUND, result, "Result mismatch.");

}

void test_RequestContext_get_params_count() {
    // SETUP
    RequestContext r = {
        .params={
            { .name="param1", .value="value1" },
            { .name="param2", .value="value2" }
        }
    };
    size_t actual_count = -1;

    // EXECUTE
    CyneResult result = RequestContext_get_params_count(&r, &actual_count);

    // ASSERTS
    TEST_ASSERT_EQUAL_MESSAGE(SUCCESS, result, "Result mismatch.");
    TEST_ASSERT_EQUAL_MESSAGE(2, actual_count, "Count mismatch.");

}

void test_RequestContext_get_params_count_should_return_0_when_params_is_empty() {
    // SETUP
    RequestContext r = {
        .params={
            { .name="", .value="" }
        }
    };
    size_t actual_count = -1;

    // EXECUTE
    CyneResult result = RequestContext_get_params_count(&r, &actual_count);

    // ASSERTS
    TEST_ASSERT_EQUAL_MESSAGE(SUCCESS, result, "Result mismatch.");
    TEST_ASSERT_EQUAL_MESSAGE(0, actual_count, "Count mismatch.");

}

void test_RequestContext_get_json_should_return_json_view_when_content_type_is_application_json() {
    // SETUP
    unsigned char json_buffer[1024];
    Arena json_arena;
    Arena_init(&json_arena, json_buffer, 1024);

    RequestContext r = {
        .req = (HttpRequest){
            .headers={
               (HttpHeader){.field="Content-Type", .value="application/json"}
            },
            .headers_count=1,
            .body="{\"string\": \"This is string.\"}",
            .content_length=29,
        }
    };

    JsonObjectView* json;

    // EXECUTE
    CyneResult result = RequestContext_get_json(&r, &json_arena, (JsonValueView**)&json);

    // ASSERTS
    TEST_ASSERT_EQUAL_MESSAGE(SUCCESS, result, "Result mismatch.");
    TEST_ASSERT_NOT_EQUAL_MESSAGE(0, json_arena.offset, "Arena offset mismatch.");
    TEST_ASSERT_EQUAL_MESSAGE(JSON_OBJ, json->value.type, "Type mismatch.");
    TEST_ASSERT_EQUAL_MESSAGE(1, json->size, "Size mismatch.");
    TEST_ASSERT_EQUAL_MESSAGE(2, json->members[0]->key.start, "Key start mismatch.");
    TEST_ASSERT_EQUAL_MESSAGE(8, json->members[0]->key.end, "Key end mismatch.");
    TEST_ASSERT_EQUAL_MESSAGE(JSON_STR, json->members[0]->value->type, "Value type mismatch.");
    TEST_ASSERT_EQUAL_MESSAGE(12, ((JsonNativeView*)json->members[0]->value)->window.start, "Value start mismatch.");
    TEST_ASSERT_EQUAL_MESSAGE(27, ((JsonNativeView*)json->members[0]->value)->window.end, "Value end mismatch.");
}

void test_RequestContext_get_json_should_return_NOT_JSON_CONTENT_when_content_type_is_not_application_json() {
    // SETUP
    unsigned char json_buffer[1024];
    Arena json_arena;
    Arena_init(&json_arena, json_buffer, 1024);

    RequestContext r = {
        .req = (HttpRequest){
            .headers={
               (HttpHeader){.field="Content-Type", .value="text/plain"}
            },
            .headers_count=1,
            .body="{\"string\": \"This is string.\"}",
            .content_length=29,
        }
    };

    JsonObjectView* json;

    // EXECUTE
    CyneResult result = RequestContext_get_json(&r, &json_arena, (JsonValueView**)&json);

    // ASSERTS
    TEST_ASSERT_EQUAL_MESSAGE(NOT_JSON_CONTENT, result, "Result mismatch.");
    TEST_ASSERT_EQUAL_MESSAGE(0, json_arena.offset, "Arena offset mismatch.");
}

int main() {
    UNITY_BEGIN();

    RUN_TEST(test_RequestContext_get_params_value);
    RUN_TEST(test_RequestContext_get_params_value_should_return_PARAM_NOT_FOUND_when_param_does_not_exist);

    RUN_TEST(test_RequestContext_get_params_count);
    RUN_TEST(test_RequestContext_get_params_count_should_return_0_when_params_is_empty);

    RUN_TEST(test_RequestContext_get_json_should_return_json_view_when_content_type_is_application_json);
    RUN_TEST(test_RequestContext_get_json_should_return_NOT_JSON_CONTENT_when_content_type_is_not_application_json);

    return UNITY_END();
}