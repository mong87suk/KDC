#ifndef __MESSAGE_H__
#define __MESSAGE_H__

#include "m_boolean.h"

typedef struct _Message Message;

Message* new_mesg(long int time, int str_len, char *str);
void destroy_mesg(Message *mesg);

BOOLEAN message_set_time(Message *mesg, long int time);
BOOLEAN message_set_str_len(Message *mesg, int str_len);
BOOLEAN message_set_str(Message *mesg, char *str);

long int message_get_time(Message *mesg);
int message_get_str_len(Message *mesg);
char* message_get_str(Message *mesg);
int message_get_size();

Message* message_create_array(int lne);
Message* message_next(Message* mesgs, int i);
#endif
