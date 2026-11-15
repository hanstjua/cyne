#include <stdio.h>
#include <string.h>

#include "../libs/unity/src/unity.h"
#include "../src/url.h"


#define TEST_ASSERT_EQUAL_ENDPOINT_VAR(a, b) assert_endpoint_variable(a, b);
void assert_endpoint_variable(EndpointVariable expected, EndpointVariable actual) {
    TEST_ASSERT_EQUAL_STRING_MESSAGE(expected.name, actual.name, "UrlVariable name mismatch.");
    TEST_ASSERT_EQUAL_MESSAGE(expected.type, actual.type, "UrlVariable type mismatch.");
    
    switch(expected.type) {
        case STRING:
        TEST_ASSERT_EQUAL_STRING_MESSAGE(expected.s, actual.s, "UrlVariable string mismatch.");
        break;
        case PATH:
        TEST_ASSERT_EQUAL_STRING_MESSAGE(expected.p, actual.p, "UrlVariable path mismatch.");
        break;
        case INT:
        TEST_ASSERT_EQUAL_MESSAGE(expected.i, actual.i, "UrlVariable integer mismatch.");
        break;
        case FLOAT:
        TEST_ASSERT_EQUAL_MESSAGE(expected.f, actual.f, "UrlVariable float mismatch.");
        break;
        case UNKNOWN_VAR_TYPE:
        break;
    }
}

void setUp(void) {}
void tearDown(void) {}

void test_parse_endpoint() {
    // SETUP
    char input[MAX_ENDPOINT_LEN + 1] = "/target1/target2/xxxheyxxx/there/more/path/123/123.0";
    char rule[MAX_ENDPOINT_RULE_LEN + 1] = "/target1/target2/xxx<no_type>xxx/<s:s_type>/<p:p_type>/<i:i_type>/<f:f_type>";
    EndpointVariable expected_vars[MAX_ENDPOINT_VARIABLE_COUNT] = {
        {.name="no_type", .type=STRING, .s="hey"},
        {.name="s_type", .type=STRING, .s="there"},
        {.name="p_type", .type=PATH, .p="more/path"},
        {.name="i_type", .type=INT, .i=123},
        {.name="f_type", .type=FLOAT, .f=123.0}
    };
    bool actual_match = false;
    EndpointVariable actual_vars[MAX_ENDPOINT_VARIABLE_COUNT];
    CyneResult result;

    // EXECUTE
    result = parse_endpoint(input, rule, &actual_match, actual_vars);

    // ASSERTS
    TEST_ASSERT_EQUAL_MESSAGE(SUCCESS, result, "Result mismatch.");
    TEST_ASSERT_EQUAL_MESSAGE(true, actual_match, "Match mismatch.");
    for (int i=0; i<5; i++) {
        TEST_ASSERT_EQUAL_ENDPOINT_VAR(expected_vars[i], actual_vars[i]);
    }
    TEST_ASSERT_EQUAL_MESSAGE(UNKNOWN_VAR_TYPE, actual_vars[5].type, "'Terminating' endpoint variable not found.");
}

void test_parse_endpoint_should_output_false_when_endpoint_and_rule_does_not_match() {
    // SETUP
    char input[MAX_ENDPOINT_LEN + 1] = "/endpoint_will_never_match";
    char rule[MAX_ENDPOINT_RULE_LEN + 1] = "/endpoint_will_never_matcg";
    bool actual_match = true;
    EndpointVariable actual_vars[MAX_ENDPOINT_VARIABLE_COUNT];
    CyneResult result;

    // EXECUTE
    result = parse_endpoint(input, rule, &actual_match, actual_vars);

    // ASSSERTS
    TEST_ASSERT_EQUAL_MESSAGE(SUCCESS, result, "Result mismatch.");
    TEST_ASSERT_EQUAL_MESSAGE(false, actual_match, "Match mismatch.");
    TEST_ASSERT_EQUAL_MESSAGE(UNKNOWN_VAR_TYPE, actual_vars[0].type, "URL Variable type mismatch.");
}

