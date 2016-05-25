#ifndef __SERVER_H__
#define __SERVER_H__

#include "looper.h"

typedef enum {
    ADD_CLIENT_SUCCESS = 1,
    ADD_CLIENT_FAILURE = -1
} ADD_CLIENT_RESULT;

typedef enum {
        APPEND_DATA_SUCCESS = 1,
            APPEND_DATA_FAILURE  = -1
} APPEND_DATA_RESULT;

typedef enum {
    CLIENT_READ_REQ_FAILURE = -1,
    CLIENT_READ_REQ_READY = 0,
    CLIENT_READ_REQ_START = 1,
    CLIENT_READ_REQ_FINISH = 2
} CLIENT_READ_REQ_STATE;

typedef struct _Server Server;
typedef struct _Client Client;

Server* new_server(Looper *looper);
void destroy_server(Server *server);

#endif
