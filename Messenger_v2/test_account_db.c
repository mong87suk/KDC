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
    int id;
    int count;
    char *user_id = "hjeong";
    char *pw = "qwer1234";
    char *error_pw;
    char *email = "mmm@hanmail.net";
    char *confirm = "1111";
    char *mobile = "01088254687";
    char *cmp_pw;
    int result;

    account1 = new_account(user_id, pw, email, confirm, mobile);
    assert(account1);
    assert(account_get_id(account1) == -1);

    account_db = account_db_open(ACCOUNT_DATA_FORMAT);
    assert(account_db);

    count = account_db_get_account_count(account_db);
    id = account_db_add_account(account_db, account1);
    assert(id > 0);
    assert(account_db_get_account_count(account_db) == (count + 1));
    
    result = account_set_id(account1, id);
    assert(result == TRUE);

    result = account_db_identify_account(account_db, user_id, "222");
    assert(result == FALSE);

    result = account_db_identify_account(account_db, user_id, pw);
    assert(result == TRUE);

    account2 = account_db_find_account(account_db, user_id);
    assert(account_get_id(account2) == id);
    assert(strcmp(account_get_user_id(account2), user_id) == 0);
    assert(strcmp(account_get_pw(account2), pw) == 0);
    assert(strcmp(account_get_email(account2), email) == 0);
    assert(strcmp(account_get_confirm(account2), confirm) == 0);
    assert(strcmp(account_get_mobile(account2), mobile) == 0);

    account_db_delete_account(account_db, user_id, pw);
    assert(account_db_get_account_count(account_db) == count);

    count = account_db_get_account_count(account_db);
    id = account_db_add_account(account_db, account1);
    assert(account_db_get_account_count(account_db) == (count + 1));

    count = account_db_get_account_count(account_db);
    result = account_db_add_account(account_db, account1);
    assert(result < 0);
    assert(account_db_get_account_count(account_db) == count);

    error_pw = "qwer223";
    account_db_delete_account(account_db, user_id, error_pw);
    assert(account_db_get_account_count(account_db) == count);

    cmp_pw = account_db_get_pw(account_db, user_id, "2222");
    assert(cmp_pw == NULL);

    cmp_pw = account_db_get_pw(account_db, user_id, confirm);
    assert(cmp_pw);
    assert(strcmp(cmp_pw, pw) == 0);
    free(cmp_pw);
    account_db_delete_account(account_db, user_id, pw); 
    assert(account_db_get_account_count(account_db) == (count - 1));

    destroy_account(account1);
    destroy_account(account2);
    return 0;
}