void test_parse_endpoint_should_return_MAX_URL_MATCHING_ATTEMPTS_EXCEEDED_when_endpoint_matching_attempt_limit_is_reached() {
    // SETUP
    char input[MAX_ENDPOINT_LEN + 1] = "/endpoint_will_never_match_because_it_does_not_have_integer";
    char rule[MAX_ENDPOINT_RULE_LEN + 1] = "/<string><string1><string2><string3><string4><string5><i:integer>";
    bool actual_match = true;
    EndpointVariable actual_vars[MAX_ENDPOINT_VARIABLE_COUNT];
    CyneResult result;

    // EXECUTE
    result = parse_endpoint(input, rule, &actual_match, actual_vars);

    // ASSSERTS
    TEST_ASSERT_EQUAL_MESSAGE(MAX_ENDPOINT_MATCHING_ATTEMPTS_EXCEEDED, result, "Result mismatch.");
    TEST_ASSERT_EQUAL_MESSAGE(false, actual_match, "Match mismatch.");
    TEST_ASSERT_EQUAL_MESSAGE(UNKNOWN_VAR_TYPE, actual_vars[0].type, "endpoint Variable type mismatch.");
}

void test_parse_endpoint_should_return_TOO_MANY_URL_VARIABLES_when_rule_specifies_too_many_endpoint_variables() {
    // SETUP
    char input[MAX_ENDPOINT_LEN + 1] = "/rule_has_too_many_variables";
    char rule[MAX_ENDPOINT_RULE_LEN + 1] = "/<s0><s1><s2><s3><s4><s5><s6><s7><s8><s9><s10><s11><s12><s13><s14><s15><s16><s17><s18><s19>";
    bool actual_match = true;
    EndpointVariable actual_vars[MAX_ENDPOINT_VARIABLE_COUNT];
    CyneResult result;

    // EXECUTE
    result = parse_endpoint(input, rule, &actual_match, actual_vars);

    // ASSSERTS
    TEST_ASSERT_EQUAL_MESSAGE(TOO_MANY_ENDPOINT_VARIABLES, result, "Result mismatch.");
    TEST_ASSERT_EQUAL_MESSAGE(false, actual_match, "Match mismatch.");
    TEST_ASSERT_EQUAL_MESSAGE(UNKNOWN_VAR_TYPE, actual_vars[0].type, "endpoint Variable type mismatch.");
}

void test_parse_endpoint_should_return_URL_RULE_TOO_LONG_when_rule_exceeds_length_limit() {
    // SETUP
    char input[MAX_ENDPOINT_LEN + 1] = "/rule_too_long";
    char* rule = "/0123456789101112131415161718192021222324252627282930313233343536373839404142434445464748495051525354555657585960616263646566676869";
    bool actual_match = true;
    EndpointVariable actual_vars[MAX_ENDPOINT_VARIABLE_COUNT];
    CyneResult result;

    // EXECUTE
    result = parse_endpoint(input, rule, &actual_match, actual_vars);

    // ASSSERTS
    TEST_ASSERT_EQUAL_MESSAGE(ENDPOINT_RULE_TOO_LONG, result, "Result mismatch.");
    TEST_ASSERT_EQUAL_MESSAGE(false, actual_match, "Match mismatch.");
    TEST_ASSERT_EQUAL_MESSAGE(UNKNOWN_VAR_TYPE, actual_vars[0].type, "endpoint Variable type mismatch.");
}

void test_parse_endpoint_should_return_EMPTY_URL_RULE_when_endpoint_rule_is_empty_string() {
    // SETUP
    char* input = "/";
    char* rule = "";
    bool actual_match = true;
    EndpointVariable actual_vars[MAX_ENDPOINT_VARIABLE_COUNT];
    CyneResult result;

    // EXECUTE
    result = parse_endpoint(input, rule, &actual_match, actual_vars);

    // ASSSERTS
    TEST_ASSERT_EQUAL_MESSAGE(EMPTY_ENDPOINT_RULE, result, "Result mismatch.");
    TEST_ASSERT_EQUAL_MESSAGE(false, actual_match, "Match mismatch.");
    TEST_ASSERT_EQUAL_MESSAGE(UNKNOWN_VAR_TYPE, actual_vars[0].type, "endpoint Variable type mismatch.");
}

