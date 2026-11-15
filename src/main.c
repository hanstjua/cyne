#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include "configs.h"
#include "error.h"
#include "logger.h"
#include "server.h"
#include "socket.h"
#include "message.h"
#include "url.h"

#ifdef __linux__
#include "poller_linux.c"
#else
#include "poller_darwin.c"
#endif

#define SLEEP(sec) do {              \
    struct timespec req;             \
    req.tv_sec = 0;                  \
    req.tv_nsec = (sec) * 1000000000L; \
    nanosleep(&req, NULL);           \
} while (0)                          \


typedef struct ContextArrayItem {
    RequestContext request_context;
    int sockfd;
    bool occupied;
} ContextArrayItem;

typedef struct ContextArray {
    ContextArrayItem items[MAX_CONCURRENT_REQUESTS];
    size_t available;
} ContextArray;

bool ContextArray_init(ContextArray* c) {
    for (size_t i=0; i<MAX_CONCURRENT_REQUESTS; i++) {
        c->items[i].occupied = false;
    }
    c->available = MAX_CONCURRENT_REQUESTS;
    return true;
}

size_t ContextArray_allocate_for(ContextArray* c, int sockfd) {
    if (c->available == 0) {
        return -1;
    }

    size_t start_index = sockfd % MAX_CONCURRENT_REQUESTS;
    for (size_t i=0; i<MAX_CONCURRENT_REQUESTS; i++) {
        size_t cur_index = (start_index + i) % MAX_CONCURRENT_REQUESTS;
        if (c->items[cur_index].sockfd == sockfd) {
            return cur_index;
        }
        if (c->items[cur_index].occupied == false) {
            c->items[cur_index].sockfd = sockfd;
            c->items[cur_index].occupied = true;
            c->available -= 1;
            return cur_index;
        }
    }

    return -1;
}

bool ContextArray_deallocate(ContextArray* c, int sockfd) {
    size_t start_index = sockfd % MAX_CONCURRENT_REQUESTS;
    for (size_t i=0; i<MAX_CONCURRENT_REQUESTS; i++) {
        size_t cur_index = (start_index + i) % MAX_CONCURRENT_REQUESTS;
        if (c->items[cur_index].occupied && c->items[start_index + i].sockfd == sockfd) {
            c->items[cur_index].occupied = false;
            c->available += 1;
            return true;
        }
    }

    return false;
}

static char request_buffer[MAX_REQUEST_SIZE + 1];
static Event events[MAX_EVENT_COUNT];
static ContextArray context_array;

