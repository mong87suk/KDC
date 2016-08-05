#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <kdc/DBLinkedList.h>

#include "account_db.h"
#include "account.h"
#include "database.h"
#include "utils.h"
#include "stream_buf.h"
#include "entry_point.h"

struct _AccountDB {
    DataBase *database;
};

static Stream_Buf *account_db_new_account_info_buf(char *info, int len) {
    if (!info) {
        LOGD("Can't new account info buf\n");
        return NULL;
    }

    if (len <= 0) {
        LOGD("Can't new account info buf\n");
        return NULL;
    }

    Stream_Buf *stream_buf = new_stream_buf(sizeof(int) + len);
    if (!stream_buf) {
        LOGD("Failed to make the StreamBuf\n");
        return NULL;
    }

    memcpy(stream_buf_get_available(stream_buf), &len, sizeof(int));
    BOOLEAN result = stream_buf_increase_pos(stream_buf, sizeof(int));
    if (result == FALSE) {
        LOGD("Failed to increase pos\n");
        destroy_stream_buf(stream_buf);
        return NULL;
    }

    memcpy(stream_buf_get_available(stream_buf), info, len);
    result = stream_buf_increase_pos(stream_buf, len);
    if (result == FALSE) {
        LOGD("Failed to increase pos\n");
        destroy_stream_buf(stream_buf);
        return NULL;
    }

    return stream_buf;
}

static Stream_Buf *account_db_new_entry(Account *account, int field_mask) {
    int count = utils_get_colum_count(field_mask);
    if (count <= 0) {
        LOGD("Failed to get colum count\n");
        return NULL;
    }
    count -= 1;
    
    DList *stream_buf_list = NULL;
    Stream_Buf *stream_buf = NULL;
    
    int buf_size = 0;
    int index = 0;
    int column = 0;
    int len = 0;
    char *str;
    
    for (int i = count; i >= 0; i--) {
        column = (field_mask >> (FIELD_SIZE * i)) & FIELD_TYPE_FLAG;
        if (column == STRING_FIELD || column == KEYWORD_FIELD) {
            switch(index) {
            case USER_ID:
                str = account_get_user_id(account);
                len = strlen(str);
                stream_buf = account_db_new_account_info_buf(str, len);
                stream_buf_list = d_list_append(stream_buf_list, stream_buf);
                if (!stream_buf || !stream_buf_list) {
                    LOGD("Failed new entry\n");
                    return NULL;
                }
                buf_size += sizeof(int);
                buf_size += len;
                break;

            case PW:
                str = account_get_pw(account);
                len = strlen(str);
                stream_buf = account_db_new_account_info_buf(str, len);
                stream_buf_list = d_list_append(stream_buf_list, stream_buf);
                if (!stream_buf || !stream_buf_list) {
                    LOGD("Failed new entry\n");
                    return NULL;
                }
                buf_size += sizeof(int);
                buf_size += len;
                break;

            case EMAIL:
                str = account_get_email(account);
                len = strlen(str);
                stream_buf = account_db_new_account_info_buf(str, len);
                stream_buf_list = d_list_append(stream_buf_list, stream_buf);
                if (!stream_buf || !stream_buf_list) {
                    LOGD("Failed new entry\n");
                    return NULL;
                }
                buf_size += sizeof(int);
                buf_size += len;
                break;

            case CONFIRM:
                str = account_get_confirm(account);
                len = strlen(str);
                stream_buf = account_db_new_account_info_buf(str, len);
                stream_buf_list = d_list_append(stream_buf_list, stream_buf);
                if (!stream_buf || !stream_buf_list) {
                    LOGD("Failed new entry\n");
                    return NULL;
                }
                buf_size += sizeof(int);
                buf_size += len;
                break;

            case MOBILE:
                str = account_get_mobile(account);
                len = strlen(str);
                stream_buf = account_db_new_account_info_buf(str, len);
                stream_buf_list = d_list_append(stream_buf_list, stream_buf);
                if (!stream_buf || !stream_buf_list) {
                    LOGD("Failed new entry\n");
                    return NULL;
                }
                buf_size += sizeof(int);
                buf_size += len;
                break;

            default:
                break;
            }
        }
        index++;
    }
    stream_buf = new_stream_buf(buf_size);
    BOOLEAN result = utils_append_data_to_buf(stream_buf_list, stream_buf);
    utils_destroy_stream_buf_list(stream_buf_list);
    if (result == FALSE) {
        LOGD("Failed to append data\n");
        return NULL;
    }
    return stream_buf;
}