void test_parse_endpoint_should_return_URL_TOO_LONG_when_endpoint_exceeds_length_limit() {
    // SETUP
    char* input = "/01234567891112131415161718192021222324252627282930313233343536373839";
    char rule[MAX_ENDPOINT_RULE_LEN + 1] = "/endpoint_too_long";
    bool actual_match = true;
    EndpointVariable actual_vars[MAX_ENDPOINT_VARIABLE_COUNT];
    CyneResult result;

    // EXECUTE
    result = parse_endpoint(input, rule, &actual_match, actual_vars);

    // ASSSERTS
    TEST_ASSERT_EQUAL_MESSAGE(ENDPOINT_TOO_LONG, result, "Result mismatch.");
    TEST_ASSERT_EQUAL_MESSAGE(false, actual_match, "Match mismatch.");
    TEST_ASSERT_EQUAL_MESSAGE(UNKNOWN_VAR_TYPE, actual_vars[0].type, "endpoint Variable type mismatch.");
}

void test_parse_endpoint_should_return_EMPTY_URL_when_endpoint_is_empty_string() {
    // SETUP
    char* input = "";
    char* rule = "/";
    bool actual_match = true;
    EndpointVariable actual_vars[MAX_ENDPOINT_VARIABLE_COUNT];
    CyneResult result;

    // EXECUTE
    result = parse_endpoint(input, rule, &actual_match, actual_vars);

    // ASSSERTS
    TEST_ASSERT_EQUAL_MESSAGE(EMPTY_ENDPOINT, result, "Result mismatch.");
    TEST_ASSERT_EQUAL_MESSAGE(false, actual_match, "Match mismatch.");
    TEST_ASSERT_EQUAL_MESSAGE(UNKNOWN_VAR_TYPE, actual_vars[0].type, "endpoint Variable type mismatch.");
}

void test_parse_endpoint_should_return_URL_VARIABLE_VALUE_TOO_LONG_when_string_value_exceeds_length_limit() {
    // SETUP
    char input[MAX_ENDPOINT_LEN + 1] = "/this_string_value_will_definitely_exceed_the_length_limit";
    char rule[MAX_ENDPOINT_RULE_LEN + 1] = "/<some_var>";
    bool actual_match = true;
    EndpointVariable actual_vars[MAX_ENDPOINT_VARIABLE_COUNT];
    CyneResult result;

    // EXECUTE
    result = parse_endpoint(input, rule, &actual_match, actual_vars);

    // ASSSERTS
    TEST_ASSERT_EQUAL_MESSAGE(ENDPOINT_VARIABLE_VALUE_TOO_LONG, result, "Result mismatch.");
    TEST_ASSERT_EQUAL_MESSAGE(false, actual_match, "Match mismatch.");
    TEST_ASSERT_EQUAL_MESSAGE(UNKNOWN_VAR_TYPE, actual_vars[0].type, "endpoint Variable type mismatch.");
}

void test_parse_endpoint_should_return_URL_VARIABLE_VALUE_TOO_LONG_when_path_value_exceeds_length_limit() {
    // SETUP
    char input[MAX_ENDPOINT_LEN + 1] = "/this/path/value/will/definitely/exceed/the/length/limit";
    char rule[MAX_ENDPOINT_RULE_LEN + 1] = "/<p:some_path>";
    bool actual_match = true;
    EndpointVariable actual_vars[MAX_ENDPOINT_VARIABLE_COUNT];
    CyneResult result;

    // EXECUTE
    result = parse_endpoint(input, rule, &actual_match, actual_vars);

    // ASSSERTS
    TEST_ASSERT_EQUAL_MESSAGE(ENDPOINT_VARIABLE_VALUE_TOO_LONG, result, "Result mismatch.");
    TEST_ASSERT_EQUAL_MESSAGE(false, actual_match, "Match mismatch.");
    TEST_ASSERT_EQUAL_MESSAGE(UNKNOWN_VAR_TYPE, actual_vars[0].type, "endpoint Variable type mismatch.");
}

