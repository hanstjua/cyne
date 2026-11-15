#include <sys/event.h>
#include <sys/time.h>
#include <unistd.h>
#include "configs.h"
#include "error.h"
#include "logger.h"
#include "poller.h"

typedef struct Poller {
    size_t kq;
} Poller;

CyneResult Poller_init(Poller* out) {
    int result = kqueue();
    if (result == -1) {
        return POLLER_CREATION_FAILED;
    }
    out->kq = result;

    return SUCCESS;
}

CyneResult Poller_add(Poller* p, int fd) {
    struct kevent ev;
    int result;

    EV_SET(&ev, fd, EVFILT_READ, EV_ADD, 0, 0, NULL);

    result = kevent(p->kq, &ev, 1, NULL, 0, NULL);
    if (result == -1) {
        return SOCKET_ADD_FAILED;
    }

    return SUCCESS;
}

CyneResult Poller_monitor(Poller* p, int fd, int event_flags, void* userdata) {
    struct kevent ev[2];
    char ev_count = 0;
    int result;
    DEBUG("flags: %d %d %d", event_flags, event_flags & READ_EV, event_flags & WRITE_EV);

    if (event_flags & READ_EV) {
        DEBUG("set read");
        EV_SET(ev + ev_count, fd, EVFILT_READ, EV_ADD, 0, 0, userdata);
        ev_count++;
    }

    if (event_flags & WRITE_EV) {
        DEBUG("set write");
        EV_SET(ev + ev_count, fd, EVFILT_WRITE, EV_ADD, 0, 0, userdata);
        ev_count++;
    }

    result = kevent(p->kq, ev, ev_count, NULL, 0, NULL);
    if (result == -1) {
        return SOCKET_ADD_FAILED;
    }
    DEBUG("events count: %d", ev_count);
    return SUCCESS;
}

CyneResult Poller_unmonitor(Poller* p, int fd, int event_flags, void* userdata) {
    struct kevent ev[2];
    char ev_count = 0;
    int result;

    if (event_flags & READ_EV) {
        EV_SET(ev + ev_count, fd, EVFILT_READ, EV_DELETE, 0, 0, userdata);
        ev_count++;
    }

    if (event_flags & WRITE_EV) {
        EV_SET(ev + ev_count, fd, EVFILT_WRITE, EV_DELETE, 0, 0, userdata);
        ev_count++;
    }

    result = kevent(p->kq, ev, ev_count, NULL, 0, NULL);
    if (result == -1) {
        return SOCKET_ADD_FAILED;
    }
    return SUCCESS;
}

CyneResult Poller_wait(Poller* p, Event* out_events, size_t* out_count) {
    struct kevent evs[MAX_EVENT_COUNT];

    int available = kevent(p->kq, NULL, 0, evs, MAX_EVENT_COUNT, NULL);
    if (available < 0) {
        ERROR("Got poller error");
        return SOCKET_WAIT_FAILED;
    }

    *out_count = available;

    for (int i=0; i<available; i++) {
        if (evs[i].flags & EV_EOF) {
            DEBUG("Client (%lu) closed connection.", evs[i].ident);
            Poller_unmonitor(p, evs[i].ident, READ_EV | WRITE_EV, NULL);
            *out_count -= 1;
            continue;
        }

        out_events[i].userdata = evs[i].udata;
        out_events[i].flags = 0;
        out_events[i].fd = evs[i].ident;

        if (evs[i].filter == EVFILT_READ) {
            out_events[i].flags |= READ_EV;
        }

        if (evs[i].filter == EVFILT_WRITE) {
            out_events[i].flags |= WRITE_EV;
        }

        if (evs[i].flags & EV_ERROR) {
            out_events[i].flags |= ERROR_EV;
        }
        DEBUG("Event: %d %d", out_events[i].fd, out_events[i].flags);
    }

    return SUCCESS;
}

CyneResult Poller_drop(Poller* p, int fd) {
    DEBUG("Dropping fd %d for poller %ld ...", fd, (long)p);
    return SUCCESS;

}