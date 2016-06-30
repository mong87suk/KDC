#ifndef __LOOPER_H__
#define __LOOPER_H__

#define LOOPER_IN_EVENT    0x0001
#define LOOPER_OUT_EVENT   0x0002
#define LOOPER_HUP_EVENT   0x0004

typedef struct _Looper Looper;
typedef struct _Watcher Watcher;
typedef struct _Timer Timer;

Looper* new_looper();
int looper_run(Looper *looper);
void looper_stop(Looper *looper);
void looper_add_watcher(Looper* looper, int fd, void (*handle_events)(int fd, void *user_data, int revents), void *user_data, int events);
void looper_add_timer(Looper* looper, unsigned int interval, void (*callback)(void *user_data), void *user_data);
void looper_remove_watcher(Looper *looper, int fd);
void looper_remove_all_watchers(Looper *looper);
void destroy_looper(Looper *looper);
void looper_remove_timer(Looper *looper, void *user_data);

#endif
