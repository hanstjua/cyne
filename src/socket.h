#ifndef CYNE_SOCKET_H
#define CYNE_SOCKET_H

#include <stdint.h>
#include <stdio.h>
#include "error.h"

typedef struct Socket {
    int fd;
} Socket;

CyneResult Socket_init(Socket* out);
CyneResult Socket_bind(Socket* s, const char* host, uint16_t port);
CyneResult Socket_accept(Socket* s, Socket* out);
CyneResult Socket_listen(Socket* s);
CyneResult Socket_recv(Socket* s, size_t max_len, char* out_buf, size_t* out_len);
CyneResult Socket_send(Socket* s, char* payload, size_t payload_len);

/* End of header */
#endif