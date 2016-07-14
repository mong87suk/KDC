#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

#include "DBLinkedList.h"
#include "looper.h"
#include "utils.h"
#include "m_boolean.h"

struct _Looper {
    DList *watcher_list;
    DList *timer_list;
    unsigned int watcher_last_id;
    unsigned int timer_last_id;
    int state;
};

struct _Watcher {
    int fd;
    unsigned int id;
    void *user_data;
    void (*handle_events)(int fd, void *user_data, unsigned int id, int revents);
    short events;
};

struct _Timer {
    unsigned int id;
    void *user_data;
    BOOLEAN (*callback)(void *user_data, unsigned int id);
    unsigned int interval;
    long expiration;
};

//This struct is used to get timeout.
struct _TimerData {
    Looper *looper;
    long cur_time;
    long timeout;
};

//This struct is used to get pollfds
struct _FDData {
    struct pollfd *fds;
    int len; 
};

static long looper_get_monotonic_time(void) {
    long cur_time;
    struct timespec ts;

    if (clock_gettime(CLOCK_MONOTONIC, &ts) < 0) {
        LOGD("Failed to get time\n");
        return -1;
    }

    cur_time = (ts.tv_sec * 1000) + (ts.tv_nsec / 1000000);
    return cur_time;
}

static void looper_free_timer(void *timer) {
    if (!timer) {
        LOGD("There is nothing to point timer\n");
        return;
    }
    free(timer);
}

void static looper_remove_timer(Looper *looper, Timer *timer) {
    DList *list;

    list = looper->timer_list;

    looper->timer_list =  d_list_remove_with_data(list, timer, looper_free_timer);
}

static void looper_handle_event(void *data, void *user_data) {
    short revents;
    unsigned int looper_event;

    Watcher *watcher;
    struct pollfd *pfd;

    watcher = (Watcher *) data;
    pfd = (struct pollfd *) user_data;
    revents = pfd->revents;

    looper_event = 0;

    if (revents & POLLIN) {
        looper_event |= LOOPER_IN_EVENT;
    }

    if (revents & POLLOUT) {
        looper_event |= LOOPER_HUP_EVENT;
    }

    if (revents & POLLHUP) {
        looper_event |= LOOPER_HUP_EVENT;
    }

    if ((watcher->events & revents) && (watcher->fd == pfd->fd)) {
        watcher->handle_events(watcher->fd, watcher->user_data, watcher->id, looper_event);
    }    
}

static void looper_watcher_dispatch(DList *list, struct pollfd fd) {
    d_list_foreach(list, looper_handle_event, &fd);
}

static void looper_timer_dispatch(void *data, void *user_data) {
    long timeout;
    long cur_time;
    long cmp_timeout;

    Timer *timer;
    BOOLEAN result;
    struct _TimerData *timer_data;

    timer_data = (struct _TimerData *) user_data;
    cur_time = timer_data->cur_time;
    cmp_timeout = timer_data->timeout;
    timer = (Timer *) data;

    timeout = timer->expiration - cur_time;
    if (timeout <= 0) { 
        timeout = 0;
        result = timer->callback(timer->user_data, timer->id);
        if (result == FALSE) {
            looper_remove_timer(timer_data->looper, timer);
        } else if (result == TRUE) {
            timer->expiration = cur_time + timer->interval;
        }
    }

    if (timeout < cmp_timeout) {
        timer_data->timeout = timeout;
    }
}

static long looper_get_timeout(Looper *looper) {
    long cur_time;
    long timeout;

    BOOLEAN result;
    DList *next;
    DList *list;
    Timer *timer;
    struct _TimerData timer_data;

    list = looper->timer_list;
    if (!list) {
        return 0;
    }

    next = d_list_next(list);
    timeout = 0;
    timer_data.looper = looper;
    
    cur_time = looper_get_monotonic_time();
    if (cur_time < 0) {
        LOGD("Failed to get current time\n");
    }
    
    timer = (Timer *) d_list_get_data(list);
    if (timer) {
        timeout = timer->expiration - cur_time;
        if (timeout <= 0) {
            result = timer->callback(timer->user_data, timer->id);
            timeout = 0;
            if (result == FALSE) {
                looper_remove_timer(looper, timer);
            } else if (result == TRUE) {
                timer->expiration = cur_time + timer->interval;
            }
        }
    }
    
    if (next) {
        timer_data.cur_time = cur_time;
        timer_data.timeout = timeout;
        d_list_foreach(next, looper_timer_dispatch, &timer_data);
        timeout = timer_data.timeout;
    }

    return timeout;
}