void test_parse_endpoint_should_return_URL_VARIABLE_NAME_TOO_LONG_when_endpoint_variable_name_exceeds_length_limit() {
    // SETUP
    char input[MAX_ENDPOINT_LEN + 1] = "/this_is_the_value";
    char rule[MAX_ENDPOINT_RULE_LEN + 1] = "/<p:ridiculously_long_variable_name>";
    bool actual_match = true;
    EndpointVariable actual_vars[MAX_ENDPOINT_VARIABLE_COUNT];
    CyneResult result;

    // EXECUTE
    result = parse_endpoint(input, rule, &actual_match, actual_vars);

    // ASSSERTS
    TEST_ASSERT_EQUAL_MESSAGE(ENDPOINT_VARIABLE_NAME_TOO_LONG, result, "Result mismatch.");
    TEST_ASSERT_EQUAL_MESSAGE(false, actual_match, "Match mismatch.");
    TEST_ASSERT_EQUAL_MESSAGE(UNKNOWN_VAR_TYPE, actual_vars[0].type, "endpoint Variable type mismatch.");
}

void test_QueryParamParser_init() {
    // SETUP
    QueryParam params[1];
    QueryParamParser parser;
    CyneResult result;
    params[0].name[0] = 0;
    params[0].value[0] = 0;

    // EXECUTE
    result = QueryParamParser_init(&parser, params, 1);

    TEST_ASSERT_EQUAL_MESSAGE(SUCCESS, result, "Result mismatch.");
    TEST_ASSERT_EQUAL_MESSAGE(params, parser.params, "Params mismatch.");
    TEST_ASSERT_EQUAL_MESSAGE(PARSE_NAME, parser.state, "Parser state mismatch.");
    TEST_ASSERT_EQUAL_MESSAGE(1, parser.params_capacity, "Params capacity mismatch.");
    TEST_ASSERT_EQUAL_MESSAGE(0, parser.params_count, "Params count mismatch.");

}

void test_QueryParamParser_init_should_set_last_param_to_last_non_empty_slot_in_params_if_any() {
    // SETUP
    QueryParam params[3];
    QueryParamParser parser;
    CyneResult result;

    snprintf(params[0].name, MAX_QUERY_PARAM_NAME_LEN, "%s", "uuid");
    snprintf(params[0].value, MAX_QUERY_PARAM_VALUE_LEN, "%s", "abcd1234");
    snprintf(params[1].name, MAX_QUERY_PARAM_NAME_LEN, "%s", "column");
    snprintf(params[1].value, MAX_QUERY_PARAM_VALUE_LEN, "%s", "title");
    params[2].name[0] = 0;
    params[2].value[0] = 0;

    // EXECUTE
    result = QueryParamParser_init(&parser, params, 3);

    TEST_ASSERT_EQUAL_MESSAGE(SUCCESS, result, "Result mismatch.");
    TEST_ASSERT_EQUAL_MESSAGE(params, parser.params, "Params mismatch.");
    TEST_ASSERT_EQUAL_MESSAGE(PARSE_NAME, parser.state, "Parser state mismatch.");
    TEST_ASSERT_EQUAL_MESSAGE(3, parser.params_capacity, "Params capacity mismatch.");
    TEST_ASSERT_EQUAL_MESSAGE(2, parser.params_count, "Params count mismatch.");

}

void test_QueryParamParser_parse() {
    // SETUP
    QueryParam params[1];
    QueryParamParser parser;
    CyneResult result;
    size_t actual_count;
    char* url = "http://wwww.google.com?keyword=hello";

    // EXECUTE
    QueryParamParser_init(&parser, params, 1);
    result = QueryParamParser_parse(&parser, url, &actual_count);

    TEST_ASSERT_EQUAL_MESSAGE(SUCCESS, result, "Result mismatch.");
    TEST_ASSERT_EQUAL_STRING_MESSAGE("keyword", params[0].name, "Param name mismatch.");
    TEST_ASSERT_EQUAL_STRING_MESSAGE("hello", params[0].value, "Param value mismatch.");
    TEST_ASSERT_EQUAL_MESSAGE(PARSE_VALUE, parser.state, "Parser state mismatch.");
    TEST_ASSERT_EQUAL_MESSAGE(1, parser.params_count, "Params count mismatch.");
    TEST_ASSERT_EQUAL_MESSAGE(1, actual_count, "Count mismatch.");

}

