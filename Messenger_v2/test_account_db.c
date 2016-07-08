#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>

#include "account.h"
#include "account_db.h"
#include "DBLinkedList.h"
#include "utils.h"

int main() {
    Account *account1;
    Account *account2;

    AccountDB *account_db;
    int state;
    int count;
    char *data_format = "sssss";
    char *id = "hjeong";
    char *pw = "qwer1234";
    char *error_pw;
    char *email = "mmm@hanmail.net";
    char *confirm = "1111";
    char *mobile = "01088254687";

    char *cmp_pw;

    account1 = new_account(id,pw, email, confirm, mobile);
    assert(account1);
    account_db = account_db_open(data_format);
    assert(account_db);

    count = account_db_get_account_count(account_db);
    state = account_db_add_account(account_db, account1);
    assert(account_db_get_account_count(account_db) == (count + 1));

    state = account_db_delete_account(account_db, id, pw);
    assert(account_db_get_account_count(account_db) == count);

    count = account_db_get_account_count(account_db);
    state = account_db_add_account(account_db, account1);
    assert(account_db_get_account_count(account_db) == (count + 1));

    count = account_db_get_account_count(account_db);
    state = account_db_add_account(account_db, account1);
    assert(state < 0);
    assert(account_db_get_account_count(account_db) == count);

    error_pw = "qwer223";
    state = account_db_delete_account(account_db, id, error_pw);
    assert(state < 0);

    cmp_pw = account_db_get_pw(account_db, id, confirm);
    assert(cmp_pw);
    assert(strncmp(cmp_pw, pw, strlen(pw)) == 0);
    free(cmp_pw);
    state = account_db_delete_account(account_db, id, pw); 

    return 0;
}
