#ifndef __SERVER_H__
#define __SERVER_H__

#include "looper.h"

#define MAX_BUF_LEN 100

typedef struct _Server Server;
typedef struct _Client Client;

Server* new_server(Looper *looper);
void destroy_server(Server *server);

#endif
