#ifndef __MESSAGE_DB__
#define __MESSAGE_DB__

#include "message.h"
#include "DBLinkedList.h"

#define MEESAGE_DB "message_db"

typedef struct _MessageDB MessageDB;

MessageDB* new_message_db(char *data_format);
void destroy_message_db(MessageDB* message_db);
int add_message(MessageDB *mesg_db, Message *mesg);
DList* get_messages(MessageDB *mesg_db, int pos, int count);
int message_db_get_message_count(MessageDB *mesg_db);

#endif
