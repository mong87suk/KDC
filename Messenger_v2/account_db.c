#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "account_db.h"
#include "account.h"
#include "database.h"
#include "utils.h"
#include "DBLinkedList.h"
#include "stream_buf.h"
#include "entry_point.h"

struct _AccountDB {
    DataBase *database;
};

static Stream_Buf *account_db_new_account_info_buf(char *info, int len) {
    Stream_Buf *stream_buf;

    if (!info) {
        LOGD("Can't new account info buf\n");
        return NULL;
    }

    if (len <= 0) {
        LOGD("Can't new account info buf\n");
        return NULL;
    }

    stream_buf = new_stream_buf(sizeof(int) + len);
    if (!stream_buf) {
        LOGD("Failed to make the StreamBuf\n");
        return NULL;
    }

    memcpy(stream_buf_get_available(stream_buf), &len, sizeof(int));
    stream_buf_increase_pos(stream_buf, sizeof(int));

    memcpy(stream_buf_get_available(stream_buf), info, len);
    stream_buf_increase_pos(stream_buf, len);

    return stream_buf;
}

static Stream_Buf *account_db_new_entry(Account *account, int field_mask) {
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

static Account *account_db_new_account(Stream_Buf *entry, int field_mask) {
    int id;
    int len;
    int i;
    int index;
    int colum, count;
    char *buf;
    char *user_id, *pw, *email, *confirm, *mobile;
    int result;

    Account* account;

    account = NULL;
    if (!entry) {
        LOGD("There is nothing to point the entry\n");
        return NULL;
    }

    count = utils_get_colum_count(field_mask);
    if (count <= 0) {
        LOGD("Failed to get colum count\n");
        return NULL;
    }
    count -= 1;
    index = 0;

    buf = stream_buf_get_buf(entry);
    if (!buf) {
        LOGD("Failed to get the buf\n");
        return NULL;
    }

    memcpy(&id, buf, ID_SIZE);
    if (id < 0) {
        LOGD("ID was wrong\n");
        return NULL;
    }
    buf += ID_SIZE;

    for (i = count; i >= 0; i--) {
        index += 1;
        colum = (field_mask >> (FIELD_SIZE * i)) & FIELD_TYPE_FLAG;
        if (colum == STRING_FIELD) {
            switch(index) {
            case USER_ID:
                memcpy(&len, buf, sizeof(int));
                if (len < 0) {
                    LOGD("Failed to get len\n");
                    return NULL;
                }
                buf += sizeof(int);

                user_id = (char*) malloc(len + 1);
                if (!id) {
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
    }
    account = new_account(user_id, pw, email, confirm, mobile);
    
    free(user_id);
    free(pw);
    free(email);
    free(confirm);
    free(mobile);

    if (!account) {
        LOGD("Failed to make the Account\n");
        return NULL;
    }

    result = account_set_id(account, id);
    if (result == FALSE) {
        LOGD("Failed to set id\n");
        destroy_account(account);
        return NULL;
    }

    return account;
}

static Account *account_db_nth_account(AccountDB *account_db, int nth) {
    int field_mask;

    EntryPoint *entry_point;
    Stream_Buf *entry;
    Account *account;

    if (!account_db) {
        LOGD("There is nothing to point the AccountDB\n");
        return NULL;
    }

    field_mask = database_get_field_mask(account_db->database);
    if (field_mask == 0) {
        LOGD("Field mask was wrong\n");
        return NULL;
    }

    entry_point = database_nth_entry_point(account_db->database, nth);
    if (!entry_point) {
        LOGD("Faied to nth entry point\n");
        return NULL;
    }

    entry = entry_point_get_value(entry_point);
    account = account_db_new_account(entry, field_mask);
    if (!account) {
        LOGD("Failed to new account\n");
        return NULL;
    }
    destroy_stream_buf(entry);

    return account;
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
    int count;
    char *cmp_id;
    int entry_id;
    int i;

    EntryPoint *entry_point;
    Stream_Buf *entry;
    Account *cmp_account;

    if (!account_db || !account) {
        LOGD("Can't add the account\n");
        return -1;
    }

    field_mask = database_get_field_mask(account_db->database);
    if (field_mask == 0) {
        LOGD("Field mask was wrong\n");
        return -1;
    }

    count = database_get_entry_count(account_db->database);
    for (i = 0; i < count; i++) {
        entry_point = database_nth_entry_point(account_db->database, i);
        if (!entry_point) {
            LOGD("There is no the entry point matched id\n");
            continue;
        }
        entry = entry_point_get_value(entry_point);
        cmp_account = account_db_new_account(entry, field_mask);
        destroy_stream_buf(entry);
        if (!cmp_account) {
            LOGD("Failed to convert\n");
            continue;
        }
        cmp_id = account_get_user_id(cmp_account);
        if(strncmp(cmp_id, account_get_user_id(account), strlen(cmp_id)) == 0) {
            LOGD("The ID has aleady been existed\n");
            destroy_account(cmp_account);
            return -1;
        }
        destroy_account(cmp_account);
    }

    entry = account_db_new_entry(account, field_mask);
    if (!entry) {
        LOGD("Failed to new entry\n");
        return -1;
    }

    entry_id = database_add_entry(account_db->database, entry);
    destroy_stream_buf(entry);
    if (entry_id < 0) {
        LOGD("Failed to add_entry\n");
        return -1;
    }
    return entry_id;
}

DList* account_db_get_accounts(AccountDB *account_db) {
    int i;
    int field_mask;
    int count;
    DList *account_list;
    EntryPoint *entry_point;
    Stream_Buf *entry;
    Account *account;

    if (!account_db) {
        LOGD("There is nothing to point the MessageDB\n");
        return NULL;
    }

    field_mask = database_get_field_mask(account_db->database);
    if (field_mask == 0) {
        LOGD("Field mask was wrong\n");
        return NULL;
    }

    account_list = NULL;
    count = database_get_entry_count(account_db->database);
    if (count <= 0) {
        LOGD("Can't get entry\n");
        return NULL;
    }

    for (i = 0; i < count; i++) {
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

Account *account_db_find_account(AccountDB *account_db, char *user_id) {
    int i;
    int count;
    char *cmp_id;

    Account *account;
    
    if (!account_db || !user_id) {
        LOGD("Can't find the account\n");
        return NULL;
    }

    if (strlen(user_id) == 0) {
        LOGD("Can't find the account\n");
        return NULL;
    }

    count = database_get_entry_count(account_db->database);
    if (count <= 0) {
        LOGD("Can't get entry\n");
        return NULL;
    }

    for (i = 0; i < count; i++) {
        account = account_db_nth_account(account_db, i); 
        if (!account) {
            LOGD("Failed to new account\n");
            continue;
        }
        cmp_id = account_get_user_id(account);
        if (strcmp(cmp_id, user_id) == 0) {
            break;
        }
        destroy_account(account);
        account = NULL;
    }
    return account;
}

void account_db_delete_account(AccountDB *account_db, char *user_id, char *pw) {
    int id;
    char *cmp_pw;

    BOOLEAN result;
    Account *account;

    account = account_db_find_account(account_db, user_id);
    if (!account) {
        LOGD("Failed to find account\n");
        return;
    }

    cmp_pw = account_get_pw(account);
    if (strcmp(pw, cmp_pw) == 0) {
        id = account_get_id(account);
        result = database_delete_entry(account_db->database, id);
         destroy_account(account);
        if (result == FALSE) {
            LOGD("Failed to delte the entry\n");
        }
    }
}

int account_db_get_account_count(AccountDB *account_db) {
    int count;

    if (!account_db) {
        LOGD("There is noting to point the AccountDB\n");
        return -1;
    }

    count = database_get_entry_count(account_db->database);

    if (count < 0) {
        LOGD("Failed to get entry point count\n");
        return -1;
    }

    return count;
}

char *account_db_get_pw(AccountDB *account_db, char *user_id, char *confirm) {
    int len;
    char *cmp_confirm;
    char *pw;
    char *tmp;

    Account *account;
    pw = NULL;

    account = account_db_find_account(account_db, user_id);
    if (!account) {
        LOGD("Failed to find account\n");
        return NULL;
    }

    cmp_confirm = account_get_confirm(account);
    if (!cmp_confirm) {
        LOGD("Failed to get confirm\n");
        return NULL;
    }

    if (strcmp(confirm, cmp_confirm) == 0) {
        pw = account_get_pw(account);
        len = strlen(pw);
        tmp = (char*) malloc(len + 1);
        if (!tmp) {
            LOGD("Failed to create buf for pw\n");
            destroy_account(account);
            return NULL;
        }
        memset(tmp, 0, len + 1);
        strncpy(tmp, pw, len);
        pw = tmp;
    }

    destroy_account(account);
    return pw;
}

BOOLEAN account_db_identify_account(AccountDB *account_db, char *user_id, char *pw) {
    Account *account;
    char *cmp_pw;

    account = account_db_find_account(account_db, user_id);
    if (!account) {
        LOGD("Failed to find account\n");
        return FALSE;
    }

    cmp_pw = account_get_pw(account);
    if (!cmp_pw) {
        LOGD("Failed to get the pw\n");
        return FALSE;
    }

    if (strcmp(cmp_pw, pw) == 0) {
        return TRUE;
    }

    return FALSE;
}
