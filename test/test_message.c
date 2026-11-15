#include <stdlib.h>
#include <string.h>

#include "../libs/unity/src/unity.h"
#include "../src/message.h"
#include "../src/json.h"

void setUp(void) {}
void tearDown(void) {}

#define TEST_ASSERT_REQ(e, a) assert_req(e, a);
void assert_req(HttpRequest expected, HttpRequest actual) {
    TEST_ASSERT_EQUAL_MESSAGE(expected.method, actual.method, "Request method mismatch.");
    TEST_ASSERT_EQUAL_STRING_MESSAGE(expected.request_target, actual.request_target, "Request target mismatch.");
    TEST_ASSERT_EQUAL_MESSAGE(expected.headers_count, actual.headers_count, "Header count mismatch.");
    for (size_t i=0; i<actual.headers_count; i++) {
        TEST_ASSERT_EQUAL_STRING_MESSAGE(expected.headers[i].field, actual.headers[i].field, "Header field mismatch.");
        TEST_ASSERT_EQUAL_STRING_MESSAGE(expected.headers[i].value, actual.headers[i].value, "Header value mismtach.");
    }
    TEST_ASSERT_EQUAL_STRING_MESSAGE(expected.body, actual.body, "Body mismatch");
    TEST_ASSERT_EQUAL_MESSAGE(expected.content_length, actual.content_length, "Content length mismatch.");
}

void test_HttpRequest_from_str() {
    // SETUP
    char* input = "GET /target/path HTTP/1.1\r\nField1: Value1\r\nContent-Length: 12  \r\n\r\nThis is body";
    CyneResult result;
    HttpRequest out = {
        .method = UNKNOWN,
        .headers_count = 0,
        .headers = {},
        .content_length = 0,
    };
    HttpRequest expected_req = {
        .method = GET,
        .request_target = "/target/path",
        .headers_count = 2,
        .headers = {
            { .field = "field1", .value = "Value1" },
            { .field = "content-length", .value = "12" }
        },
        .content_length = 12,
        .body = "This is body"
    };

    // EXECUTE
    result = HttpRequest_from_str(input, &out);

    // ASSERTS
    TEST_ASSERT_EQUAL(SUCCESS, result);
    TEST_ASSERT_REQ(expected_req, out);
}

void test_HttpRequest_from_str_should_return_METHOD_PARSE_FAILED_when_method_is_invalid() {
    // SETUP
    char* input = "NONMETHOD /target/path HTTP/1.1\r\nField1: Value1\r\nContent-Length: 12  \r\n\r\nThis is body";
    CyneResult result;
    HttpRequest out = {
        .method = UNKNOWN,
        .headers_count = 0,
        .headers = {},
        .content_length = 0,
    };
    HttpRequest expected_req = {
        .method = UNKNOWN,
        .request_target = "",
        .headers_count = 0,
        .headers = {},
        .content_length = 0,
        .body = ""
    };


    // EXECUTE
    result = HttpRequest_from_str(input, &out);

    // ASSERTS
    TEST_ASSERT_EQUAL(METHOD_PARSE_FAILED, result);
    TEST_ASSERT_REQ(expected_req, out);
}

void test_HttpRequest_from_str_should_return_REQUEST_TARGET_PARSE_FAILED_when_request_target_is_invalid() {
    // SETUP
    char* input = "POST  HTTP/1.1\r\nField1: Value1\r\nContent-Length: 12  \r\n\r\nThis is body";
    CyneResult result;
    HttpRequest out = {
        .method = UNKNOWN,
        .headers_count = 0,
        .headers = {},
        .content_length = 0,
    };
    HttpRequest expected_req = {
        .method = UNKNOWN,
        .request_target = "",
        .headers_count = 0,
        .headers = {},
        .content_length = 0,
        .body = ""
    };

    // EXECUTE
    result = HttpRequest_from_str(input, &out);

    // ASSERTS
    TEST_ASSERT_EQUAL(REQUEST_TARGET_PARSE_FAILED, result);
    TEST_ASSERT_REQ(expected_req, out);
}

void test_HttpRequest_from_str_should_return_START_LINE_CRLF_NOT_FOUND_when_start_line_crlf_not_found() {
    // SETUP
    char* input = "POST / HTTP/1.1";
    CyneResult result;
    HttpRequest out = {
        .method = UNKNOWN,
        .headers_count = 0,
        .headers = {},
        .content_length = 0,
    };
    HttpRequest expected_req = {
        .method = UNKNOWN,
        .request_target = "",
        .headers_count = 0,
        .headers = {},
        .content_length = 0,
        .body = ""
    };

    // EXECUTE
    result = HttpRequest_from_str(input, &out);

    // ASSERTS
    TEST_ASSERT_EQUAL(START_LINE_CRLF_NOT_FOUND, result);
    TEST_ASSERT_REQ(expected_req, out);
}

void test_HttpRequest_from_str_should_return_HEADER_FIELD_CONTAINS_WHITESPACE_when_field_has_whitespace() {
    // SETUP
    char* input = "POST / HTTP/1.1\r\n Content-Length: 12  \r\n\r\nThis is body";
    CyneResult result;
    HttpRequest out = {
        .method = UNKNOWN,
        .headers_count = 0,
        .headers = {},
        .content_length = 0,
    };
    HttpRequest expected_req = {
        .method = UNKNOWN,
        .request_target = "",
        .headers_count = 0,
        .headers = {},
        .content_length = 0,
        .body = ""
    };

    // EXECUTE
    result = HttpRequest_from_str(input, &out);

    // ASSERTS
    TEST_ASSERT_EQUAL(HEADER_FIELD_CONTAINS_WHITESPACE, result);
    TEST_ASSERT_REQ(expected_req, out);
}

void test_HttpRequest_from_str_should_return_HEADER_FIELD_DELIMITER_NOT_FOUND_when_field_delimiter_not_found() {
    // SETUP
    char* input = "POST / HTTP/1.1\r\nContent-Length\r\n\r\nThis is body";
    CyneResult result;
    HttpRequest out = {
        .method = UNKNOWN,
        .headers_count = 0,
        .headers = {},
        .content_length = 0,
    };
    HttpRequest expected_req = {
        .method = UNKNOWN,
        .request_target = "",
        .headers_count = 0,
        .headers = {},
        .content_length = 0,
        .body = ""
    };

    // EXECUTE
    result = HttpRequest_from_str(input, &out);

    // ASSERTS
    TEST_ASSERT_EQUAL(HEADER_FIELD_DELIMITER_NOT_FOUND, result);
    TEST_ASSERT_REQ(expected_req, out);
}

