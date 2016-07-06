#ifndef __ACCOUNT_DB_H__
#define __ACCOUNT_DB_H__

#include "account.h"
#include "DBLinkedList.h"

#define ACCOUNT_DB "account_db"
#define ID          1
#define PW          2
#define EMAIL       3
#define CONFIRM     4
#define MOBILE      5

typedef struct _AccountDB AccountDB;

AccountDB* account_db_open(char *data_format);
void account_db_close(AccountDB* account_db);
void account_db_delete_all(AccountDB* account_db);
int account_db_add_account(AccountDB *account_db, Account *account);
DList* account_db_get_accounts(AccountDB *account_db);
int account_db_delete_account(AccountDB *account_db, char *id, char *pw);
int account_db_get_account_count(AccountDB *account_db);
Account* account_db_nth_account(AccountDB *account_db, int nth);

#endif