static void looper_match_fd(void *data, void *user_data) {
    Watcher *watcher;
    int *count_data;

    count_data = (int *) user_data;
    watcher = (Watcher *) data;

    if (count_data[0] != watcher->fd) {
        count_data[0] = watcher->fd;
        count_data[1] += 1;
    } 
}

static int looper_get_fd_count(DList *watcher_list) {
    // It it a value to save a fd and count;
    // count_data[0] to save fd of watcher
    // count_data[1] to fd count
    int count_data[2];
    
    memset(count_data, 0, sizeof(count_data));
    d_list_foreach(watcher_list, looper_match_fd, count_data);
    
    return count_data[1];
}


static void looper_match_fd_with_pollfd(void *data, void *user_data) {
    int i;

    struct pollfd *fds;
    struct _FDData *fd_data;
    Watcher *watcher;

    watcher = (Watcher *) data;
    fd_data = (struct _FDData *) user_data;

    fds = fd_data->fds;

    for (i = 0; i < fd_data->len; i++) {
        if (fds[i].fd == watcher->fd) {
            fds[i].events |= watcher->events;
            break;
        } else {
            fds[i].fd = watcher->fd;
            fds[i].events = watcher->events;
            break;           
        }
    }
}

/**
  * looper_get_fds:
  * @looper: a pointer to Looper which includes watch_list
  * @fds   : a pointer to pollfd
  *
  * Set events vaule of pollfd
  **/
static void looper_get_fds(DList *watcher_list, struct pollfd *fds, int n_fds) {
    int i;

    struct _FDData fd_data;

    for (i = 0; i < n_fds; i++) {
        fds[i].fd = 0;
    }

    fd_data.fds = fds;
    fd_data.len = n_fds;
    d_list_foreach(watcher_list, looper_match_fd_with_pollfd, &fd_data);
}

static int looper_match_watcher_with_id(void *data1, void *data2) {
    Watcher *watcher = (Watcher *) data1;
    unsigned int id = *((unsigned int *) data2);

    if (watcher->id == id) {
        return 1;
    } else {
        return 0;
    }
}

static int looper_match_timer(void *data1, void *data2) {
    Timer *timer = (Timer *) data1;
    unsigned int id = *((unsigned int *) data2);

    if (timer->id == id) {
        return 1;
    } else {
        return 0;
    }
}

static void looper_destroy_watcher(void *data) {
    Watcher *watcher;

    watcher = (Watcher *) data;
    free(watcher);
}

Looper *new_looper(void) {
    Looper *looper;
    looper = (Looper*) malloc(sizeof(Looper));
    looper->watcher_last_id = 0;
    looper->timer_last_id = 0;
    looper->watcher_list = NULL;
    looper->state = 0;
    return looper;
}

void looper_stop(Looper *looper) {
    if (!looper) {
        LOGD("There is nothing to pointer the Looper\n");
        return;
    }
    looper->state = 0;
    return;
}

void looper_remove_watcher(Looper *looper, unsigned int id) {
    if (!looper && !looper->watcher_list) {
        LOGD("Can't remove Wathcer\n");
        return;
    }
    looper->watcher_list = d_list_remove_with_user_data(looper->watcher_list, &id, looper_match_watcher_with_id, looper_destroy_watcher);
}

/**
  * looper run:
  * @looper: Looper is struct which includes wathcer_list and state
  *
  * Run stops when state of looper is 0
  **/
