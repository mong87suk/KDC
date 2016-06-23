#include <stdlib.h>
#include <stdio.h>

#include "message_db.h"
#include "database.h"
#include "utils.h"

struct _MessageDB {
    char *data_format;
    DataBase *database;
};

MessageDB* new_message_db(char *data_format) {
    MessageDB *message_db;

    if (!data_format) {
        LOGD("There is nothing to point the data format\n");
        return NULL;
    }

    message_db = (MessageDB*) malloc(sizeof(MessageDB));
    if (!message_db) {
        LOGD("Failed to make the MessageDB\n");
        return NULL;
    }
    database = new_database(MEESAGE_DB, data_foramt);
    if (!database) {
        LOGD("Failed to make the DataBase\n");
        return NULL;
    }

    message_db->data_format = data_format;
    message_db->database = database;


    return message_db;
}

void destroy_message_db(MessageDB* message_db) {
    if(!message_db) {
        LOGD("There is nothing to point the MessageDB\n");
        return;
    }

    free(message_db->data_format);
    free(message_db);
}


int add_message(MessageDB *mesg_db, Message *mesg) {
    if (!mesg_db || !mesg) {
        LOGD("Can't add the message\n");
        return -1;
    }
}