void test_HttpRequest_from_str_should_return_HEADER_CRLF_NOT_FOUND_when_header_crlf_not_found() {
    // SETUP
    char* input = "POST / HTTP/1.1\r\nContent-Length: 12  ";
    CyneResult result;
    HttpRequest out = {
        .method = UNKNOWN,
        .headers_count = 0,
        .headers = {},
        .content_length = 0,
    };
    HttpRequest expected_req = {
        .method = UNKNOWN,
        .request_target = "",
        .headers_count = 0,
        .headers = {},
        .content_length = 0,
        .body = ""
    };

    // EXECUTE
    result = HttpRequest_from_str(input, &out);

    // ASSERTS
    TEST_ASSERT_EQUAL(HEADER_CRLF_NOT_FOUND, result);
    TEST_ASSERT_REQ(expected_req, out);
}

void test_HttpRequest_from_str_should_return_BODY_LEN_MISMATCH_when_actual_body_size_is_shorter_than_expected() {
    // SETUP
    char* input = "POST / HTTP/1.1\r\nContent-Length: 12\r\n\r\nShort.";
    CyneResult result;
    HttpRequest out = {
        .method = UNKNOWN,
        .headers_count = 0,
        .headers = {},
        .content_length = 0,
    };
    HttpRequest expected_req = {
        .method = UNKNOWN,
        .request_target = "",
        .headers_count = 0,
        .headers = {},
        .content_length = 0,
        .body = ""
    };

    // EXECUTE
    result = HttpRequest_from_str(input, &out);

    // ASSERTS
    TEST_ASSERT_EQUAL(BODY_LEN_MISMATCH, result);
    TEST_ASSERT_REQ(expected_req, out);
}

void test_HttpRequest_from_str_should_concat_values_when_duplicate_fields_are_found() {
    // SETUP
    char* input = "PATCH / HTTP/1.1\r\nContent-Length: 0\r\nField: 1\r\nfield: 2\r\nField: 3\r\n\r\n";
    CyneResult result;
    HttpRequest out = {
        .method = UNKNOWN,
        .headers_count = 0,
        .headers = {},
        .content_length = 0,
    };
    HttpRequest expected_req = {
        .method = PATCH,
        .request_target = "/",
        .headers_count = 2,
        .headers = {
            { .field="content-length", .value="0" },
            { .field="field", .value="1,2,3" }
        },
        .content_length = 0,
        .body = ""
    };

    // EXECUTE
    result = HttpRequest_from_str(input, &out);

    // ASSERTS
    TEST_ASSERT_EQUAL(SUCCESS, result);
    TEST_ASSERT_REQ(expected_req, out);
}

void test_HttpRequest_from_str_should_parse_chunked_body_correctly() {
    // SETUP
    char* input = "PATCH / HTTP/1.1\r\nTransfer-Encoding: chunked\r\n\r\n4\r\nThis\r\n6\r\n chunk\r\n1\r\n.\r\n0\r\n\r\n";
    CyneResult result;
    HttpRequest out = {
        .method = UNKNOWN,
        .headers_count = 0,
        .headers = {},
        .content_length = 0,
    };
    HttpRequest expected_req = {
        .method = PATCH,
        .request_target = "/",
        .headers_count = 1,
        .headers = {
            { .field="transfer-encoding", .value="chunked" }
        },
        .content_length = 11,
        .body = "This chunk."
    };

    // EXECUTE
    result = HttpRequest_from_str(input, &out);

    // ASSERTS
    TEST_ASSERT_EQUAL(SUCCESS, result);
    TEST_ASSERT_REQ(expected_req, out);
}

void test_HttpRequest_from_str_should_return_BODY_CHUNKS_PARSE_FAILED_when_chunk_size_is_mismatched() {
    // SETUP
    char* input = "PATCH / HTTP/1.1\r\nTransfer-Encoding: chunked\r\n\r\n4\r\nWrong size payload.\r\n0\r\n\r\n";
    CyneResult result;
    HttpRequest out = {
        .method = UNKNOWN,
        .headers_count = 0,
        .headers = {},
        .content_length = 0,
    };
    HttpRequest expected_req = {
        .method = UNKNOWN,
        .request_target = "",
        .headers_count = 0,
        .headers = {},
        .content_length = 0,
        .body = ""
    };

    // EXECUTE
    result = HttpRequest_from_str(input, &out);

    // ASSERTS
    TEST_ASSERT_EQUAL(BODY_CHUNKS_PARSE_FAILED, result);
    TEST_ASSERT_REQ(expected_req, out);
}

void test_HttpRequest_from_str_should_return_BODY_CHUNKS_PARSE_FAILED_when_framing_is_incorrect() {
    // SETUP
    char* input = "PATCH / HTTP/1.1\r\nTransfer-Encoding: chunked\r\n\r\n4\r\nFour\r\nNo size.\r\n0\r\n\r\n";
    CyneResult result;
    HttpRequest out = {
        .method = UNKNOWN,
        .headers_count = 0,
        .headers = {},
        .content_length = 0,
    };
    HttpRequest expected_req = {
        .method = UNKNOWN,
        .request_target = "",
        .headers_count = 0,
        .headers = {},
        .content_length = 0,
        .body = ""
    };

    // EXECUTE
    result = HttpRequest_from_str(input, &out);

    // ASSERTS
    TEST_ASSERT_EQUAL(BODY_CHUNKS_PARSE_FAILED, result);
    TEST_ASSERT_REQ(expected_req, out);
}

void test_HttpRequest_from_str_should_return_BODY_CHUNKS_PARSE_FAILED_when_ending_chunk_not_found() {
    // SETUP
    char* input = "PATCH / HTTP/1.1\r\nTransfer-Encoding: chunked\r\n\r\n4\r\nFour\r\n";
    CyneResult result;
    HttpRequest out = {
        .method = UNKNOWN,
        .headers_count = 0,
        .headers = {},
        .content_length = 0,
    };
    HttpRequest expected_req = {
        .method = UNKNOWN,
        .request_target = "",
        .headers_count = 0,
        .headers = {},
        .content_length = 0,
        .body = ""
    };

    // EXECUTE
    result = HttpRequest_from_str(input, &out);

    // ASSERTS
    TEST_ASSERT_EQUAL(BODY_CHUNKS_PARSE_FAILED, result);
    TEST_ASSERT_REQ(expected_req, out);
}

void test_HttpRequest_from_str_should_return_BODY_CHUNKS_PARSE_FAILED_when_body_chunk_ends_abruptly() {
    // SETUP
    char* input = "PATCH / HTTP/1.1\r\nTransfer-Encoding: chunked\r\n\r\n12\r\nSudden sto";
    CyneResult result;
    HttpRequest out = {
        .method = UNKNOWN,
        .headers_count = 0,
        .headers = {},
        .content_length = 0,
    };
    HttpRequest expected_req = {
        .method = UNKNOWN,
        .request_target = "",
        .headers_count = 0,
        .headers = {},
        .content_length = 0,
        .body = ""
    };

    // EXECUTE
    result = HttpRequest_from_str(input, &out);

    // ASSERTS
    TEST_ASSERT_EQUAL(BODY_CHUNKS_PARSE_FAILED, result);
    TEST_ASSERT_REQ(expected_req, out);
}

