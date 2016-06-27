#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <string.h>

#include "message_db.h"
#include "message.h"
#include "database.h"
#include "utils.h"
#include "entry_point.h"
#include "DBLinkedList.h"
#include "m_boolean.h"

struct _MessageDB {
    char *data_format;
    DataBase *database;
};

static int message_db_comp_field_mask(int field_mask1, int field_mask2) {
    if (field_mask1 < 0 || field_mask2 < 0) {
        LOGD("the Field mask was wrong\n");
        return FALSE;
    }

    if (field_mask1 == field_mask2) {
        return TRUE;
    }

    return FALSE;
}

static Stream_Buf* message_db_convert_message_to_entry(Message *mesg, int field_mask) {
    int mesg_time;
    int len;
    int colum, count, buf_size;
    char *str;
    DList *stream_buf_list;
    Stream_Buf *stream_buf;

    if (!mesg) {
        LOGD("There is nothing to point the mesg\n");
        return NULL;
    }

    count = utils_get_count_to_move_flag(field_mask);
    if (count < 0) {
        LOGD("the field mask was wrong\n");
        return NULL;
    }

    stream_buf_list = NULL;
    buf_size = 0;

    do {
        colum = (field_mask >> (FIELD_SIZE * count)) & FIELD_TYPE_FLAG;
        switch (colum) {
        case INTEGER_FIELD:
            stream_buf = new_stream_buf(sizeof(int));
            if (!stream_buf) {
                LOGD("Failed to make the StreamBuf\n");
                return NULL;
            }
            mesg_time = (int) message_get_time(mesg);
            if (mesg_time < 0) {
                LOGD("Failed to get the mesg time\n");
                return NULL;
            }
            buf_size += sizeof(int);
            memcpy(stream_buf_get_available(stream_buf), &mesg_time, sizeof(int));
            stream_buf_increase_position(stream_buf, sizeof(int));
            stream_buf_list = d_list_append(stream_buf_list, stream_buf);
            break;

        case STRING_FIELD:
            len = message_get_str_len(mesg);
            if (len < 0) {
                LOGD("Failed to get the str len\n");
                return NULL;
            }
            stream_buf = new_stream_buf(sizeof(int));
            if (!stream_buf) {
                LOGD("Failed to make the StreamBuf\n");
                return NULL;
            }
            buf_size += sizeof(int);
            memcpy(stream_buf_get_available(stream_buf), &len, sizeof(int));
            stream_buf_increase_position(stream_buf, sizeof(int));
            stream_buf_list = d_list_append(stream_buf_list, stream_buf);

            str = message_get_str(mesg);
            if (!str) {
                LOGD("Failed to get the str\n");
                return NULL;
            }
            stream_buf = new_stream_buf(len);
            if (!stream_buf) {
                LOGD("Failed to make the StreamBuf\n");
                return NULL;
            }
            buf_size += len;
            memcpy(stream_buf_get_available(stream_buf), str, len);
            stream_buf_increase_position(stream_buf, len);
            stream_buf_list = d_list_append(stream_buf_list, stream_buf);

            break;

        case 0:
            break;

        default:
            LOGD("field mask was wrong\n");
            return NULL;
        }
        count--;
    } while (colum && count >= 0);

    stream_buf = new_stream_buf(buf_size);
    utils_append_data_to_buf(stream_buf_list, stream_buf);
    utils_destroy_stream_buf_list(stream_buf_list);

    return stream_buf;
}

Message* message_db_convert_entry_to_message(Stream_Buf *entry, int field_mask) {
    char *buf, *str;
    int colum, count;
    int len, mesg_time;
    Message *mesg;

    if (!entry || field_mask < 0) {
        LOGD("Can't failed convert\n");
        return NULL;
    }

    buf = stream_buf_get_buf(entry);
    if (!buf) {
        LOGD("Failed to get the buf\n");
        return NULL;
    }

    count = utils_get_count_to_move_flag(field_mask);
    if (count < 0) {
        LOGD("the field mask was wrong\n");
        return NULL;
    }

    do {
        colum = (field_mask >> (FIELD_SIZE * count)) & FIELD_TYPE_FLAG;
        switch (colum) {
        case INTEGER_FIELD:
            memcpy(&mesg_time, buf, sizeof(int));
            if (mesg_time < 0) {
                LOGD("Failed to get mesg time\n");
                return NULL;
            }
            buf += sizeof(int);
            break;

        case STRING_FIELD:
            memcpy(&len, buf, sizeof(int));
            if (len < 0) {
                LOGD("Failed to get len\n");
                return NULL;
            }
            buf += sizeof(int);

            str = (char*) malloc(len);
            if (!str) {
                LOGD("Failed to make buf\n");
                return NULL;
            }
            memcpy(str, buf, len);
            buf += len;
            break;

        case 0:
            break;

        default:
            LOGD("field mask was wrong\n");
            return NULL;
        }
        count--;
    } while (colum && count >= 0);

    mesg = new_mesg((long int)mesg_time, len, str);
    if (!mesg) {
        LOGD("Failed to make the Message\n");
        return NULL;
    }

    return mesg;
}

