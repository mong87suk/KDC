#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "account_db.h"
#include "account.h"
#include "database.h"
#include "utils.h"
#include "DBLinkedList.h"
#include "stream_buf.h"

struct _AccountDB {
    DataBase *database;
};

static DList* account_db_add_info_buf(DList *stream_buf_list, char *info, int info_len) {
    Stream_Buf *stream_buf;

    stream_buf = NULL;

    if (info_len < 0 || !info) {
        LOGD("Failed to get str len\n");
        return NULL;
    }

    stream_buf = new_stream_buf(sizeof(int));
    if (!stream_buf) {
        LOGD("Failed to make the StreamBuf\n");
        return NULL;
    }
    memcpy(stream_buf_get_available(stream_buf), &info_len, sizeof(int));
    stream_buf_increase_pos(stream_buf, sizeof(int));
    stream_buf_list = d_list_append(stream_buf_list, stream_buf);

    stream_buf = new_stream_buf(info_len);
    if (!stream_buf) {
        LOGD("Failed to make the StreamBuf\n");
        return NULL;
    }
    memcpy(stream_buf_get_available(stream_buf), info, info_len);
    stream_buf_increase_pos(stream_buf, info_len);
    stream_buf_list = d_list_append(stream_buf_list, stream_buf);

    return stream_buf_list;
}

static Stream_Buf* account_db_new_entry(Account *account, int field_mask) {
    int len;
    int i;
    int index;
    int colum, count, buf_size;
    char *str;
    DList *stream_buf_list;
    Stream_Buf *stream_buf;
    BOOLEAN result;

    if (!account) {
        LOGD("There is nothing to point the mesg\n");
        return NULL;
    }

    count = utils_get_colum_count(field_mask);
    if (count <= 0) {
        LOGD("Failed to get colum count\n");
        return NULL;
    }
    count -= 1;
    stream_buf_list = NULL;
    stream_buf = NULL;
    buf_size = 0;
    index = 0;

    for (i = count; i >= 0; i--) {
        index += 1;
        colum = (field_mask >> (FIELD_SIZE * i)) & FIELD_TYPE_FLAG;
        if (colum == STRING_FIELD) {
            switch(index) {
            case ID:
                len = account_get_id_len(account);
                str = account_get_id(account);
                stream_buf_list = account_db_add_info_buf(stream_buf_list, str, len);
                if (!stream_buf_list) {
                    LOGD("Failed to add info buf\n");
                    return NULL;
                }
                buf_size += sizeof(int);
                buf_size += len;
                break;

            case PW:
                len = account_get_pw_len(account);
                str = account_get_pw(account);
                stream_buf_list = account_db_add_info_buf(stream_buf_list, str, len);
                if (!stream_buf_list) {
                    LOGD("Failed to add info buf\n");
                    return NULL;
                }
                buf_size += sizeof(int);
                buf_size += len;
                break;

            case EMAIL:
                len = account_get_email_len(account);
                str = account_get_email(account);
                stream_buf_list = account_db_add_info_buf(stream_buf_list, str, len);
                if (!stream_buf_list) {
                    LOGD("Failed to add info buf\n");
                    return NULL;
                }
                buf_size += sizeof(int);
                buf_size += len;
                break;

            case CONFIRM:
                len = account_get_confirm_len(account);
                str = account_get_confirm(account);
                stream_buf_list = account_db_add_info_buf(stream_buf_list, str, len);
                if (!stream_buf_list) {
                    LOGD("Failed to add info buf\n");
                    return NULL;
                }
                buf_size += sizeof(int);
                buf_size += len;
                break;

            case MOBILE:
                len = account_get_mobile_len(account);
                str = account_get_mobile(account);
                stream_buf_list = account_db_add_info_buf(stream_buf_list, str, len);
                if (!stream_buf_list) {
                    LOGD("Failed to add info buf\n");
                    return NULL;
                }
                buf_size += sizeof(int);
                buf_size += len;
                break;

            default:
                break;
            }
        }
    }

    stream_buf = new_stream_buf(buf_size);
    result = utils_append_data_to_buf(stream_buf_list, stream_buf);
    utils_destroy_stream_buf_list(stream_buf_list);
    if (result == FALSE) {
        LOGD("Failed to append data\n");
        return NULL;
    }
    return stream_buf;
}

AccountDB* account_db_open(char *data_format) {
    AccountDB *account_db;
    DataBase *database;

    if (!data_format) {
        LOGD("There is nothing to point the data format\n");
        return NULL;
    }

    database = database_open(ACCOUNT_DB, data_format);
    if (!database) {
        LOGD("Failed to make the DataBase\n");
        return NULL;
    }

    account_db = (AccountDB*) malloc(sizeof(AccountDB));
    if (!account_db) {
        LOGD("Failed to make the AccountDB\n");
        return NULL;
    }

    account_db->database = database;
    return account_db;
}

void account_db_close(AccountDB* account_db) {
    if(!account_db) {
        LOGD("There is nothing to point the AccountDB\n");
        return;
    }

    database_close(account_db->database);
    free(account_db);
}

void account_db_delete_all(AccountDB* account_db) {
    if (!account_db) {
        LOGD("There is nothing to point the AccountDB\n");
        return;
    }
    database_delete_all(account_db->database);
}

int account_db_add_account(AccountDB *account_db, Account *account) {
    int field_mask;
    Stream_Buf *entry;
    int id;

    if (!account_db || !account) {
        LOGD("Can't add the account\n");
        return -1;
    }

    field_mask = database_get_field_mask(account_db->database);
    if (field_mask == 0) {
        LOGD("Field mask was wrong\n");
        return -1;
    }

    entry = account_db_new_entry(account, field_mask);
    if (!entry) {
        LOGD("Failed to new entry\n");
        return -1;
    }

    id = database_add_entry(account_db->database, entry);
    destroy_stream_buf(entry);
    if (id < 0) {
        LOGD("Failed to add_entry\n");
        return -1;
    }

    return id;
}