void test_HttpRequest_from_str_should_return_BODY_CHUNK_TOO_LONG_when_chunk_size_exceeds_15_digits() {
    // SETUP
    char* input = "PATCH / HTTP/1.1\r\nTransfer-Encoding: chunked\r\n\r\n1000000000000000\r\nSuper long chunk ...";
    CyneResult result;
    HttpRequest out = {
        .method = UNKNOWN,
        .headers_count = 0,
        .headers = {},
        .content_length = 0,
    };
    HttpRequest expected_req = {
        .method = UNKNOWN,
        .request_target = "",
        .headers_count = 0,
        .headers = {},
        .content_length = 0,
        .body = ""
    };

    // EXECUTE
    result = HttpRequest_from_str(input, &out);

    // ASSERTS
    TEST_ASSERT_EQUAL(BODY_CHUNK_TOO_LONG, result);
    TEST_ASSERT_REQ(expected_req, out);
}

void test_HttpRequest_from_str_should_handle_empty_body_requests() {
    // SETUP
    char* input = "GET /target/path HTTP/1.1\r\nField1: Value1\r\n\r\n";
    CyneResult result;
    HttpRequest out = {
        .method = UNKNOWN,
        .headers_count = 0,
        .headers = {},
        .content_length = 0,
    };
    HttpRequest expected_req = {
        .method = GET,
        .request_target = "/target/path",
        .headers_count = 1,
        .headers = {
            { .field = "field1", .value = "Value1" }
        },
        .content_length = 0,
        .body = ""
    };

    // EXECUTE
    result = HttpRequest_from_str(input, &out);

    // ASSERTS
    TEST_ASSERT_EQUAL(SUCCESS, result);
    TEST_ASSERT_REQ(expected_req, out);
}

void test_str_to_start_line() {
    // SETUP
    char* input = "GET /target/path HTTP/1.1\r\nField1: Value1\r\nContent-Length: 12  \r\n\r\nThis is body";
    HttpMethod method;
    char request_target[MAX_REQUEST_TARGET_LEN + 1] = "";
    size_t read;
    CyneResult result;

    // EXECUTE
    result = str_to_start_line(input, &method, request_target, &read);
    
    // ASSSERT
    TEST_ASSERT_EQUAL_MESSAGE(SUCCESS, result, "Result mismatch.");
    TEST_ASSERT_EQUAL_MESSAGE(GET, method, "Method mismatch.");
    TEST_ASSERT_EQUAL_STRING_MESSAGE("/target/path", request_target, "Target mismatch.");
    TEST_ASSERT_EQUAL_MESSAGE(27, read, "Read bytes mismatch.");
}

void test_str_to_start_line_should_return_METHOD_PARSE_FAILED_when_method_is_invalid() {
    // SETUP
    char* input = "NONMETHOD /target/path HTTP/1.1\r\nField1: Value1\r\nContent-Length: 12  \r\n\r\nThis is body";
    HttpMethod method = UNKNOWN;
    char request_target[MAX_REQUEST_TARGET_LEN + 1] = "";
    size_t read = 0;
    CyneResult result;

    // EXECUTE
    result = str_to_start_line(input, &method, request_target, &read);
    
    // ASSSERTS
    TEST_ASSERT_EQUAL_MESSAGE(METHOD_PARSE_FAILED, result, "Result mismatch.");
    TEST_ASSERT_EQUAL_MESSAGE(UNKNOWN, method, "Method mismatch.");
    TEST_ASSERT_EQUAL_STRING_MESSAGE("", request_target, "Target mismatch.");
    TEST_ASSERT_EQUAL_MESSAGE(0, read, "Read bytes mismatch.");
}

void test_str_to_start_line_should_return_START_LINE_CRLF_NOT_FOUND_when_start_line_crlf_not_found() {
    // SETUP
    char* input = "POST / HTTP/1.1";
    HttpMethod method;
    char request_target[MAX_REQUEST_TARGET_LEN + 1] = "";
    size_t read = 0;
    CyneResult result;

    // EXECUTE
    result = str_to_start_line(input, &method, request_target, &read);
    
    // ASSSERTS
    TEST_ASSERT_EQUAL_MESSAGE(START_LINE_CRLF_NOT_FOUND, result, "Result mismatch.");
    TEST_ASSERT_EQUAL_MESSAGE(UNKNOWN, method, "Method mismatch.");
    TEST_ASSERT_EQUAL_STRING_MESSAGE("", request_target, "Target mismatch.");
    TEST_ASSERT_EQUAL_MESSAGE(0, read, "Read bytes mismatch.");
}

void test_str_to_headers() {
    // SETUP
    char* input = "Field1: Value1\r\nContent-Length: 12  \r\n\r\nThis is body";
    HttpHeader headers[10];
    size_t header_count = 0;
    size_t read = 0;
    CyneResult result;

    // EXECUTE
    result = str_to_headers(input, headers, &header_count, &read);
    
    // ASSSERT
    TEST_ASSERT_EQUAL_MESSAGE(SUCCESS, result, "Result mismatch.");
    TEST_ASSERT_EQUAL_MESSAGE(2, header_count, "Header count mismatch.");
    TEST_ASSERT_EQUAL_STRING_MESSAGE("field1", headers[0].field, "Field name mismatch.");
    TEST_ASSERT_EQUAL_STRING_MESSAGE("Value1", headers[0].value, "Value mismatch.");
    TEST_ASSERT_EQUAL_STRING_MESSAGE("content-length", headers[1].field, "Field name mismatch.");
    TEST_ASSERT_EQUAL_STRING_MESSAGE("12", headers[1].value, "Value mismatch.");
    TEST_ASSERT_EQUAL_MESSAGE(40, read, "Read bytes mismatch.");
}

void test_str_to_headers_should_return_HEADER_FIELD_CONTAINS_WHITESPACE_when_field_has_whitespace() {
    // SETUP
    char* input = "   Field1: Value1\r\nContent-Length: 12  \r\n\r\nThis is body";
    HttpHeader headers[10];
    size_t header_count = 0;
    size_t read = 0;
    CyneResult result;

    // EXECUTE
    result = str_to_headers(input, headers, &header_count, &read);
    
    // ASSSERT
    TEST_ASSERT_EQUAL_MESSAGE(HEADER_FIELD_CONTAINS_WHITESPACE, result, "Result mismatch.");
    TEST_ASSERT_EQUAL_MESSAGE(0, header_count, "Header count mismatch.");
    TEST_ASSERT_EQUAL_MESSAGE(0, read, "Read bytes mismatch.");
}