MessageDB* new_message_db(char *data_format) {
    MessageDB *message_db;
    DataBase *database;

    if (!data_format) {
        LOGD("There is nothing to point the data format\n");
        return NULL;
    }

    message_db = (MessageDB*) malloc(sizeof(MessageDB));
    if (!message_db) {
        LOGD("Failed to make the MessageDB\n");
        return NULL;
    }
    database = new_database(MEESAGE_DB, data_format);
    if (!database) {
        LOGD("Failed to make the DataBase\n");
        return NULL;
    }

    message_db->data_format = data_format;
    message_db->database = database;


    return message_db;
}

void destroy_message_db(MessageDB* mesg_db) {
    if(!mesg_db) {
        LOGD("There is nothing to point the MessageDB\n");
        return;
    }

    destroy_database(mesg_db->database);
    free(mesg_db->data_format);
    free(mesg_db);
}

void message_db_delete_all(MessageDB *mesg_db) {
    if (!mesg_db) {
        LOGD("There is nothing to point the MessageDB\n");
        return;
    }

    database_delete_all(mesg_db->database);
    free(mesg_db->data_format);
    free(mesg_db);
}

int message_db_add_message(MessageDB *mesg_db, Message *mesg) {
    int field_mask;
    Stream_Buf *entry;
    int id;
    int result;

    if (!mesg_db || !mesg) {
        LOGD("Can't add the message\n");
        return -1;
    }
    
    field_mask = database_convert_data_format_to_field_mask(mesg_db->data_format);
    result = message_db_comp_field_mask(field_mask, database_get_field_mask(mesg_db->database));
    if (!result) {
        LOGD("Filed mask is not matched\n");
        return -1;
    }

    entry = message_db_convert_message_to_entry(mesg, field_mask);
    if (!entry) {
        LOGD("Failed to convert messaget to entry\n");
        return -1;
    }

    id = database_add_entry(mesg_db->database, entry);
    if (id < 0) {
        LOGD("Failed to add_entry\n");
        return -1;
    }

    return id;
}

DList* message_db_get_messages(MessageDB *mesg_db, int pos, int count) {
    int i;
    int field_mask, result;
    DList *mesg_list;
    EntryPoint *entry_point;
    Stream_Buf *entry;
    Message *mesg;

    if (!mesg_db) {
        LOGD("There is nothing to point the MessageDB\n");
        return NULL;
    }

    if (count == 0) {
        LOGD("There is no message\n");
        return NULL;
    }

    field_mask = database_convert_data_format_to_field_mask(mesg_db->data_format);
    result = message_db_comp_field_mask(field_mask, database_get_field_mask(mesg_db->database));
    if (!result) {
        LOGD("Filed mask is not matched\n");
        return NULL;
    }

    mesg_list = NULL;
    LOGD("pos :%d count:%d\n", pos, count);

    for (i = pos; i < (pos +count); i++) {
        entry_point = database_get_entry_point(mesg_db->database, i);
        if (!entry_point) {
            LOGD("There is no the entry point matched id\n");
            continue;
        }
        entry = entry_point_get_value(entry_point);
        mesg = message_db_convert_entry_to_message(entry, field_mask);
        if (!mesg) {
            LOGD("Failed to convert\n");
            return NULL;
        }

        mesg_list = d_list_append(mesg_list, mesg);
    }

    return mesg_list;
}

int message_db_get_message_count(MessageDB *mesg_db) {
    int count;

    if (!mesg_db) {
        LOGD("There is nothing to point the mesg_db\n");
        return -1;
    }
    
    count = database_get_entry_point_count(mesg_db->database);

    if (count < 0) {
        LOGD("Failed to get entry point\n");
        return -1;
    }

    return count;
}

