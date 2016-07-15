#ifndef __LOOPER_H__
#define __LOOPER_H__

#include "m_boolean.h"

#define LOOPER_IN_EVENT    0x0001
#define LOOPER_OUT_EVENT   0x0002
#define LOOPER_HUP_EVENT   0x0004

typedef struct _Looper Looper;
typedef struct _Watcher Watcher;
typedef struct _Timer Timer;

Looper *new_looper(void);
int looper_run(Looper *looper);
void looper_stop(Looper *looper);
unsigned int looper_add_watcher(Looper* looper, int fd, void (*handle_events)(int fd, void *user_data, unsigned int id, int revents), void *user_data, int events);
unsigned int looper_add_timer(Looper* looper, unsigned int interval, BOOLEAN (*callback)(void *user_data, unsigned int id), void *user_data);
void looper_remove_watcher(Looper *looper, unsigned int id);
void looper_remove_all_watchers(Looper *looper);
void destroy_looper(Looper *looper);
void looper_remove_timer_with_id(Looper *looper, int id);
void looper_remove_all_timer(Looper *looper);

#endif