static Account *account_db_new_account(Stream_Buf *entry, int field_mask) {
    if (!entry) {
        LOGD("There is nothing to point the entry\n");
        return NULL;
    }

    int count = utils_get_colum_count(field_mask);
    if (count <= 0) {
        LOGD("Failed to get colum count\n");
        return NULL;
    }
    count -= 1;
    
    char *buf = stream_buf_get_buf(entry);
    if (!buf) {
        LOGD("Failed to get the buf\n");
        return NULL;
    }

    int id = 0;
    memcpy(&id, buf, ID_SIZE);
    if (id < 0) {
        LOGD("ID was wrong\n");
        return NULL;
    }
    buf += ID_SIZE;

    int len = 0;
    int column = 0;
    int index = 0;
    char *user_id, *pw, *email, *confirm, *mobile;
    
    for (int i = count; i >= 0; i--) {
        column = (field_mask >> (FIELD_SIZE * i)) & FIELD_TYPE_FLAG;
        if (column == STRING_FIELD || column == KEYWORD_FIELD) {
            switch(index) {
            case USER_ID:
                memcpy(&len, buf, sizeof(int));
                if (len < 0) {
                    LOGD("Failed to get len\n");
                    return NULL;
                }
                buf += sizeof(int);

                user_id = (char*) malloc(len + 1);
                if (!user_id) {
                    LOGD("Failed to make buf\n");
                    return NULL;
                }
                memset(user_id, 0, len + 1);
                strncpy(user_id, buf, len);
                buf += len;
                break;

            case PW:
                memcpy(&len, buf, sizeof(int));
                if (len < 0) {
                    LOGD("Failed to get len\n");
                    return NULL;
                }
                buf += sizeof(int);

                pw = (char*) malloc(len + 1);
                if (!pw) {
                    LOGD("Failed to make buf\n");
                    return NULL;
                }
                memset(pw, 0, len + 1);
                strncpy(pw, buf, len);
                buf += len;
                break;

            case EMAIL:
                memcpy(&len, buf, sizeof(int));
                if (len < 0) {
                    LOGD("Failed to get len\n");
                    return NULL;
                }
                buf += sizeof(int);

                email = (char*) malloc(len + 1);
                if (!email) {
                    LOGD("Failed to make buf\n");
                    return NULL;
                }
                memset(email, 0, len + 1);
                strncpy(email, buf, len);
                buf += len;
                break;

            case CONFIRM:
                memcpy(&len, buf, sizeof(int));
                if (len < 0) {
                    LOGD("Failed to get len\n");
                    return NULL;
                }
                buf += sizeof(int);

                confirm = (char*) malloc(len + 1);
                if (!confirm) {
                    LOGD("Failed to make buf\n");
                    return NULL;
                }
                memset(confirm, 0, len + 1);
                strncpy(confirm, buf, len);
                buf += len;
                break;

            case MOBILE:
                memcpy(&len, buf, sizeof(int));
                if (len < 0) {
                    LOGD("Failed to get len\n");
                    return NULL;
                }
                buf += sizeof(int);

                mobile = (char*) malloc(len + 1);
                if (!mobile) {
                    LOGD("Failed to make buf\n");
                    return NULL;
                }
                memset(mobile, 0, len + 1);
                strncpy(mobile, buf, len);
                buf += len;
                break;
                             
            default:
                break;
            }
        }
        index++;
    }

    Account *account = new_account(user_id, pw, email, confirm, mobile);
    
    free(user_id);
    free(pw);
    free(email);
    free(confirm);
    free(mobile);

    if (!account) {
        LOGD("Failed to make the Account\n");
        return NULL;
    }

    BOOLEAN result = account_set_id(account, id);
    if (result == FALSE) {
        LOGD("Failed to set id\n");
        destroy_account(account);
        return NULL;
    }

    return account;
}

