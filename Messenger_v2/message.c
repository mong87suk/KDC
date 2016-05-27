#include <stdlib.h>
#include <stdio.h>

#include "message.h"
#include "utils.h"
#include "m_boolean.h"

struct _Message
{
    long int time;
    int str_len;
    char *str;
};


Message* new_mesg(long int time, int str_len, char *str) {
    Message* mesg;

    mesg = (Message*) malloc(sizeof(Message));
    if (!mesg) {
        LOGD("Failed to make the Message\n");
        return NULL;
    }

    mesg->time = time;
    mesg->str_len = str_len;
    mesg->str = str;

    return mesg;
}

int set_time(Message *mesg, long int time) {
    if (!mesg) {
        LOGD("Failed to set the time\n");
        return FALSE;
    }

    mesg->time = time;
    return TRUE;
}

int set_str_len(Message *mesg, int str_len) {
    if (!mesg) {
        LOGD("Failed to set the str_len\n");
        return FALSE;
    }

    mesg->str_len = str_len;
    return TRUE;
}

char* set_str(Message *mesg, char *str) {
    if (!mesg) {
        LOGD("Failed to set the str\n");
        return NULL;
    }

    mesg->str = str;
    return NULL;
}

long int get_time(Message *mesg) {
    if (!mesg) {
        LOGD("There is nothing to point the mesg\n");
        return -1;
    }

    return mesg->time;
}

int get_str_len(Message *mesg) {
    if (!mesg) {
        LOGD("There is nothing to point the str_len\n");
        return -1;
    }

    return mesg->str_len;
}

char* get_str(Message *mesg) {
    if (!mesg) {
        LOGD("There is nothing to point the mesg\n");
        return NULL;
    }

    return mesg->str;
}
