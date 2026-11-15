#include <arpa/inet.h>
#include <fcntl.h>
#include <string.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include "socket.h"

#define SOCKET_BACKLOG 512

CyneResult Socket_init(Socket* s) {
    s->fd = socket(AF_INET, SOCK_STREAM, 0);
    if (s->fd == -1) {
        // TODO: return error
    }

    // allow address reuse
    int yes = 1;
    setsockopt(s->fd, SOL_SOCKET, SO_REUSEADDR, (void*)&yes, sizeof(yes));
    
    // set socket to nonblock
    int flags = fcntl(s->fd, F_GETFL, 0);
    fcntl(s->fd, F_SETFL, flags | O_NONBLOCK);

    return SUCCESS;
}

CyneResult Socket_bind(Socket* s, const char* host, uint16_t port) {
    struct sockaddr_in addr;

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);

    if (inet_pton(AF_INET, host, &addr.sin_addr) != 1) {
        // TODO: handle invalid address
    }

    if (bind(s->fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        // TODO: return error
    }
    return SUCCESS;
}

CyneResult Socket_accept(Socket* s, Socket* out) {
    out->fd = accept(s->fd, NULL, NULL);
    if (out->fd == -1) {
        // TODO: return error
        return SUCCESS;
    }

    // set socket to nonblock
    int flags = fcntl(out->fd, F_GETFL, 0);
    fcntl(out->fd, F_SETFL, flags | O_NONBLOCK);

    return SUCCESS;
}

CyneResult Socket_listen(Socket* s) {
    if (listen(s->fd, SOCKET_BACKLOG) == -1) {
        // TODO: return error
    }
    return SUCCESS;
}

CyneResult Socket_recv(Socket* s, size_t max_len, char* out_buf, size_t* out_len) {
    size_t read = recv(s->fd, out_buf, max_len, 0);
    if (read == (size_t)-1) {
        return NO_DATA_FROM_SOCKET;
    }
    out_buf[read] = 0;
    *out_len = read;
    return SUCCESS;
}

CyneResult Socket_send(Socket* s, char* payload, size_t payload_len) {
    size_t wrote = send(s->fd, payload, payload_len, MSG_DONTWAIT);
    if (wrote == (size_t)-1) {
        // TODO: return error
    }
    return SUCCESS;
}