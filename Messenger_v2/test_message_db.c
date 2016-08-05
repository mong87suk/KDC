#include <stdio.h>
#include <assert.h>
#include <time.h>
#include <string.h>

#include <kdc/DBLinkedList.h>

#include "message.h"
#include "message_db.h"
#include "utils.h"

static void free_message(void *data) {
    Message *mesg;

    mesg = (Message *) data;

    if (!mesg) {
        LOGD("There is nothing to point the Message\n");
        return;
    }    

    destroy_mesg(mesg);
}

int main() {
    int id;
    char *str = "hellow";
    int str_len;
    int result;
    int count;
    time_t current_time;

    DList *mesg_list;
    DList *list;
    MessageDB *message_db;
    Message *mesg1;
    Message *mesg2;

    current_time = time(NULL);

    assert(current_time != ((time_t) - 1));

    str_len = strlen(str); 
    mesg1 = new_mesg(current_time, str_len, str);
    assert(mesg1);
    assert(message_get_id(mesg1) == -1);

    message_db = message_db_open(MESSAGE_DB_DATA_FORMAT);
    assert(message_db);

    count = message_db_get_message_count(message_db);
    id = message_db_add_mesg(message_db, mesg1);
    assert(id > 0);
    assert(message_db_get_message_count(message_db) == (count + 1));
    result = message_set_id(mesg1, id);
    assert(result == TRUE);

    count = message_db_get_message_count(message_db);
    mesg_list = message_db_get_messages(message_db, 1, count);
    assert(mesg_list);

    list = d_list_last(mesg_list);
    assert(list);

    mesg2 = (Message*) d_list_get_data(list);
    assert(message_get_id(mesg2) == id);
    assert(message_get_time(mesg2) == current_time);
    assert(message_get_str_len(mesg2) == str_len);
    assert(strncmp(message_get_str(mesg2), str, str_len) == 0);
    d_list_free(mesg_list, free_message);

    message_db_delete_all(message_db);
    assert(message_db_get_message_count(message_db) == 0);

    message_db_close(message_db);

    return 0;
}
