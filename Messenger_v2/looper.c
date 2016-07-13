#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

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

static int looper_set_sort_rule(void *data1, void *data2) {
    Timer *timer1;
    Timer *timer2;

    if (!data1 || !data2) {
        LOGD("Can't set sort rule\n");
        return 0;
    }

    timer1 = (Timer*) data1;
    timer2 = (Timer*) data2;

    if (timer1->interval < timer2->interval) {
        return 1;
    } 
    return 0;
}

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

static void looper_dispatch(Looper *looper, int n_timer) {
    DList *list;
    Timer *timer;
    long cur_time;
    int result;
    int i;

    cur_time = looper_get_monotonic_time();
    if (cur_time < 0) {
        LOGD("Failed to get current time\n");
    }
    
    list = looper->timer_list;

    for (i = 0; i < n_timer; i++) {
        timer = (Timer*) d_list_get_data(list);
        if (timer) {
            if (timer->expiration - cur_time <= 0) {
                result = timer->callback(timer->user_data, timer->id);
                if (result == FALSE) {
                    looper_remove_timer(looper, timer);
                } else if (result == TRUE) {
                    timer->expiration = cur_time + timer->interval;
                }
            }
        }
        list = d_list_next(list); 
    }
}

static long looper_get_timeout(Looper *looper, int n_timer) {
    int cur_time;

    Timer *timer;
    DList *list;

    cur_time = looper_get_monotonic_time();
    if (cur_time < 0) {
        LOGD("Failed to get current time");
        return 0;
    }

    if (n_timer > 0) {
        looper->timer_list = d_list_insert_sort(looper->timer_list, looper_set_sort_rule);
        list = d_list_last(looper->timer_list);
        timer = (Timer*) d_list_get_data(list);
        if (!timer) {
            LOGD("Failed to get data\n");
            return 0;
        }
        return timer->expiration - cur_time;
    }
    return 0;
}


static int looper_get_fds_len(DList *watcher_list) {
    int len;
    int fd;
    
    DList *list;
    Watcher *watcher;

    len = 0;
    fd = 0;

    if (watcher_list) {
        watcher = d_list_get_data(watcher_list);
        fd = watcher->fd;
        while ((list = d_list_next(watcher_list)) {
            watcher = d_list_get_data(list);
            if (fd != watcher->fd) {
                len++;
            }
        }
    }
    return len;
}

/**
  * looper_get_fds:
  * @looper: a pointer to Looper which includes watch_list
  * @fds   : a pointer to pollfd
  *
  * Set events vaule of pollfd
  **/
static void looper_get_fds(DList *watcher_list, struct pollfd *fds) {
    int count;
    int fd;
    short events;
    int i;

    DList *next;
    Watcher *watcher;

    count = 0;

    while (watcher_list) {
        next = d_list_next(watcher_list);
        watcher = (Watcher*) d_list_get_data(watcher_list);
        if (!watcher) {
            LOGD("There is nothing to pointer the Watcher\n");
            break;
        }

        if (count == 0) {
            fds[0].fd = watcher->fd;
            fds[0].events = watcher->events;
            count++;
        } else {
            for (i = 0; i < count; i++) {
                if (fds[i].fd == watcher->fd) {
                    fds[i].events |= watcher->events;
                } else {
                    fds[i].fd = watcher->fd;
                    fds[i].events = watcher->events;
                    count++;
                }
            }
        } 
        watcher_list = next;
    }
}

static int looper_match_watcher_with_fd(void *data1, void *data2) {
    Watcher *watcher = (Watcher*) data1;
    int fd = *((int*) data2);

    if (watcher->fd == fd) {
        return 1;
    } else {
        return 0;
    }
}

static int looper_match_watcher_with_id(void *data1, void *data2) {
    Watcher *watcher = (Watcher*) data1;
    unsigned int id = *((unsigned int*) data2);

    if (watcher->id == id) {
        return 1;
    } else {
        return 0;
    }
}

static int looper_match_timer(void *data1, void *data2) {
    Timer *timer = (Timer*) data1;
    unsigned int id = *((unsigned int*) data2);

    if (timer->id == id) {
        return 1;
    } else {
        return 0;
    }
}

static Watcher *looper_find_watcher(Looper *looper, int fd) {
    Watcher *watcher;

    watcher = d_list_find_data(looper->watcher_list, looper_match_watcher_with_fd, &fd);
    return watcher;
}

static void looper_destroy_watcher(void *data) {
    Watcher *watcher;

    watcher = (Watcher*) data;
    free(watcher);
}

Looper *new_looper() {
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
    short revents;
    unsigned int looper_event;
    int fd, n_revents;
    int i;
    int n_watcher, n_timer, n_fds;
    long timeout;

    Watcher *watcher;

    nfds = 0;

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
        n_fds = looper_get_fds_len(looper->watcher_list);
        n_timer = d_list_length(looper->timer_list);
        if ((n_watcher <= 0) && (n_timer <= 0)) {
            LOGD("There is no Watcher\n");
            return 0;
        }

        looper_dispatch(looper, n_timer);
        timeout = looper_get_timeout(looper, n_timer);

        struct pollfd fds[n_fds];
        looper_get_fds(looper->watcher_list, fds);

        n_revents = poll(fds, n_watcher, timeout);

        if (n_revents > 0) {
            for (i = 0; i < n_watcher; i++) {
                if (fds[i].revents != 0) {
                    fd = fds[i].fd;
                    revents = fds[i].revents;
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

                    watcher = looper_find_watcher(looper, fd);
                    if (!watcher) {
                        continue;
                    }
                    watcher->handle_events(fd, watcher->user_data, watcher->id, looper_event);
                }
            }
        }

        if (n_timer) {
            looper_dispatch(looper, n_timer);
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