#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>

#include <kdc/DBLinkedList.h>

#include "account.h"
#include "account_db.h"
#include "utils.h"

int main() {
    char *user_id = "hjeong";
    char *pw = "qwer1234";
    char *error_pw;
    char *email = "mmm@hanmail.net";
    char *confirm = "1111";
    char *mobile = "01088254687";

    Account *account1 = new_account(user_id, pw, email, confirm, mobile);
    assert(account1);
    assert(account_get_id(account1) == -1);

    AccountDB *account_db = account_db_open(ACCOUNT_DATA_FORMAT);
    assert(account_db);

    int count = account_db_get_account_count(account_db);
    int id = account_db_add_account(account_db, account1);
    assert(account_db_get_account_count(account_db) == (count + 1));
    
    BOOLEAN result = account_set_id(account1, id);
    assert(result > 0);

    assert(account_db_identify_account(account_db, user_id, "222") == -1);
    assert(account_db_identify_account(account_db, user_id, pw) == 0);

    id = account_db_delete_account(account_db, user_id, pw);
    assert(account_db_get_account_count(account_db) == count);

    count = account_db_get_account_count(account_db);
    id = account_db_add_account(account_db, account1);
    assert(account_db_get_account_count(account_db) == (count + 1));

    count = account_db_get_account_count(account_db);
    id = account_db_add_account(account_db, account1);
    assert(id < 0);
    assert(account_db_get_account_count(account_db) == count);
    
    error_pw = "qwer223";
    id = account_db_delete_account(account_db, user_id, error_pw);
    
    assert(id < 0);
    
    assert(account_db_get_account_count(account_db) == count);
    char *cmp_pw = account_db_get_pw(account_db, user_id, "2222");
    assert(cmp_pw == NULL);

    cmp_pw = account_db_get_pw(account_db, user_id, confirm);
    assert(cmp_pw);
    assert(strcmp(cmp_pw, pw) == 0);
    free(cmp_pw);
    result = account_db_delete_account(account_db, user_id, pw);
    assert(account_db_get_account_count(account_db) == (count - 1));

    account_db_delete_all(account_db);
    assert(account_db_get_account_count(account_db) == 0);

    destroy_account(account1);
    return 0;
}
