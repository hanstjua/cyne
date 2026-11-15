#ifndef CYNE_POLLER_H
#define CYNE_POLLER_H

#include <stdint.h>
#include <stdio.h>
#include "error.h"

typedef enum EventFlags {
    READ_EV = 1,
    WRITE_EV = 2,
    ERROR_EV = 4
} EventFlags;

typedef struct Event {
    void* userdata;
    int fd;
    char flags;   
} Event;

typedef struct Poller Poller;

CyneResult Poller_init(Poller* out);
CyneResult Poller_add(Poller* p, int fd);
CyneResult Poller_monitor(Poller* p, int fd, int event_flags, void* userdata);
CyneResult Poller_unmonitor(Poller* p, int fd, int event_flags, void* userdata);
CyneResult Poller_wait(Poller* p, Event* out_events, size_t* out_count);
CyneResult Poller_drop(Poller* p, int fd);

#endif