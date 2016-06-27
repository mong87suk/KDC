#ifndef __MESSAGE_DB__
#define __MESSAGE_DB__

#include "message.h"
#include "DBLinkedList.h"

#define MEESAGE_DB "message_db"

typedef struct _MessageDB MessageDB;

MessageDB* new_message_db(char *data_format);
void destroy_message_db(MessageDB* mesg_db);
void message_db_delete_all(MessageDB *mesg_db);
int message_db_add_mesg(MessageDB *mesg_db, Message *mesg);
DList* message_db_get_messages(MessageDB *mesg_db, int pos, int count);
int message_db_get_message_count(MessageDB *mesg_db);

#endif
