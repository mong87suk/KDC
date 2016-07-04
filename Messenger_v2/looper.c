#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "DBLinkedList.h"
#include "looper.h"
#include "utils.h"

struct _Looper {
    DList *watcher_list;
    DList *timer_list;
    int state;
};

struct _Watcher {
    int fd;
    void *user_data;
    void (*handle_events)(int fd, void *user_data, int revents);
    short events;
};

struct _Timer {
    void *user_data;
    BOOLEAN (*callback)(void *user_data);
    unsigned int interval;
    unsigned int expired;
    unsigned int callback_count;
    struct timespec called_t;
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

    if (timer1->expired < timer2->expired) {
        return 1;
    } else if (timer1->expired == timer2->expired) {
        if (timer1->callback_count < TRUE && timer2->callback_count) {
            return 1;
        }
    }
    return 0;
}

static struct timespec looper_time_diff(struct timespec cur_t, struct timespec called_t) {
    struct timespec temp;

    if ((cur_t.tv_nsec - called_t.tv_nsec) < 0) {
        temp.tv_sec = cur_t.tv_sec - called_t.tv_sec - 1;
        temp.tv_nsec = 1000000000 + cur_t.tv_nsec - called_t.tv_nsec;
    } else {
        temp.tv_sec = cur_t.tv_sec - called_t.tv_sec;
        temp.tv_nsec = cur_t.tv_nsec - called_t.tv_nsec;
    }
    return temp;
}

static struct timespec looper_get_current_time()
{
    struct timespec tv;

    if (clock_gettime(CLOCK_REALTIME, &tv) < 0) {
        LOGD("Failed to get time\n");
        return tv;
    }
    return tv;
}

static void looper_destroy_timer(void *timer) {
    if (!timer) {
        LOGD("There is nothing to point timer\n");
        return;
    }
    free(timer);
}

void static looper_remove_timer(Looper *looper, Timer *timer) {
    DList *list;

    list = looper->timer_list;

    looper->timer_list =  d_list_remove_with_data(list, timer, looper_destroy_timer);
}

static void looper_callback(Looper *looper, int nfds, unsigned int interval, int n_timer) {
    DList *list;
    Timer *timer;
    struct timespec cur_t;
    struct timespec df_t;
    long int df_ms;
    int result;
    int i;

    cur_t = looper_get_current_time();
    list = looper->timer_list;
    LOGD("interval:%d\n", interval);

    if (n_timer > 1) {
        for (i = 0; i < n_timer; i++) {
            timer  = (Timer*) d_list_get_data(list);
            df_t = looper_time_diff(cur_t, timer->called_t);
            df_ms = df_t.tv_sec * 1000 + df_t.tv_nsec / 1000000;
            timer->called_t = cur_t;

            if (i == n_timer - 1) {
                if (interval == timer->expired || df_ms >= timer->expired) {
                    result = timer->callback(timer->user_data);
                    timer->callback_count += 1;
                    if (result == 0) {
                        looper_remove_timer(looper, timer);
                    } else {
                        timer->expired = timer->interval;
                    }
                    return;
                }
            }
            timer->expired -= df_ms;
            list = d_list_next(list);
        }
    } else {
        timer = (Timer*) d_list_get_data(list);
        df_t = looper_time_diff(cur_t, timer->called_t);
        df_ms = df_t.tv_sec * 1000;
        timer->called_t = cur_t;

        if (interval == timer->expired || df_ms >= timer->expired) {
            LOGD("interval:%u timer->interval:%u\n", interval, timer->interval);
            result = timer->callback(timer->user_data);
            timer->callback_count += 1;
            if (result == 0) {
                looper_remove_timer(looper, timer);
            } else {
                timer->expired = timer->interval;
            }
        } else {
            timer->expired = (unsigned int) (interval - df_ms);
        }
    }
}

static unsigned int looper_get_time(Looper *looper) {
    int n_timer;
    Timer *timer;
    DList *list;

    n_timer = d_list_length(looper->timer_list);
    if (n_timer > 0) {
        looper->timer_list = d_list_insert_sort(looper->timer_list, looper_set_sort_rule);
        list = d_list_last(looper->timer_list);
        timer = (Timer*) d_list_get_data(list);
        if (!timer) {
            LOGD("Failed to get data\n");
            return 0;
        }
        return timer->expired;
    }
    return 0;
}

/**
  * set_fd_event:
  * @looper: a pointer to Looper which includes watch_list
  * @fds         : a pointer to pollfd
  *
  * Set events vaule of pollfd
  **/
