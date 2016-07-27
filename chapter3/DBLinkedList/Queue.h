#ifndef __QUEUE_H__
#define __QUEUE_H__

#include <stdbool.h>

typedef struct _Queue Queue;
bool empty(Queue *queue);
int size(Queue *queue);
bool push(Queue *queue, void *in);
void* pop(Queue *queue);
Queue* queue_new();
void queue_free(Queue *queue, void(*free_data)(void *data));
#endif