AccountDB *account_db_open(char *data_format) {
    if (!data_format) {
        LOGD("There is nothing to point the data format\n");
        return NULL;
    }

    DataBase *database = database_open(ACCOUNT_DB, data_format);
    if (!database) {
        LOGD("Failed to make the DataBase\n");
        return NULL;
    }

    AccountDB *account_db = (AccountDB*) malloc(sizeof(AccountDB));
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
    if (!account_db || !account) {
        LOGD("Can't add the account\n");
        return -1;
    }

    char *user_id = account_get_user_id(account);
    if (!user_id) {
        LOGD("Failed to get the user id\n");
        return -1;
    }

    Where *where = new_where(USER_ID, user_id);
    if (!where) {
        LOGD("Failed to make the where\n");
        return -1;
    }

    DList *where_list = NULL;
    where_list = d_list_append(where_list, where);
    if (!where_list) {
        LOGD("Failed to append the where\n");
        destroy_where(where);
        return -1;
    }

    DList *entry_list = database_search(account_db->database, where_list);
    destroy_where_list(where_list);
    if (entry_list) {
        LOGD("Can't make the account\n");
        destroy_matched_list(entry_list);
        return -1;
    }

    int field_mask = database_get_field_mask(account_db->database);
    if (field_mask == 0) {
        LOGD("Field mask was wrong\n");
        return -1;
    }

    Stream_Buf *entry = account_db_new_entry(account, field_mask);
    if (!entry) {
        LOGD("Failed to new entry\n");
        return -1;
    }
    int id = database_get_last_id(account_db->database);
    
    id = database_add_entry(account_db->database, entry, id);
    destroy_stream_buf(entry);

    return id;
}

DList *account_db_get_accounts(AccountDB *account_db) {
    if (!account_db) {
        LOGD("There is nothing to point the MessageDB\n");
        return NULL;
    }

    int field_mask = database_get_field_mask(account_db->database);
    if (field_mask == 0) {
        LOGD("Field mask was wrong\n");
        return NULL;
    }

    int count = database_get_entry_count(account_db->database);
    if (count <= 0) {
        LOGD("Can't get entry\n");
        return NULL;
    }

    DList *account_list = NULL;
    Stream_Buf *entry;
    EntryPoint *entry_point;
    Account *account;

    for (int i = 0; i < count; i++) {
        entry_point = database_nth_entry_point(account_db->database, i);
        if (!entry_point) {
            LOGD("There is no the entry point matched id\n");
            continue;
        }
        entry = entry_point_get_value(entry_point);
        account = account_db_new_account(entry, field_mask);
        destroy_stream_buf(entry);
        if (!account) {
            LOGD("Failed to convert\n");
            continue;
        }

        account_list = d_list_append(account_list, account);
    }

    return account_list;
}

int account_db_delete_account(AccountDB *account_db, char *user_id, char *pw) {
    if (!account_db || !account_db->database || !user_id || !pw) {
        LOGD("Can't delete the account\n");
        return -1;
    }

    if (!strlen(user_id) || !strlen(pw)) {
        LOGD("Can't delete the account\n");
        return -1; 
    }

    Where *where = new_where(USER_ID, user_id);
    if (!where) {
        LOGD("Failed to new where\n");
        return -1;
    }
    DList *where_list = NULL;
    where_list = d_list_append(where_list, where);
    if (!where_list) {
        LOGD("Failed to add the where\n");
        destroy_where(where);
        return -1;
    }

    where = new_where(PW, pw);
    if (!where) {
        LOGD("Failed to new where\n");
        destroy_where_list(where_list);
        return -1;
    }
    where_list = d_list_append(where_list, where);
    
    DList *entry_list = database_search(account_db->database, where_list);
    destroy_where_list(where_list);
    if (!entry_list) {
        LOGD("The UserID is not present\n");
        return -1;
    }

    int len = d_list_length(entry_list);
    if (len == 0 || len > 1) {
        LOGD("Can't delete the account\n");
        destroy_matched_list(entry_list);
        return -1;
    }

    EntryPoint *entry = (EntryPoint *) d_list_get_data(entry_list);
    destroy_matched_list(entry_list);
    if (!entry) {
        LOGD("Can't delete the account\n");
        return -1;
    }

    int entry_id = entry_point_get_id(entry);
    if (entry_id < 0) {
        LOGD("Can't delete the entry\n");
        return -1;
    }

    BOOLEAN result = database_delete_entry(account_db->database, entry_id);
    if (result == FALSE) {
        return -1;
    }
    
    return entry_id;
}

int account_db_get_account_count(AccountDB *account_db) {
    if (!account_db) {
        LOGD("There is noting to point the AccountDB\n");
        return -1;
    }

    int count = database_get_entry_count(account_db->database);

    if (count < 0) {
        LOGD("Failed to get entry point count\n");
        return -1;
    }

    return count;
}