void test_str_to_headers_should_return_HEADER_FIELD_DELIMITER_NOT_FOUND_when_field_delimiter_not_found() {
    // SETUP
    char* input = "Field1 Value1\r\nContent-Length: 12  \r\n\r\nThis is body";
    HttpHeader headers[10];
    size_t header_count = 0;
    size_t read = 0;
    CyneResult result;

    // EXECUTE
    result = str_to_headers(input, headers, &header_count, &read);
    
    // ASSSERT
    TEST_ASSERT_EQUAL_MESSAGE(HEADER_FIELD_CONTAINS_WHITESPACE, result, "Result mismatch.");
    TEST_ASSERT_EQUAL_MESSAGE(0, header_count, "Header count mismatch.");
    TEST_ASSERT_EQUAL_MESSAGE(0, read, "Read bytes mismatch.");
}

void test_str_to_headers_should_return_HEADER_CRLF_NOT_FOUND_when_header_crlf_not_found() {
    // SETUP
    char* input = "Field1; Value1";
    HttpHeader headers[10];
    size_t header_count = 0;
    size_t read = 0;
    CyneResult result;

    // EXECUTE
    result = str_to_headers(input, headers, &header_count, &read);
    
    // ASSSERT
    TEST_ASSERT_EQUAL_MESSAGE(HEADER_CRLF_NOT_FOUND, result, "Result mismatch.");
    TEST_ASSERT_EQUAL_MESSAGE(0, header_count, "Header count mismatch.");
    TEST_ASSERT_EQUAL_MESSAGE(0, read, "Read bytes mismatch.");
}

void test_str_to_headers_should_concat_values_when_duplicate_fields_are_found() {
    // SETUP
    char* input = "Field1: Value1\r\nfield1: Value2  \r\n\r\nThis is body";
    HttpHeader headers[10];
    size_t header_count = 0;
    size_t read = 0;
    CyneResult result;

    // EXECUTE
    result = str_to_headers(input, headers, &header_count, &read);
    
    // ASSSERT
    TEST_ASSERT_EQUAL_MESSAGE(SUCCESS, result, "Result mismatch.");
    TEST_ASSERT_EQUAL_MESSAGE(1, header_count, "Header count mismatch.");
    TEST_ASSERT_EQUAL_STRING_MESSAGE("field1", headers[0].field, "Field name mismatch.");
    TEST_ASSERT_EQUAL_STRING_MESSAGE("Value1,Value2", headers[0].value, "Value mismatch.");
    TEST_ASSERT_EQUAL_MESSAGE(36, read, "Read bytes mismatch.");
}

void test_str_to_body() {
    // SETUP
    char* input = "This is body";
    char out[16] = "";
    CyneResult result;

    // EXECUTE
    result = str_to_body(input, 12, out);
    
    // ASSSERT
    TEST_ASSERT_EQUAL_MESSAGE(SUCCESS, result, "Result mismatch.");
    TEST_ASSERT_EQUAL_STRING_MESSAGE(input, out, "Body mismatch.");
}

void test_str_to_body_return_BODY_LEN_MISMATCH_when_actual_body_size_is_shorter_than_expected() {
    // SETUP
    char* input = "This is body";
    char out[16] = "";
    CyneResult result;

    // EXECUTE
    result = str_to_body(input, 100, out);
    
    // ASSSERT
    TEST_ASSERT_EQUAL_MESSAGE(BODY_LEN_MISMATCH, result, "Result mismatch.");
    TEST_ASSERT_EQUAL_STRING_MESSAGE("", out, "Body mismatch.");
}

void test_str_to_body_chunked() {
    // SETUP
    char* input = "4\r\nThis\r\n6\r\n chunk\r\n1\r\n.\r\n0\r\n\r\n";
    char out[16] = "";
    size_t read = 0;
    CyneResult result;

    // EXECUTE
    result = str_to_body_chunked(input, out, &read);
    
    // ASSSERT
    TEST_ASSERT_EQUAL_MESSAGE(SUCCESS, result, "Result mismatch.");
    TEST_ASSERT_EQUAL_STRING_MESSAGE("This", out, "Body mismatch.");
    TEST_ASSERT_EQUAL_MESSAGE(4, read, "Read bytes mismatch.");
}

void test_str_to_body_chunked_should_return_BODY_CHUNKS_PARSE_FAILED_when_chunk_size_is_mismatched() {
    // SETUP
    char* input = "4\r\nWrong size payload.\r\n0\r\n\r\n";
    char out[16] = "";
    size_t read = 0;
    CyneResult result;

    // EXECUTE
    result = str_to_body_chunked(input, out, &read);
    
    // ASSSERT
    TEST_ASSERT_EQUAL_MESSAGE(BODY_CHUNKS_PARSE_FAILED, result, "Result mismatch.");
    TEST_ASSERT_EQUAL_STRING_MESSAGE("", out, "Body mismatch.");
    TEST_ASSERT_EQUAL_MESSAGE(0, read, "Read bytes mismatch.");
}

void test_str_to_body_chunked_should_return_BODY_CHUNKS_PARSE_FAILED_when_framing_is_incorrect() {
    // SETUP
    char* input = "No size.\r\n0\r\n\r\n";
    char out[16] = "";
    size_t read = 0;
    CyneResult result;

    // EXECUTE
    result = str_to_body_chunked(input, out, &read);
    
    // ASSSERT
    TEST_ASSERT_EQUAL_MESSAGE(BODY_CHUNKS_PARSE_FAILED, result, "Result mismatch.");
    TEST_ASSERT_EQUAL_STRING_MESSAGE("", out, "Body mismatch.");
    TEST_ASSERT_EQUAL_MESSAGE(0, read, "Read bytes mismatch.");
}

void test_str_to_body_chunked_should_return_BODY_CHUNKS_PARSE_FAILED_when_body_chunk_ends_abruptly() {
    // SETUP
    char* input = "12\r\nSudden sto";
    char out[16] = "";
    size_t read = 0;
    CyneResult result;

    // EXECUTE
    result = str_to_body_chunked(input, out, &read);
    
    // ASSSERT
    TEST_ASSERT_EQUAL_MESSAGE(BODY_CHUNKS_PARSE_FAILED, result, "Result mismatch.");
    TEST_ASSERT_EQUAL_STRING_MESSAGE("", out, "Body mismatch.");
    TEST_ASSERT_EQUAL_MESSAGE(0, read, "Read bytes mismatch.");
}