void test_QueryParamParser_parse_should_work_when_out_count_is_NULL() {
    // SETUP
    QueryParam params[1];
    QueryParamParser parser;
    CyneResult result;
    char* url = "http://wwww.google.com?keyword=hello";

    // EXECUTE
    QueryParamParser_init(&parser, params, 1);
    result = QueryParamParser_parse(&parser, url, NULL);

    TEST_ASSERT_EQUAL_MESSAGE(SUCCESS, result, "Result mismatch.");
    TEST_ASSERT_EQUAL_STRING_MESSAGE("keyword", params[0].name, "Param name mismatch.");
    TEST_ASSERT_EQUAL_STRING_MESSAGE("hello", params[0].value, "Param value mismatch.");
    TEST_ASSERT_EQUAL_MESSAGE(PARSE_VALUE, parser.state, "Parser state mismatch.");
    TEST_ASSERT_EQUAL_MESSAGE(1, parser.params_count, "Params count mismatch.");

}

void test_QueryParamParser_parse_should_parse_multiple_params() {
    // SETUP
    QueryParam params[2];
    QueryParamParser parser;
    CyneResult result;
    size_t actual_count;
    char* url = "http://wwww.google.com?keyword=hello&platform=windows";
    for (int i=0; i<2; i++) {
        params[i].name[0] = 0;
        params[i].value[0] = 0;
    }

    // EXECUTE
    QueryParamParser_init(&parser, params, 2);
    result = QueryParamParser_parse(&parser, url, &actual_count);

    TEST_ASSERT_EQUAL_MESSAGE(SUCCESS, result, "Result mismatch.");
    TEST_ASSERT_EQUAL_STRING_MESSAGE("keyword", params[0].name, "Param name mismatch.");
    TEST_ASSERT_EQUAL_STRING_MESSAGE("hello", params[0].value, "Param value mismatch.");
    TEST_ASSERT_EQUAL_STRING_MESSAGE("platform", params[1].name, "Param name mismatch.");
    TEST_ASSERT_EQUAL_STRING_MESSAGE("windows", params[1].value, "Param value mismatch.");
    TEST_ASSERT_EQUAL_MESSAGE(PARSE_VALUE, parser.state, "Parser state mismatch.");
    TEST_ASSERT_EQUAL_MESSAGE(2, parser.params_count, "Params count mismatch.");
    TEST_ASSERT_EQUAL_MESSAGE(2, actual_count, "Count mismatch.");

}

void test_QueryParamParser_parse_should_parse_url_fragment_starting_with_query_divider_char() {
    // SETUP
    QueryParam params[2];
    QueryParamParser parser;
    CyneResult result;
    size_t actual_count;
    char* url[] = {
        "http://wwww.mysite.com?name=foo",
        "&column=bar"
    };
    for (int i=0; i<2; i++) {
        params[i].name[0] = 0;
        params[i].value[0] = 0;
    }

    // EXECUTE
    QueryParamParser_init(&parser, params, 1);
    for (int i=0; i<(int)(sizeof(url) / sizeof(char*)); i++){
        result = QueryParamParser_parse(&parser, url[i], &actual_count);
        TEST_ASSERT_EQUAL_MESSAGE(SUCCESS, result, "Result mismatch.");
        TEST_ASSERT_EQUAL_MESSAGE(1, actual_count, "Count mismatch.");

    }

    TEST_ASSERT_EQUAL_STRING_MESSAGE("name", params[0].name, "Param name mismatch.");
    TEST_ASSERT_EQUAL_STRING_MESSAGE("foo", params[0].value, "Param value mismatch.");
    TEST_ASSERT_EQUAL_STRING_MESSAGE("column", params[1].name, "Param name mismatch.");
    TEST_ASSERT_EQUAL_STRING_MESSAGE("bar", params[1].value, "Param value mismatch.");
    TEST_ASSERT_EQUAL_MESSAGE(PARSE_VALUE, parser.state, "Parser state mismatch.");
    TEST_ASSERT_EQUAL_MESSAGE(2, parser.params_count, "Params count mismatch.");

}