char *account_db_get_pw(AccountDB *account_db, char *user_id, char *confirm) {
    if (!account_db || !account_db->database || !user_id || !confirm) {
        LOGD("Can't get the pw\n");
        return NULL;
    }

    if (!strlen(user_id) || !strlen(confirm)) {
        LOGD("Can't get the pw\n");
        return NULL;
    }

    Where *where = new_where(USER_ID, user_id);
    if (!where) {
        LOGD("Failed to new where\n");
        return NULL;
    }
    DList *where_list = NULL;
    where_list = d_list_append(where_list, where);
    if (!where_list) {
        LOGD("Failed to add the where\n");
        destroy_where(where);
        return NULL;
    }

    where = new_where(CONFIRM, confirm);
    if (!where) {
        LOGD("Failed to new where\n");
        destroy_where_list(where_list);
        return NULL;
    }
    where_list = d_list_append(where_list, where);
    if (!where_list) {
        LOGD("Failed to add the where\n");
        destroy_where_list(where_list);
        return NULL;
    }

    DList *entry_list = database_search(account_db->database, where_list);
    destroy_where_list(where_list);
    if (!entry_list) {
        LOGD("The UserID is not present\n");
        return NULL;
    }

    int len = d_list_length(entry_list);
    if (len == 0 || len > 1) {
        LOGD("Can't delete the account\n");
        return NULL;
    }

    EntryPoint *entry_point = (EntryPoint *) d_list_get_data(entry_list);
    destroy_matched_list(entry_list);
    if (!entry_point) {
        LOGD("Can't find the account\n");
        return NULL;
    }

    int field_type;
    Stream_Buf *stream_buf = utils_get_data(account_db->database, entry_point, PW, &field_type);
    if (!stream_buf || field_type != STRING_FIELD) {
        return NULL;
    }

    char *buf = stream_buf_get_buf(stream_buf);
    if (!buf) {
        destroy_stream_buf(stream_buf);
        return NULL;
    }

    len = strlen(buf);
    if (len < 0) {
        LOGD("Can't find the password\n");
        destroy_stream_buf(stream_buf);
        return NULL;
    }
    
    char *pw = (char*) malloc(len + 1);
    memset(pw, 0, len + 1);
    strncpy(pw, buf, len);
    destroy_stream_buf(stream_buf);

    return pw;
}

int account_db_identify_account(AccountDB *account_db, char *user_id, char *pw) {
    if (!account_db || !account_db->database || !user_id || !pw) {
        LOGD("Can't identify the account\n");
        return -1;
    }

    if (!strlen(user_id) || !strlen(pw)) {
        LOGD("Can't identify the account\n");
        return -1;
    }

    Where *where = new_where(USER_ID, user_id);
    if (!where) {
        LOGD("Failed to new where\n");
        return -1;
    }
    DList *where_list = NULL;
    where_list = d_list_append(where_list, where);
    if (!where_list) {
        LOGD("Failed to add the where\n");
        destroy_where(where);
        return -1;
    }

    where = new_where(PW, pw);
    if (!where) {
        LOGD("Failed to new where\n");
        destroy_where_list(where_list);
        return -1;
    }
    where_list = d_list_append(where_list, where);

    DList *entry_list = database_search(account_db->database, where_list);
    destroy_where_list(where_list);
    if (!entry_list) {
        LOGD("The account which matches UserID and PW is not present\n");
        return -1;
    }

    int len = d_list_length(entry_list);
    destroy_matched_list(entry_list);
    if (len == 0 || len > 1) {
        LOGD("Failed to identify the account\n");
        return -1;
    }

    EntryPoint *entry_point = d_list_get_data(entry_list);
    if (!entry_point) {
        LOGD("Failed to get the entry_point\n");
        return -1;
    }

    return entry_point_get_id(entry_point);
}

Stream_Buf *account_db_get_data(AccountDB *account_db, int id, int data_type) {
    if (!account_db || id < 0) {
        LOGD("Can't get the data\n");
        return NULL;
    }

    EntryPoint *entry = database_find_entry_point(account_db->database, id);
    if (!entry) {
        LOGD("Can't get the data");
        return NULL;
    }
    int field_type = 0;
    return utils_get_data(account_db->database, entry, data_type, &field_type);
}


