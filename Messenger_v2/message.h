#ifndef __MESSAGE_H__
#define __MESSAGE_H__
typedef struct _Message Message;

Message* new_mesg(long int time, int str_len, char *str);
void destroy_mesg(Message *mesg);

int set_time(Message *mesg, long int time);
int set_str_len(Message *mesg, int str_len);
int set_str(Message *mesg, char *str);

long int get_time(Message *mesg);
int get_str_len(Message *mesg);
char* get_str(Message *mesg);
int get_message_size();

Message *create_mesg_array(int lne);
Message* next_mesg(Message* mesgs, int i);
#endif