void test_QueryParamParser_parse_should_parse_url_fragment_ending_with_query_divider_char() {
    // SETUP
    QueryParam params[2];
    QueryParamParser parser;
    CyneResult result;
    size_t actual_count;
    char* url[] = {
        "http://wwww.mysite.com?name=foo&",
        "column=bar"
    };
    for (int i=0; i<2; i++) {
        params[i].name[0] = 0;
        params[i].value[0] = 0;
    }

    // EXECUTE
    QueryParamParser_init(&parser, params, 1);
    for (int i=0; i<(int)(sizeof(url) / sizeof(char*)); i++){
        result = QueryParamParser_parse(&parser, url[i], &actual_count);
        TEST_ASSERT_EQUAL_MESSAGE(SUCCESS, result, "Result mismatch.");
        TEST_ASSERT_EQUAL_MESSAGE(1, actual_count, "Count mismatch.");

    }

    TEST_ASSERT_EQUAL_STRING_MESSAGE("name", params[0].name, "Param name mismatch.");
    TEST_ASSERT_EQUAL_STRING_MESSAGE("foo", params[0].value, "Param value mismatch.");
    TEST_ASSERT_EQUAL_STRING_MESSAGE("column", params[1].name, "Param name mismatch.");
    TEST_ASSERT_EQUAL_STRING_MESSAGE("bar", params[1].value, "Param value mismatch.");
    TEST_ASSERT_EQUAL_MESSAGE(PARSE_VALUE, parser.state, "Parser state mismatch.");
    TEST_ASSERT_EQUAL_MESSAGE(2, parser.params_count, "Params count mismatch.");

}

void test_QueryParamParser_parse_should_parse_url_fragment_with_separated_name_and_value() {
    // SETUP
    QueryParam params[2];
    QueryParamParser parser;
    CyneResult result;
    size_t actual_count;
    char* url[] = {
        "http://wwww.mysite.com?name=",
        "foo&column",
        "=bar"
    };
    int expected_counts[] = {0, 1, 1};

    for (int i=0; i<2; i++) {
        params[i].name[0] = 0;
        params[i].value[0] = 0;
    }

    // EXECUTE
    QueryParamParser_init(&parser, params, 1);
    for (int i=0; i<(int)(sizeof(url) / sizeof(char*)); i++){
        result = QueryParamParser_parse(&parser, url[i], &actual_count);
        TEST_ASSERT_EQUAL_MESSAGE(SUCCESS, result, "Result mismatch.");
        TEST_ASSERT_EQUAL_MESSAGE(expected_counts[i], actual_count, "Count mismatch.");

    }

    TEST_ASSERT_EQUAL_STRING_MESSAGE("name", params[0].name, "Param name mismatch.");
    TEST_ASSERT_EQUAL_STRING_MESSAGE("foo", params[0].value, "Param value mismatch.");
    TEST_ASSERT_EQUAL_STRING_MESSAGE("column", params[1].name, "Param name mismatch.");
    TEST_ASSERT_EQUAL_STRING_MESSAGE("bar", params[1].value, "Param value mismatch.");
    TEST_ASSERT_EQUAL_MESSAGE(PARSE_VALUE, parser.state, "Parser state mismatch.");
    TEST_ASSERT_EQUAL_MESSAGE(2, parser.params_count, "Params count mismatch.");

}

void test_QueryParamParser_parse_should_parse_first_url_fragment_with_no_name() {
    // SETUP
    QueryParam params[1];
    QueryParamParser parser;
    CyneResult result;
    size_t actual_count;
    char* url[] = {
        "http://wwww.mysite.com?",
        "name=foo"
    };
    int expected_counts[] = {0, 1};

    params[0].name[0] = 0;
    params[0].value[0] = 0;

    // EXECUTE
    QueryParamParser_init(&parser, params, 1);
    for (int i=0; i<(int)(sizeof(url) / sizeof(char*)); i++){
        result = QueryParamParser_parse(&parser, url[i], &actual_count);
        TEST_ASSERT_EQUAL_MESSAGE(SUCCESS, result, "Result mismatch.");
        TEST_ASSERT_EQUAL_MESSAGE(expected_counts[i], actual_count, "Count mismatch.");

    }

    TEST_ASSERT_EQUAL_STRING_MESSAGE("name", params[0].name, "Param name mismatch.");
    TEST_ASSERT_EQUAL_STRING_MESSAGE("foo", params[0].value, "Param value mismatch.");
    TEST_ASSERT_EQUAL_MESSAGE(PARSE_VALUE, parser.state, "Parser state mismatch.");
    TEST_ASSERT_EQUAL_MESSAGE(1, parser.params_count, "Params count mismatch.");

}

