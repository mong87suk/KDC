#ifndef __SERVER_H__
#define __SERVER_H__

#include "looper.h"

#define SND_ALL         0
#define SND_ONE         1

typedef enum {
    READY_TO_READ_REQ = 0,
    START_TO_READ_REQ = 1,
    FINISH_TO_READ_REQ = 2
} READ_REQ_STATE;

typedef struct _Server Server;
typedef struct _Client Client;
typedef struct _UserData UserData;
typedef struct _Online   Online;

Server *new_server(Looper *looper);
void destroy_server(Server *server);

#endif
