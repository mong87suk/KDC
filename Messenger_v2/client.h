#ifndef __CLIENT_H__
#define __CLIENT_H__

#include "looper.h"

#define BUF_SIZE 3
#define REQ_STR "command "
#define REQ_STR_MIN_LEN 10
#define REQ_FIRST_OR_LAST_MESG_PACKET_SIZE 12

typedef enum {
    READY_TO_READ_RES = 0,
    START_TO_READ_RES = 1,
    FINISH_TO_READ_RES = 2
} READ_RES_STATE;

typedef struct _Client Client;

Client* new_client(Looper *looper);
void destroy_client(Client *client);

#endif