void test_str_to_body_chunked_should_return_BODY_CHUNK_TOO_LONG_when_chunk_size_exceeds_15_digits() {
    // SETUP
    char* input = "1000000000000000\r\nSuper long chunk ...";
    char out[16] = "";
    size_t read = 0;
    CyneResult result;

    // EXECUTE
    result = str_to_body_chunked(input, out, &read);
    
    // ASSSERT
    TEST_ASSERT_EQUAL_MESSAGE(BODY_CHUNK_TOO_LONG, result, "Result mismatch.");
    TEST_ASSERT_EQUAL_STRING_MESSAGE("", out, "Body mismatch.");
    TEST_ASSERT_EQUAL_MESSAGE(0, read, "Read bytes mismatch.");
}

void test_get_headers_value() {
    // SETUP
    HttpHeader headers[] = {
        {.field="field1", .value="value1"},
        {.field="field2", .value="value2"}
    };
    char value[MAX_HEADER_VALUE_LEN] = "";

    // EXECUTE
    get_header_value(headers, 2, "field2", value);

    // ASSERTS
    TEST_ASSERT_EQUAL_STRING_MESSAGE("value2", value, "Header value mismatch.");
}

void test_get_headers_value_should_not_modify_out_value_when_field_is_not_found() {
    // SETUP
    HttpHeader headers[] = {
        {.field="field1", .value="value1"},
        {.field="field2", .value="value2"}
    };
    char value[MAX_HEADER_VALUE_LEN + 1] = "";

    // EXECUTE
    get_header_value(headers, 2, "field3", value);

    // ASSERTS
    TEST_ASSERT_EQUAL_STRING_MESSAGE("", value, "Header value mismatch.");
}

void test_get_headers_value_should_ignore_field_case() {
    // SETUP
    HttpHeader headers[] = {
        {.field="field1", .value="value1"},
        {.field="field2", .value="value2"}
    };
    char value[MAX_HEADER_VALUE_LEN + 1] = "";

    // EXECUTE
    get_header_value(headers, 2, "FIELD2", value);

    // ASSERTS
    TEST_ASSERT_EQUAL_STRING_MESSAGE("value2", value, "Header value mismatch.");
}

void test_HttpResponse_init() {
    // SETUP
    HttpResponse res;
    res.headers_count = -1;
    res.content_length = -1;
    res.reason_phrase[0] = 'a';
    res.body[0] = 'a';
    res.status_code = -1;

    // EXECUTE
    CyneResult result = HttpResponse_init(&res);
    
    // ASSERTS
    TEST_ASSERT_EQUAL_MESSAGE(SUCCESS, result, "Result mismatch.");
    TEST_ASSERT_EQUAL_MESSAGE(0, res.headers_count, "Headers count mismatch.");
    TEST_ASSERT_EQUAL_MESSAGE(0, res.content_length, "Content length mismatch.");
    TEST_ASSERT_EQUAL_MESSAGE(0, res.status_code, "Status code mismatch.");
    TEST_ASSERT_EQUAL_STRING_MESSAGE("", res.reason_phrase, "Reason phrase mismatch.");
    TEST_ASSERT_EQUAL_STRING_MESSAGE("", res.body, "Body mismatch.");
}

void test_HttpResponse_set_header_should_add_new_header_when_there_is_no_header() {
    // SETUP
    HttpResponse res;

    // EXECUTE
    HttpResponse_init(&res);
    CyneResult result = HttpResponse_set_header(&res, "Field1", "Value1");
    
    // ASSERTS
    TEST_ASSERT_EQUAL_MESSAGE(SUCCESS, result, "Result mismatch.");
    TEST_ASSERT_EQUAL_MESSAGE(1, res.headers_count, "Headers count mismatch.");
    TEST_ASSERT_EQUAL_STRING_MESSAGE("Field1", res.headers[0].field, "Header field mismatch.");
    TEST_ASSERT_EQUAL_STRING_MESSAGE("Value1", res.headers[0].value, "Header value mismatch.");
}

void test_HttpResponse_set_header_should_update_existing_header_when_field_already_exist() {
    // SETUP
    HttpResponse res;

    // EXECUTE
    HttpResponse_init(&res);
    HttpResponse_set_header(&res, "Field1", "Value1");
    CyneResult result = HttpResponse_set_header(&res, "Field1", "NewValue1");
    
    // ASSERTS
    TEST_ASSERT_EQUAL_MESSAGE(SUCCESS, result, "Result mismatch.");
    TEST_ASSERT_EQUAL_MESSAGE(1, res.headers_count, "Headers count mismatch.");
    TEST_ASSERT_EQUAL_STRING_MESSAGE("Field1", res.headers[0].field, "Header field mismatch.");
    TEST_ASSERT_EQUAL_STRING_MESSAGE("NewValue1", res.headers[0].value, "Header value mismatch.");
}

void test_HttpResponse_set_header_should_add_new_header_when_field_not_found() {
    // SETUP
    HttpResponse res;

    // EXECUTE
    HttpResponse_init(&res);
    HttpResponse_set_header(&res, "Field1", "Value1");
    CyneResult result = HttpResponse_set_header(&res, "Field2", "Value2");
    
    // ASSERTS
    TEST_ASSERT_EQUAL_MESSAGE(SUCCESS, result, "Result mismatch.");
    TEST_ASSERT_EQUAL_MESSAGE(2, res.headers_count, "Headers count mismatch.");
    TEST_ASSERT_EQUAL_STRING_MESSAGE("Field2", res.headers[1].field, "Header field mismatch.");
    TEST_ASSERT_EQUAL_STRING_MESSAGE("Value2", res.headers[1].value, "Header value mismatch.");
}

void test_HttpResponse_set_header_should_return_TOO_MANY_HEADERS_when_header_count_is_about_to_exceed_max_limit() {
    // SETUP
    HttpResponse res;
    char field[10];

    // EXECUTE
    HttpResponse_init(&res);
    for (size_t i=0; i<MAX_HEADER_COUNT; i++) {
        sprintf(field, "%zu", i);
        HttpResponse_set_header(&res, field, "0");
    }
    CyneResult result = HttpResponse_set_header(&res, "Field1", "Value1");
    
    // ASSERTS
    TEST_ASSERT_EQUAL_MESSAGE(TOO_MANY_HEADERS, result, "Result mismatch.");
}

void test_HttpResponse_set_header_should_return_HEADER_FIELD_TOO_LONG_when_field_length_exceeds_max_limit() {
    // SETUP
    HttpResponse res;
    char field[MAX_HEADER_FIELD_LEN + 2];
    for (size_t i=0; i<MAX_HEADER_FIELD_LEN + 1; i++) {
        field[i] = 'a';
    }
    field[MAX_HEADER_FIELD_LEN + 1] = 0;

    // EXECUTE
    HttpResponse_init(&res);
    CyneResult result = HttpResponse_set_header(&res, field, "Value1");
    
    // ASSERTS
    TEST_ASSERT_EQUAL_MESSAGE(HEADER_FIELD_TOO_LONG, result, "Result mismatch.");
}

