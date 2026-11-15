#ifndef CYNE_SERVER_H
#define CYNE_SERVER_H

#include "error.h"
#include "poller.h"
#include "request_context.h"
#include "socket.h"

CyneResult before_loop();
CyneResult handle_request(RequestContext* request_context);
CyneResult after_loop();

/* End of header */
#endif