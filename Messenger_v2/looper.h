#ifndef __LOOPER_H__
#define __LOOPER_H__

#define LOOPER_NO_EVENT    0
#define LOOPER_IN_EVENT    1
#define LOOPER_HUP_EVENT   2

typedef struct _Looper Looper;
typedef struct _Watcher Watcher;

Looper* new_looper();
int run(Looper *looper);
void stop(Looper *looper);
void add_watcher(Looper* looper, int fd, void (*handle_events)(int fd, void *user_data, int revents), void *user_data, int events);
void remove_watcher(Looper *looper, int fd);
void remove_all_watchers(Looper *looper);
void destroy_looper(Looper *looper);

#endif