void test_HttpResponse_set_header_should_return_HEADER_VALUE_TOO_LONG_when_value_length_exceeds_max_limit() {
    // SETUP
    HttpResponse res;
    char value[MAX_HEADER_VALUE_LEN + 2];
    for (size_t i=0; i<MAX_HEADER_VALUE_LEN + 1; i++) {
        value[i] = 'a';
    }
    value[MAX_HEADER_VALUE_LEN + 1] = 0;

    // EXECUTE
    HttpResponse_init(&res);
    CyneResult result = HttpResponse_set_header(&res, "Field1", value);
    
    // ASSERTS
    TEST_ASSERT_EQUAL_MESSAGE(HEADER_VALUE_TOO_LONG, result, "Result mismatch.");
}

void test_HttpResponse_set_header_should_return_HEADER_FIELD_CONTAINS_WHITESPACE_when_whitespace_is_found_in_field() {
    // SETUP
    HttpResponse res;

    // EXECUTE
    HttpResponse_init(&res);
    CyneResult result = HttpResponse_set_header(&res, "Content Type", "text/html");
    
    // ASSERTS
    TEST_ASSERT_EQUAL_MESSAGE(HEADER_FIELD_CONTAINS_WHITESPACE, result, "Result mismatch.");
}

void test_HttpResponse_to_str() {
    // SETUP
    HttpResponse res = {
        .body = "This is body.",
        .content_length = 13,
        .headers = {
            {.field="Field1", .value="Value1"},
            {.field="Field2", .value="Value2"}
        },
        .headers_count = 2,
        .reason_phrase = "OK",
        .status_code = 200
    };
    char out[MAX_RESPONSE_LEN + 1] = "";
    CyneResult result;

    // EXECUTE
    result = HttpResponse_to_str(&res, out);
    
    // ASSERTS
    TEST_ASSERT_EQUAL_MESSAGE(SUCCESS, result, "Result mismatch.");
    TEST_ASSERT_EQUAL_STRING_MESSAGE("HTTP/1.1 200 OK\r\nContent-Length: 13\r\nField1: Value1\r\nField2: Value2\r\n\r\nThis is body.", out, "Response string mismatch.");
}

void test_HttpResponse_redirect() {
    HttpResponse res;

    HttpResponse_init(&res);
    CyneResult result = HttpResponse_redirect(&res, "/new-page", false);

    TEST_ASSERT_EQUAL_MESSAGE(SUCCESS, result, "Result mismatch.");
    TEST_ASSERT_EQUAL_MESSAGE(302, res.status_code, "Status code mismatch.");
    TEST_ASSERT_EQUAL_MESSAGE(1, res.headers_count, "Headers count mismatch.");
    TEST_ASSERT_EQUAL_STRING_MESSAGE("Location", res.headers[0].field, "Header field mismatch.");
    TEST_ASSERT_EQUAL_STRING_MESSAGE("/new-page", res.headers[0].value, "Header value mismatch.");
    TEST_ASSERT_EQUAL_STRING_MESSAGE("Found", res.reason_phrase, "Reason phrase mismatch.");
}

void test_HttpResponse_redirect_should_set_status_code_307_when_preserve_request_is_true() {
    HttpResponse res;

    HttpResponse_init(&res);
    CyneResult result = HttpResponse_redirect(&res, "/new-page", true);

    TEST_ASSERT_EQUAL_MESSAGE(SUCCESS, result, "Result mismatch.");
    TEST_ASSERT_EQUAL_MESSAGE(307, res.status_code, "Status code mismatch.");
    TEST_ASSERT_EQUAL_MESSAGE(1, res.headers_count, "Headers count mismatch.");
    TEST_ASSERT_EQUAL_STRING_MESSAGE("Location", res.headers[0].field, "Header field mismatch.");
    TEST_ASSERT_EQUAL_STRING_MESSAGE("/new-page", res.headers[0].value, "Header value mismatch.");
    TEST_ASSERT_EQUAL_STRING_MESSAGE("Found", res.reason_phrase, "Reason phrase mismatch.");
}

void test_HttpResponse_permanent_redirect() {
    HttpResponse res;

    HttpResponse_init(&res);
    CyneResult result = HttpResponse_permanent_redirect(&res, "/new-page", false);

    TEST_ASSERT_EQUAL_MESSAGE(SUCCESS, result, "Result mismatch.");
    TEST_ASSERT_EQUAL_MESSAGE(301, res.status_code, "Status code mismatch.");
    TEST_ASSERT_EQUAL_MESSAGE(1, res.headers_count, "Headers count mismatch.");
    TEST_ASSERT_EQUAL_STRING_MESSAGE("Location", res.headers[0].field, "Header field mismatch.");
    TEST_ASSERT_EQUAL_STRING_MESSAGE("/new-page", res.headers[0].value, "Header value mismatch.");
    TEST_ASSERT_EQUAL_STRING_MESSAGE("Found", res.reason_phrase, "Reason phrase mismatch.");
}

void test_HttpResponse_permanenet_redirect_should_set_status_code_308_when_preserve_request_is_true() {
    HttpResponse res;

    HttpResponse_init(&res);
    CyneResult result = HttpResponse_permanent_redirect(&res, "/new-page", true);

    TEST_ASSERT_EQUAL_MESSAGE(SUCCESS, result, "Result mismatch.");
    TEST_ASSERT_EQUAL_MESSAGE(308, res.status_code, "Status code mismatch.");
    TEST_ASSERT_EQUAL_MESSAGE(1, res.headers_count, "Headers count mismatch.");
    TEST_ASSERT_EQUAL_STRING_MESSAGE("Location", res.headers[0].field, "Header field mismatch.");
    TEST_ASSERT_EQUAL_STRING_MESSAGE("/new-page", res.headers[0].value, "Header value mismatch.");
    TEST_ASSERT_EQUAL_STRING_MESSAGE("Found", res.reason_phrase, "Reason phrase mismatch.");
}

void test_HttpResponse_not_modified() {
    HttpResponse res;

    HttpResponse_init(&res);
    CyneResult result = HttpResponse_not_modified(&res);

    TEST_ASSERT_EQUAL_MESSAGE(SUCCESS, result, "Result mismatch.");
    TEST_ASSERT_EQUAL_MESSAGE(304, res.status_code, "Status code mismatch.");
    TEST_ASSERT_EQUAL_STRING_MESSAGE("Not Modified", res.reason_phrase, "Reason phrase mismatch.");
}

