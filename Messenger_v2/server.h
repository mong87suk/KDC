#ifndef __SERVER_H__
#define __SERVER_H__

#include "looper.h"

typedef enum {
    APPEND_DATA_SUCCESS = 1,
    APPEND_DATA_FAILURE  = -1
} APPEND_DATA_RESULT;

typedef enum {
    READY_TO_READ_REQ = 0,
    START_TO_READ_REQ = 1,
    FINISH_TO_READ_REQ = 2
} READ_REQ_STATE;

typedef struct _Server Server;
typedef struct _Client Client;
typedef struct _UserData UserData;

Server* new_server(Looper *looper);
void destroy_server(Server *server);

#endif
