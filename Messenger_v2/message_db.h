#ifndef __MESSAGE_DB__
#define __MESSAGE_DB__

#define MEESAGE_DB "message_db"

typedef struct _MessageDB MessageDB;

MessageDB* new_message_db(char *data_format);
void destroy_message_db(MessageDB* message_db);

#endif
