CC := gcc
CFLAGS := -Wall -Wextra -Werror -g
UNITY := libs/unity/src/unity.c
BUILD_DIR := build

test_message: test/test_message.c src/message.c src/message.h src/json.h src/logger.h
	mkdir -p $(BUILD_DIR)
	rm -f $(BUILD_DIR)/test_message
	$(CC) $(CFLAGS) test/test_message.c src/message.c src/json.c $(UNITY) -o $(BUILD_DIR)/test_message
	./$(BUILD_DIR)/test_message

test_url: test/test_url.c src/url.c src/url.h src/logger.h
	mkdir -p $(BUILD_DIR)
	rm -f $(BUILD_DIR)/test_url
	$(CC) $(CFLAGS) test/test_url.c src/url.c $(UNITY) -o $(BUILD_DIR)/test_url
	./$(BUILD_DIR)/test_url

test_request_context: test/test_request_context.c src/request_context.c src/url.c src/url.h src/json.c src/json.h src/arena.h src/message.c src/message.h src/logger.h
	mkdir -p $(BUILD_DIR)
	rm -f $(BUILD_DIR)/test_request_context
	$(CC) $(CFLAGS) test/test_request_context.c src/request_context.c src/url.c src/json.c src/message.c $(UNITY) -o $(BUILD_DIR)/test_request_context
	./$(BUILD_DIR)/test_request_context

test_json: test/test_json.c src/json.c src/json.h src/arena.h src/logger.h
	mkdir -p $(BUILD_DIR)
	rm -f $(BUILD_DIR)/test_json
	$(CC) $(CFLAGS) test/test_json.c src/json.c $(UNITY) -o $(BUILD_DIR)/test_json
	./$(BUILD_DIR)/test_json

basic_server: src/*.c src/*.h example/basic/*.c
	mkdir -p $(BUILD_DIR)
	rm -f $(BUILD_DIR)/basic_server
	$(CC) $(CFLAGS) src/main.c example/basic/basic_server.c src/socket.c src/url.c src/message.c src/request_context.c src/json.c -o $(BUILD_DIR)/basic_server
	./$(BUILD_DIR)/basic_server