int looper_run(Looper *looper) {
    int n_revents;
    int i;
    int n_watcher, n_timer, n_fds;
    long timeout;

    n_fds = 0;

    if (!looper) {
        LOGD("There is nothing to pointer the Looper\n");
        return 0;
    }

    if (!(d_list_length(looper->watcher_list)) && !(d_list_length(looper->timer_list))) {
        LOGD("There is no Watcher and Timer\n");
        return 0;
    }

    looper->state = 1;

    while (looper->state) {
        n_watcher = d_list_length(looper->watcher_list);
        n_timer = d_list_length(looper->timer_list);
        if ((n_watcher <= 0) && (n_timer <= 0)) {
            LOGD("There is no Watcher\n");
            return 0;
        }
        timeout = looper_get_timeout(looper);
        n_fds = looper_get_fd_count(looper->watcher_list);
        struct pollfd fds[n_fds];
        if (n_fds > 0) {
            looper_get_fds(looper->watcher_list, fds, n_fds);
        }
        n_revents = poll(fds, n_fds, timeout);
        if (n_revents > 0) {
            for (i = 0; i < n_revents; i++) {
                if (fds[i].revents != 0) {
                    looper_watcher_dispatch(looper->watcher_list, fds[i]);
                }
            }
        }
    }
    return looper->state;
}

unsigned int looper_add_watcher(Looper* looper, int fd, void (*handle_events)(int fd, void *user_data, unsigned int id, int looper_event), void *user_data, int looper_event) {
    DList *list;
    Watcher *watcher;
    short events;

    watcher = (Watcher*) malloc(sizeof(Watcher));
    events = 0;
    if (!watcher) {
        printf("Failed to make Watcher\n");
        return -1;
    }

    looper->watcher_last_id += 1;
    watcher->id = looper->watcher_last_id;
    watcher->fd = fd;
    watcher->handle_events = handle_events;
    watcher->user_data = user_data;

    if (looper_event & LOOPER_IN_EVENT) {
        events |= POLLIN;
    }

    if (looper_event & LOOPER_OUT_EVENT) {
        events |= POLLOUT;
    }

    if (looper_event & LOOPER_HUP_EVENT) {
        events |= POLLHUP;
    }

    watcher->events = events;

    list = looper->watcher_list;
    looper->watcher_list = d_list_append(list, watcher);

    return looper->watcher_last_id;
}

unsigned int looper_add_timer(Looper* looper, unsigned int interval, BOOLEAN (*callback)(void *user_data, unsigned int id), void *user_data) {
    Timer *timer;
    long cur_time;

    timer = (Timer*) malloc(sizeof(Timer));
    if (!timer) {
        LOGD("Failed to make Timer\n");
        return -1;
    }

    cur_time = looper_get_monotonic_time();
    if (cur_time < 0) {
        LOGD("Failed to get current time\n");
        return -1;
    }

    timer->user_data = user_data;
    timer->interval = interval;
    timer->expiration = cur_time + interval;
    timer->callback = callback;
    looper->timer_list = d_list_append(looper->timer_list, timer);
    if (!looper->timer_list) {
        LOGD("Failed to add timer\n");
        return -1;
    }
    looper->timer_last_id += 1;
    timer->id = looper->timer_last_id;

    return looper->timer_last_id;
}

void looper_remove_all_watchers(Looper *looper) {
    if (!looper && !(looper->watcher_list)) {
        printf("There is nothing to pointer the Looper\n");
        return;
    }
    d_list_free(looper->watcher_list, looper_destroy_watcher);
    looper->watcher_list = NULL;
}

void looper_remove_timer_with_id(Looper *looper, int id) {
    if (!looper || id < 0) {
        LOGD("Can't remove timer\n");
        return;
    }

    if (!looper->timer_list) {
        LOGD("Can't remove timer\n");
        return;
    }

    looper->timer_list = d_list_remove_with_user_data(looper->timer_list, &id, looper_match_timer, looper_free_timer);
}

void destroy_looper(Looper *looper) {
    if (!looper) {
        LOGD("There is nothing to pointer the Looper\n");
        return;
    }
    looper_remove_all_watchers(looper);
    free(looper);
}