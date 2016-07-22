#ifndef __ACCOUNT_DB_H__
#define __ACCOUNT_DB_H__

#include "account.h"
#include "DBLinkedList.h"

#define ACCOUNT_DATA_FORMAT "sssss"
#define ACCOUNT_DB          "account_db"
#define USER_ID             0
#define PW                  1
#define EMAIL               2
#define CONFIRM             3
#define MOBILE              4

typedef struct _AccountDB AccountDB;

AccountDB *account_db_open(char *data_format);
void account_db_close(AccountDB* account_db);
void account_db_delete_all(AccountDB* account_db);
int account_db_add_account(AccountDB *account_db, Account *account);
DList *account_db_get_accounts(AccountDB *account_db);
BOOLEAN account_db_delete_account(AccountDB *account_db, char *user_id, char *pw);
int account_db_get_account_count(AccountDB *account_db);
char *account_db_get_pw(AccountDB *account_db, char *id, char *confirm);
Account *account_db_find_account(AccountDB *account_db, char *user_id);
Account *account_db_identify_account(AccountDB *account_db, char *usr_id, char *pw);

#endif