void test_QueryParamParser_parse_should_parse_url_fragment_with_incomplete_name() {
    // SETUP
    QueryParam params[1];
    QueryParamParser parser;
    CyneResult result;
    size_t actual_count;
    char* url[] = {
        "http://wwww.mysite.com?na",
        "me=foo"
    };
    int expected_counts[] = {0, 1};

    params[0].name[0] = 0;
    params[0].value[0] = 0;

    // EXECUTE
    QueryParamParser_init(&parser, params, 1);
    for (int i=0; i<(int)(sizeof(url) / sizeof(char*)); i++){
        result = QueryParamParser_parse(&parser, url[i], &actual_count);
        TEST_ASSERT_EQUAL_MESSAGE(SUCCESS, result, "Result mismatch.");
        TEST_ASSERT_EQUAL_MESSAGE(expected_counts[i], actual_count, "Count mismatch.");

    }

    TEST_ASSERT_EQUAL_STRING_MESSAGE("name", params[0].name, "Param name mismatch.");
    TEST_ASSERT_EQUAL_STRING_MESSAGE("foo", params[0].value, "Param value mismatch.");
    TEST_ASSERT_EQUAL_MESSAGE(PARSE_VALUE, parser.state, "Parser state mismatch.");
    TEST_ASSERT_EQUAL_MESSAGE(1, parser.params_count, "Params count mismatch.");

}

void test_QueryParamParser_parse_should_parse_name_that_spans_across_different_chunks() {
    // SETUP
    QueryParam params[1];
    QueryParamParser parser;
    CyneResult result;
    size_t actual_count;
    char* url[] = {
        "http://wwww.mysite.com?a",
        "verylongparam",
        "name=value"
    };
    int expected_counts[] = {0, 0, 1};

    params[0].name[0] = 0;
    params[0].value[0] = 0;

    // EXECUTE
    QueryParamParser_init(&parser, params, 1);
    for (int i=0; i<(int)(sizeof(url) / sizeof(char*)); i++){
        result = QueryParamParser_parse(&parser, url[i], &actual_count);
        TEST_ASSERT_EQUAL_MESSAGE(SUCCESS, result, "Result mismatch.");
        TEST_ASSERT_EQUAL_MESSAGE(expected_counts[i], actual_count, "Count mismatch.");

    }

    TEST_ASSERT_EQUAL_STRING_MESSAGE("averylongparamname", params[0].name, "Param name mismatch.");
    TEST_ASSERT_EQUAL_STRING_MESSAGE("value", params[0].value, "Param value mismatch.");
    TEST_ASSERT_EQUAL_MESSAGE(PARSE_VALUE, parser.state, "Parser state mismatch.");
    TEST_ASSERT_EQUAL_MESSAGE(1, parser.params_count, "Params count mismatch.");

}

void test_QueryParamParser_parse_should_parse_value_that_spans_across_different_chunks() {
    // SETUP
    QueryParam params[1];
    QueryParamParser parser;
    CyneResult result;
    size_t actual_count;
    char* url[] = {
        "http://wwww.mysite.com?name=a",
        "verylong",
        "value"
    };
    int expected_counts[] = {1, 1, 1};

    params[0].name[0] = 0;
    params[0].value[0] = 0;

    // EXECUTE
    QueryParamParser_init(&parser, params, 1);
    for (int i=0; i<(int)(sizeof(url) / sizeof(char*)); i++){
        result = QueryParamParser_parse(&parser, url[i], &actual_count);
        TEST_ASSERT_EQUAL_MESSAGE(SUCCESS, result, "Result mismatch.");
        TEST_ASSERT_EQUAL_MESSAGE(expected_counts[i], actual_count, "Count mismatch.");

    }

    TEST_ASSERT_EQUAL_STRING_MESSAGE("name", params[0].name, "Param name mismatch.");
    TEST_ASSERT_EQUAL_STRING_MESSAGE("averylongvalue", params[0].value, "Param value mismatch.");
    TEST_ASSERT_EQUAL_MESSAGE(PARSE_VALUE, parser.state, "Parser state mismatch.");
    TEST_ASSERT_EQUAL_MESSAGE(1, parser.params_count, "Params count mismatch.");

}