void test_HttpResponse_bad_request() {
    HttpResponse res;

    HttpResponse_init(&res);
    CyneResult result = HttpResponse_bad_request(&res);

    TEST_ASSERT_EQUAL_MESSAGE(SUCCESS, result, "Result mismatch.");
    TEST_ASSERT_EQUAL_MESSAGE(400, res.status_code, "Status code mismatch.");
    TEST_ASSERT_EQUAL_STRING_MESSAGE("Bad Request", res.reason_phrase, "Reason phrase mismatch.");
}

void test_HttpResponse_forbidden() {
    HttpResponse res;

    HttpResponse_init(&res);
    CyneResult result = HttpResponse_forbidden(&res);

    TEST_ASSERT_EQUAL_MESSAGE(SUCCESS, result, "Result mismatch.");
    TEST_ASSERT_EQUAL_MESSAGE(403, res.status_code, "Status code mismatch.");
    TEST_ASSERT_EQUAL_STRING_MESSAGE("Forbidden", res.reason_phrase, "Reason phrase mismatch.");
}

void test_HttpResponse_not_found() {
    HttpResponse res;

    HttpResponse_init(&res);
    CyneResult result = HttpResponse_not_found(&res);

    TEST_ASSERT_EQUAL_MESSAGE(SUCCESS, result, "Result mismatch.");
    TEST_ASSERT_EQUAL_MESSAGE(404, res.status_code, "Status code mismatch.");
    TEST_ASSERT_EQUAL_STRING_MESSAGE("Not Found", res.reason_phrase, "Reason phrase mismatch.");
}

void test_HttpResponse_not_allowed() {
    HttpResponse res;
    HttpMethod allowed[] = {GET, POST, PUT, DELETE};

    HttpResponse_init(&res);
    CyneResult result = HttpResponse_not_allowed(&res, allowed, 4);

    TEST_ASSERT_EQUAL_MESSAGE(SUCCESS, result, "Result mismatch.");
    TEST_ASSERT_EQUAL_MESSAGE(405, res.status_code, "Status code mismatch.");
    TEST_ASSERT_EQUAL_STRING_MESSAGE("Method Not Allowed", res.reason_phrase, "Reason phrase mismatch.");
    TEST_ASSERT_EQUAL_STRING_MESSAGE("Allow", res.headers[0].field, "Header field mismatch.");
    TEST_ASSERT_EQUAL_STRING_MESSAGE("GET, POST, PUT, DELETE", res.headers[0].value, "Header value mismatch.");
}

void test_HttpResponse_gone() {
    HttpResponse res;

    HttpResponse_init(&res);
    CyneResult result = HttpResponse_gone(&res);

    TEST_ASSERT_EQUAL_MESSAGE(SUCCESS, result, "Result mismatch.");
    TEST_ASSERT_EQUAL_MESSAGE(410, res.status_code, "Status code mismatch.");
    TEST_ASSERT_EQUAL_STRING_MESSAGE("Gone", res.reason_phrase, "Reason phrase mismatch.");
}

void test_HttpResponse_server_error() {
    HttpResponse res;

    HttpResponse_init(&res);
    CyneResult result = HttpResponse_server_error(&res);

    TEST_ASSERT_EQUAL_MESSAGE(SUCCESS, result, "Result mismatch.");
    TEST_ASSERT_EQUAL_MESSAGE(500, res.status_code, "Status code mismatch.");
    TEST_ASSERT_EQUAL_STRING_MESSAGE("Server Error", res.reason_phrase, "Reason phrase mismatch.");
}

void test_HttpResponse_text() {
    HttpResponse res;
    char* body = "<html>Hello, World!</html>";

    HttpResponse_init(&res);
    CyneResult result = HttpResponse_text(&res, body);

    TEST_ASSERT_EQUAL_MESSAGE(SUCCESS, result, "Result mismatch.");
    TEST_ASSERT_EQUAL_MESSAGE(200, res.status_code, "Status code mismatch.");
    TEST_ASSERT_EQUAL_STRING_MESSAGE("OK", res.reason_phrase, "Reason phrase mismatch.");
    TEST_ASSERT_EQUAL_STRING_MESSAGE(body, res.body, "Body mismatch.");
    TEST_ASSERT_EQUAL_MESSAGE(strlen(body), res.content_length, "Content length mismatch.");
}

void test_HttpResponse_text_should_return_BODY_TOO_LONG_when_text_length_exceeds_max_limit() {
    HttpResponse res;
    char body[MAX_BODY_LEN + 2];
    for (size_t i=0; i<MAX_BODY_LEN + 1; i++) {
        body[i] = 'a';
    }
    body[MAX_BODY_LEN + 1] = 0;

    HttpResponse_init(&res);
    CyneResult result = HttpResponse_text(&res, body);

    TEST_ASSERT_EQUAL_MESSAGE(BODY_TOO_LONG, result, "Result mismatch.");
}

void test_HttpResponse_json() {
    HttpResponse res;
    unsigned char json_buffer[1024];
    Arena a;
    Arena_init(&a , json_buffer, 1024);
    JsonValue* json = JOBJ(&a, 3, 
        JMEM(&a, "string", JSTR(&a, "This is string.")),
        JMEM(&a, "array", JARR(&a, 2, JINT(&a , 1), JFLT(&a, 2.0f))),
        JMEM(&a, "object", JOBJ(&a, 1,
            JMEM(&a, "inner_string", JSTR(&a, "Another string."))
        ))
    );
    char* expected = "{\"string\": \"This is string.\", \"array\": [1, 2.000000], \"object\": {\"inner_string\": \"Another string.\"}}";

    HttpResponse_init(&res);
    CyneResult result = HttpResponse_json(&res, json);

    TEST_ASSERT_EQUAL_MESSAGE(SUCCESS, result, "Result mismatch.");
    TEST_ASSERT_EQUAL_MESSAGE(200, res.status_code, "Status code mismatch.");
    TEST_ASSERT_EQUAL_STRING_MESSAGE("OK", res.reason_phrase, "Reason phrase mismatch.");
    TEST_ASSERT_EQUAL_STRING_MESSAGE(expected, res.body, "Body mismatch.");
    TEST_ASSERT_EQUAL_MESSAGE(strlen(expected), res.content_length, "Content length mismatch.");
}