static void looper_get_fds(DList *watcher_list, struct pollfd *fds) {
    DList *next;
    Watcher *watcher;
    int index;
    index = 0;

    while (watcher_list) {
        next = d_list_next(watcher_list);
        watcher = (Watcher*) d_list_get_data(watcher_list);

        if (!watcher) {
            printf("%s %s There is nothing to pointer the Watcher\n", __FILE__, __func__);
            break;
        }

        fds[index].fd = watcher->fd;
        fds[index].events = watcher->events;
        watcher_list = next;
        index++;
    }
}

static int looper_match_watcher(void *data1, void *data2) {
    Watcher *watcher = (Watcher*) data1;
    int fd = *((int*) data2);

    if (!watcher) {
        printf("%s %s There is nothing to point the Watcher\n", __FILE__, __func__);
        return 0;
    }

    if (watcher->fd == fd) {
        return 1;
    } else {
        return 0;
    }
}

static Watcher* looper_find_watcher(Looper *looper, int fd) {
    Watcher *watcher;
    if (!looper) {
        printf("%s %s There is nothing to pointer the Looper\n", __FILE__, __func__);
        return NULL;
    }

    watcher = d_list_find_data(looper->watcher_list, looper_match_watcher, &fd);
    return watcher;
}

static void looper_destroy_watcher(void *data) {
    Watcher *watcher;

    if (!data) {
        printf("%s %s There is nothing to pointer the Watcher\n", __FILE__, __func__);
        return;
    }
    watcher = (Watcher*) data;
    free(watcher);
}

Looper* new_looper() {
    Looper *looper;
    looper = (Looper*) malloc(sizeof(Looper));
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

/**
  * looper run:
  * @looper: Looper is struct which includes wathcer_list and state
  *
  * Run stops when state of looper is 0
  **/
int looper_run(Looper *looper) {
    short revents;
    unsigned int looper_event;
    int fd, nfds;
    int i;
    int n_watcher, n_timer;
    int interval;
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
        n_watcher = d_list_length(looper->watcher_list);
        n_timer = d_list_length(looper->timer_list);
        if ((n_watcher <= 0) && (n_timer <= 0)) {
            LOGD("There is no Watcher\n");
            return 0;
        }

        interval = looper_get_time(looper);

        struct pollfd fds[n_watcher];
        looper_get_fds(looper->watcher_list, fds);

        nfds = poll(fds, n_watcher, interval);

        if (nfds > 0) {
            for (i = 0; i < n_watcher; i++) {
                if (fds[i].revents != 0) {
                    LOGD("pollfd index:%d revent:%d\n", i, fds[i].revents);
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
                    watcher->handle_events(fd, watcher->user_data, looper_event);
                }
            }
        }

        if (n_timer) {
            looper_callback(looper, nfds, interval, n_timer);
        }
    }
    return looper->state;
}

void looper_add_watcher(Looper* looper, int fd, void (*handle_events)(int fd, void *user_data, int looper_event), void *user_data, int looper_event) {
    DList *list;
    Watcher *watcher;
    short events;

    watcher = (Watcher*) malloc(sizeof(Watcher));
    events = 0;
    if (!watcher) {
        printf("Failed to make Watcher\n");
        return;
    }

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
}

void looper_add_timer(Looper* looper, unsigned int interval, BOOLEAN (*callback)(void *user_data), void *user_data) {
    Timer *timer;
    struct timespec called_t;

    timer = (Timer*) malloc(sizeof(Timer));
    if (!timer) {
        LOGD("Failed to make Timer\n");
        return;
    }

    called_t = looper_get_current_time();
    LOGD("interval:%u\n", interval);

    timer->user_data = user_data;
    timer->callback = callback;
    timer->callback_count = 0;
    timer->interval = interval;
    timer->expired = interval;
    timer->called_t = called_t;

    looper->timer_list = d_list_append(looper->timer_list, timer);
}

void looper_remove_all_watchers(Looper *looper) {
    if (!looper && !(looper->watcher_list)) {
        printf("There is nothing to pointer the Looper\n");
        return;
    }
    d_list_free(looper->watcher_list, looper_destroy_watcher);
    looper->watcher_list = NULL;
}

void looper_remove_watcher(Looper *looper, int fd) {
    if (!looper && !looper->watcher_list) {
        LOGD("Can't remove Wathcer\n");
        return;
    }

    looper->watcher_list = d_list_remove_with_user_data(looper->watcher_list, &fd, looper_match_watcher, looper_destroy_watcher);
}

void destroy_looper(Looper *looper) {
    if (!looper) {
        LOGD("There is nothing to pointer the Looper\n");
        return;
    }
    looper_remove_all_watchers(looper);
    free(looper);
}


