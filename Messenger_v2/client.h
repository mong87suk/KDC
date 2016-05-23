#ifndef __CLIENT_H__
#define __CLIENT_H__

#include "looper.h"

#define BUF_SIZE 3
#define REQ_STR "command "
#define REQ_STR_MIN_LEN 10
#define REQ_FIRST_OR_LAST_MESG_PACKET_SIZE 12

typedef enum {
    CREATE_PACKET_SUCCESS = 1,
    CREATE_PACKET_FAILURE = -1
} CREATE_PACKET_RESULT;

typedef enum {
    APPEND_DATA_SUCCESS = 1,
    APPEND_DATA_FAILURE  = -1
} APPEND_DATA_RESULT;

typedef struct _Client Client;

Client* new_client(Looper *looper);
void destroy_client(Client *client);

#endif