int main(int argc, char* argv[]) {
    int g_clients_count = 0;
    int opt, port = 8812;
    char host[15] = "127.0.0.1";
    char* port_end;
    Socket socket, client;
    size_t request_len, temp_request_len;
    Poller poller;
    size_t event_count = 0;
    CyneResult result;
    char error_name[128];

    ContextArray_init(&context_array);

    while ((opt = getopt(argc, argv, "h:p:")) != -1) {
        switch (opt) {
            case 'h':
            snprintf(host, 15, "%s", optarg);
            break;

            case 'p':
            port = strtol(optarg, &port_end, 10);
            if (*port_end != 0) {
                port = -1;
                ERROR("invalid port: %s", optarg);
            }
            break;
            
            default:
            break;
        }
    }

    if (Socket_init(&socket) != SUCCESS) {
        ERROR("Failed to initialize socket.");
        return 1;
    }
    if (Socket_bind(&socket, host, port) != SUCCESS) {
        ERROR("Failed to bind to %s:%d", host, port);
        return 1;
    }
    if (Socket_listen(&socket) != SUCCESS) {
        ERROR("Failed to listen.");
        return 1;
    }

    INFO("Running on http://%s:%d ...", host, port);

    if (Poller_init(&poller) != SUCCESS || Poller_monitor(&poller, socket.fd, READ_EV, NULL) != SUCCESS) {
        ERROR("Failed to initialize poller.");
        return 1;
    }

    if (before_loop() != SUCCESS) {
        ERROR("Error when executing before_loop().");
        return 1;
    }

    for (;;) {
        if (Poller_wait(&poller, events, &event_count) != SUCCESS || event_count < 1) {
            ERROR("Poller wait error ...");
            continue;
        }

        for (size_t i=0; i<event_count; i++) {
            if ((events[i].flags & READ_EV)) {
                if (events[i].fd == socket.fd && g_clients_count < MAX_CONNECTIONS) {
                    if (Socket_accept(&socket, &client) != SUCCESS) {
                        continue;
                    }
                    Poller_add(&poller, client.fd);

                } else {
                    client.fd = events[i].fd;
                    size_t index = ContextArray_allocate_for(&context_array, client.fd);

                    if (Socket_recv(&client, MAX_REQUEST_SIZE, request_buffer, &request_len) != SUCCESS) {
                        ERROR("Failed to read from socket.");
                        HttpResponse_server_error(&context_array.items[index].request_context.res);
                        Poller_monitor(&poller, client.fd, WRITE_EV, (void*)(context_array.items + index));
                        continue;
                    }
                    
                    time_t start_time = time(NULL);
                    HttpRequest* cur_req = &(context_array.items[index].request_context.req);
                    result = HttpRequest_from_str(request_buffer, cur_req); 
                    while (result != SUCCESS && time(NULL) - start_time < REQUEST_RECEIVE_TIMEOUT_SEC) {
                        INFO("Failed to parse request with error %s. Retrying ...", e2s(result, 128, error_name));

                        SLEEP(0.001);

                        result = Socket_recv(&client, MAX_REQUEST_SIZE - request_len, request_buffer + request_len, &temp_request_len);
                        if (result != SUCCESS && result != NO_DATA_FROM_SOCKET) { // TODO: handle eagain
                            break;
                        }
                        request_len += temp_request_len;

                        result = HttpRequest_from_str(request_buffer, cur_req); 
                    }

                    if (result != SUCCESS) {
                        ERROR("Failed to parse request with error %s:\nrequest target: %s\nbody: %s", e2s(result, 128, error_name), cur_req->request_target, cur_req->body);
                        HttpResponse_server_error(&context_array.items[index].request_context.res);
                        Poller_monitor(&poller, client.fd, WRITE_EV, (void*)(context_array.items + index));
                        continue;
                    }

                    QueryParamParser query_parser;
                    for (size_t i=0; i<MAX_QUERY_PARAM_COUNT; i++) {
                        context_array.items[index].request_context.params[i].name[0] = 0;
                        context_array.items[index].request_context.params[i].value[0] = 0;
                    }

                    result = QueryParamParser_init(&query_parser, context_array.items[index].request_context.params, MAX_QUERY_PARAM_COUNT);
                    if (result != SUCCESS) {
                        ERROR("Query parser init failed: %s", e2s(result, 128, error_name));
                        HttpResponse_bad_request(&context_array.items[index].request_context.res);
                        Poller_monitor(&poller, client.fd, WRITE_EV, (void*)(context_array.items + index));
                        continue;
                    }

                    result = QueryParamParser_parse(&query_parser, context_array.items[index].request_context.req.request_target, NULL);
                    if (result != SUCCESS) {
                        ERROR("Query parser init failed: %s", e2s(result, 128, error_name));
                        HttpResponse_bad_request(&context_array.items[index].request_context.res);
                        Poller_monitor(&poller, client.fd, WRITE_EV, (void*)(context_array.items + index));
                        continue;
                    }

                    result = handle_request((RequestContext*)(context_array.items + index));
                    if (result != SUCCESS) {
                        ERROR("Failed to handle request with error %s:\nrequest target: %s\nbody: %s", e2s(result, 128, error_name), cur_req->request_target, cur_req->body);
                        HttpResponse_server_error(&context_array.items[index].request_context.res);
                    }

                    Poller_monitor(&poller, client.fd, WRITE_EV, (void*)(context_array.items + index));
                }
            } else if (events[i].flags & WRITE_EV) {
                // TODO: handle error
                INFO("sending response ...");
                char payload[MAX_RESPONSE_LEN + 1];
                for (int i=0; i<MAX_RESPONSE_LEN + 1; i++) {
                    payload[i] = 0;
                }

                ContextArrayItem* cai = events[i].userdata;
                HttpResponse_to_str(&(cai->request_context.res), payload);
                
                Socket client;
                client.fd = events[i].fd;
                Socket_send(&client, payload, strlen(payload));
                Poller_unmonitor(&poller, events[i].fd, WRITE_EV, NULL);
                ContextArray_deallocate(&context_array, events[i].fd);

            } else {
                // TODO: handle error
                Poller_drop(&poller, events[i].fd);
                ContextArray_deallocate(&context_array, events[i].fd);

            }
        }
    }

    if (after_loop() != SUCCESS) {
        ERROR("Error when executing after_loop().");
    }

    return 0;
}