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

void destroy_mesg(Message *mesg) {
    if (!mesg) {
        LOGD("Can't destroy the mesg\n");
        return;
    }

    if(mesg->str) {
        free(mesg->str);
    }

    free(mesg);
}
int message_set_time(Message *mesg, long int time) {
    if (!mesg) {
        LOGD("Failed to set the time\n");
        return FALSE;
    }

    mesg->time = time;
    return TRUE;
}

int message_set_str_len(Message *mesg, int str_len) {
    if (!mesg) {
        LOGD("Failed to set the str_len\n");
        return FALSE;
    }

    mesg->str_len = str_len;
    return TRUE;
}

int message_set_str(Message *mesg, char *str) {
    if (!mesg) {
        LOGD("Failed to set the str\n");
        return FALSE;
    }

    mesg->str = str;
    return TRUE;
}

long int message_get_time(Message *mesg) {
    if (!mesg) {
        LOGD("There is nothing to point the mesg\n");
        return -1;
    }

    return mesg->time;
}

int message_get_str_len(Message *mesg) {
    if (!mesg) {
        LOGD("There is nothing to point the str_len\n");
        return -1;
    }

    return mesg->str_len;
}

char* message_get_str(Message *mesg) {
    if (!mesg) {
        LOGD("There is nothing to point the mesg\n");
        return NULL;
    }

    return mesg->str;
}

int message_get_size() {
    return sizeof(Message);
}

Message* message_create_array(int len) {
    Message *mesgs;
    if (len == 0) {
        LOGD("Len is zero\n");
        return NULL;
    }

    mesgs = (Message*) malloc(sizeof(Message) * len);
    if (!mesgs) {
        LOGD("Failed to make the mesg array\n");
        return NULL;
    }

    return mesgs;
}

Message* message_next(Message *mesgs, int i) {
    if (!mesgs) {
        LOGD("There is nothing to point the message\n");
        return NULL;
    }

    return (mesgs + i );
}