void test_QueryParamParser_parse_should_parse_multi_value_param() {
    // SETUP
    QueryParam params[1];
    QueryParamParser parser;
    CyneResult result;
    size_t actual_count;
    char* url[] = {
        "http://wwww.mysite.com?",
        "name=value1&name=value2&",
        "name=val",
        "ue3&na",
        "me=value4"
    };
    int expected_counts[] = {0, 1, 1, 1, 1};

    params[0].name[0] = 0;
    params[0].value[0] = 0;

    // EXECUTE
    QueryParamParser_init(&parser, params, 1);
    for (int i=0; i<(int)(sizeof(url) / sizeof(char*)); i++){
        result = QueryParamParser_parse(&parser, url[i], &actual_count);
        TEST_ASSERT_EQUAL_MESSAGE(SUCCESS, result, "Result mismatch.");
        TEST_ASSERT_EQUAL_MESSAGE(expected_counts[i], actual_count, "Count mismatch.");

    }

    TEST_ASSERT_EQUAL_STRING_MESSAGE("name", params[0].name, "Param name mismatch.");
    TEST_ASSERT_EQUAL_STRING_MESSAGE("value1,value2,value3,value4", params[0].value, "Param value mismatch.");
    TEST_ASSERT_EQUAL_MESSAGE(PARSE_VALUE, parser.state, "Parser state mismatch.");
    TEST_ASSERT_EQUAL_MESSAGE(1, parser.params_count, "Params count mismatch.");

}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_parse_endpoint);
    RUN_TEST(test_parse_endpoint_should_output_false_when_endpoint_and_rule_does_not_match);
    RUN_TEST(test_parse_endpoint_should_return_MAX_URL_MATCHING_ATTEMPTS_EXCEEDED_when_endpoint_matching_attempt_limit_is_reached);
    RUN_TEST(test_parse_endpoint_should_return_TOO_MANY_URL_VARIABLES_when_rule_specifies_too_many_endpoint_variables);
    RUN_TEST(test_parse_endpoint_should_return_URL_RULE_TOO_LONG_when_rule_exceeds_length_limit);
    RUN_TEST(test_parse_endpoint_should_return_EMPTY_URL_RULE_when_endpoint_rule_is_empty_string);
    RUN_TEST(test_parse_endpoint_should_return_URL_TOO_LONG_when_endpoint_exceeds_length_limit);
    RUN_TEST(test_parse_endpoint_should_return_EMPTY_URL_when_endpoint_is_empty_string);
    RUN_TEST(test_parse_endpoint_should_return_URL_VARIABLE_VALUE_TOO_LONG_when_string_value_exceeds_length_limit);
    RUN_TEST(test_parse_endpoint_should_return_URL_VARIABLE_VALUE_TOO_LONG_when_path_value_exceeds_length_limit);
    RUN_TEST(test_parse_endpoint_should_return_URL_VARIABLE_NAME_TOO_LONG_when_endpoint_variable_name_exceeds_length_limit);

    RUN_TEST(test_QueryParamParser_init);
    RUN_TEST(test_QueryParamParser_parse_should_work_when_out_count_is_NULL);
    RUN_TEST(test_QueryParamParser_init_should_set_last_param_to_last_non_empty_slot_in_params_if_any);
    RUN_TEST(test_QueryParamParser_parse);
    RUN_TEST(test_QueryParamParser_parse_should_parse_multiple_params);
    RUN_TEST(test_QueryParamParser_parse_should_parse_url_fragment_starting_with_query_divider_char);
    RUN_TEST(test_QueryParamParser_parse_should_parse_url_fragment_ending_with_query_divider_char);
    RUN_TEST(test_QueryParamParser_parse_should_parse_url_fragment_with_separated_name_and_value);
    RUN_TEST(test_QueryParamParser_parse_should_parse_first_url_fragment_with_no_name);
    RUN_TEST(test_QueryParamParser_parse_should_parse_url_fragment_with_incomplete_name);
    RUN_TEST(test_QueryParamParser_parse_should_parse_name_that_spans_across_different_chunks);
    RUN_TEST(test_QueryParamParser_parse_should_parse_value_that_spans_across_different_chunks);
    RUN_TEST(test_QueryParamParser_parse_should_parse_multi_value_param);

    return UNITY_END();
}