void test_HttpResponse_json_should_return_BODY_TOO_LONG_when_json_payload_length_exceeds_max_limit() {
    HttpResponse res;
    char long_string[MAX_BODY_LEN + 2];
    for (size_t i=0; i<MAX_BODY_LEN + 1; i++) {
        long_string[i] = 'a';
    }
    long_string[MAX_BODY_LEN + 1] = 0;
    unsigned char json_buffer[MAX_BODY_LEN * 2];
    Arena a;
    Arena_init(&a , json_buffer, MAX_BODY_LEN * 2);
    JsonValue* json = JOBJ(&a, 1, 
        JMEM(&a, "string", JSTR(&a, long_string)),
    );

    HttpResponse_init(&res);
    CyneResult result = HttpResponse_json(&res, json);

    TEST_ASSERT_EQUAL_MESSAGE(BODY_TOO_LONG, result, "Result mismatch.");
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_HttpRequest_from_str);
    RUN_TEST(test_HttpRequest_from_str_should_return_METHOD_PARSE_FAILED_when_method_is_invalid);
    RUN_TEST(test_HttpRequest_from_str_should_return_REQUEST_TARGET_PARSE_FAILED_when_request_target_is_invalid);
    RUN_TEST(test_HttpRequest_from_str_should_return_START_LINE_CRLF_NOT_FOUND_when_start_line_crlf_not_found);
    RUN_TEST(test_HttpRequest_from_str_should_return_HEADER_FIELD_CONTAINS_WHITESPACE_when_field_has_whitespace);
    RUN_TEST(test_HttpRequest_from_str_should_return_HEADER_FIELD_DELIMITER_NOT_FOUND_when_field_delimiter_not_found);
    RUN_TEST(test_HttpRequest_from_str_should_return_HEADER_CRLF_NOT_FOUND_when_header_crlf_not_found);
    RUN_TEST(test_HttpRequest_from_str_should_return_BODY_LEN_MISMATCH_when_actual_body_size_is_shorter_than_expected);
    RUN_TEST(test_HttpRequest_from_str_should_concat_values_when_duplicate_fields_are_found);
    RUN_TEST(test_HttpRequest_from_str_should_parse_chunked_body_correctly);
    RUN_TEST(test_HttpRequest_from_str_should_return_BODY_CHUNKS_PARSE_FAILED_when_chunk_size_is_mismatched);
    RUN_TEST(test_HttpRequest_from_str_should_return_BODY_CHUNKS_PARSE_FAILED_when_framing_is_incorrect);
    RUN_TEST(test_HttpRequest_from_str_should_return_BODY_CHUNKS_PARSE_FAILED_when_ending_chunk_not_found);
    RUN_TEST(test_HttpRequest_from_str_should_return_BODY_CHUNKS_PARSE_FAILED_when_body_chunk_ends_abruptly);
    RUN_TEST(test_HttpRequest_from_str_should_return_BODY_CHUNK_TOO_LONG_when_chunk_size_exceeds_15_digits);
    RUN_TEST(test_HttpRequest_from_str_should_handle_empty_body_requests);

    RUN_TEST(test_str_to_start_line);
    RUN_TEST(test_str_to_start_line_should_return_METHOD_PARSE_FAILED_when_method_is_invalid);
    RUN_TEST(test_str_to_start_line_should_return_START_LINE_CRLF_NOT_FOUND_when_start_line_crlf_not_found);
    
    RUN_TEST(test_str_to_headers);
    RUN_TEST(test_str_to_headers_should_return_HEADER_FIELD_CONTAINS_WHITESPACE_when_field_has_whitespace);
    RUN_TEST(test_str_to_headers_should_return_HEADER_FIELD_DELIMITER_NOT_FOUND_when_field_delimiter_not_found);
    RUN_TEST(test_HttpRequest_from_str_should_return_HEADER_CRLF_NOT_FOUND_when_header_crlf_not_found);
    RUN_TEST(test_str_to_headers_should_concat_values_when_duplicate_fields_are_found);

    RUN_TEST(test_str_to_body);
    RUN_TEST(test_str_to_body_return_BODY_LEN_MISMATCH_when_actual_body_size_is_shorter_than_expected);
    RUN_TEST(test_str_to_body_chunked);
    RUN_TEST(test_str_to_body_chunked_should_return_BODY_CHUNKS_PARSE_FAILED_when_chunk_size_is_mismatched);
    RUN_TEST(test_str_to_body_chunked_should_return_BODY_CHUNKS_PARSE_FAILED_when_framing_is_incorrect);
    RUN_TEST(test_str_to_body_chunked_should_return_BODY_CHUNKS_PARSE_FAILED_when_body_chunk_ends_abruptly);
    RUN_TEST(test_str_to_body_chunked_should_return_BODY_CHUNK_TOO_LONG_when_chunk_size_exceeds_15_digits);

    RUN_TEST(test_get_headers_value);
    RUN_TEST(test_get_headers_value_should_not_modify_out_value_when_field_is_not_found);
    RUN_TEST(test_get_headers_value_should_ignore_field_case);

    RUN_TEST(test_HttpResponse_init);
    RUN_TEST(test_HttpResponse_set_header_should_add_new_header_when_there_is_no_header);
    RUN_TEST(test_HttpResponse_set_header_should_update_existing_header_when_field_already_exist);
    RUN_TEST(test_HttpResponse_set_header_should_add_new_header_when_field_not_found);
    RUN_TEST(test_HttpResponse_set_header_should_return_TOO_MANY_HEADERS_when_header_count_is_about_to_exceed_max_limit);
    RUN_TEST(test_HttpResponse_set_header_should_return_HEADER_FIELD_TOO_LONG_when_field_length_exceeds_max_limit);
    RUN_TEST(test_HttpResponse_set_header_should_return_HEADER_VALUE_TOO_LONG_when_value_length_exceeds_max_limit);
    RUN_TEST(test_HttpResponse_set_header_should_return_HEADER_FIELD_CONTAINS_WHITESPACE_when_whitespace_is_found_in_field);
    RUN_TEST(test_HttpResponse_to_str);
    RUN_TEST(test_HttpResponse_redirect);
    RUN_TEST(test_HttpResponse_redirect_should_set_status_code_307_when_preserve_request_is_true);
    RUN_TEST(test_HttpResponse_permanent_redirect);
    RUN_TEST(test_HttpResponse_permanenet_redirect_should_set_status_code_308_when_preserve_request_is_true);
    RUN_TEST(test_HttpResponse_not_modified);
    RUN_TEST(test_HttpResponse_bad_request);
    RUN_TEST(test_HttpResponse_forbidden);
    RUN_TEST(test_HttpResponse_not_found);
    RUN_TEST(test_HttpResponse_not_allowed);
    RUN_TEST(test_HttpResponse_gone);
    RUN_TEST(test_HttpResponse_server_error);
    RUN_TEST(test_HttpResponse_text);
    RUN_TEST(test_HttpResponse_text_should_return_BODY_TOO_LONG_when_text_length_exceeds_max_limit);
    RUN_TEST(test_HttpResponse_json);
    RUN_TEST(test_HttpResponse_json_should_return_BODY_TOO_LONG_when_json_payload_length_exceeds_max_limit);

    return UNITY_END();
}