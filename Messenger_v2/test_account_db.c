#include <stdio.h>
#include <assert.h>
#include <string.h>

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
    char *email = "mmm@hanmail.net";
    char *confirm = "1111";
    char *mobile = "01088254687";

    account1 = new_account(id, pw, email, confirm, mobile);
    assert(account1);
    account_db = account_db_open(data_format);
    assert(account_db);
    count = account_db_get_account_count(account_db);
    state = account_db_add_account(account_db, account1);
    LOGD("account db add\n");
    assert(state);
    assert(account_db_get_account_count(account_db) == (count + 1));

    LOGD("count:%d\n", count);
   /* 
    account2 = account_db_nth_account(account_db, count);
    assert(account2);
    assert(strncmp(id, account_get_id(account2), account_get_id_len(account2)) == 0);
    assert(strncmp(pw, account_get_pw(account2), account_get_pw_len(account2)) == 0);
    
    assert(strncmp(email, account_get_email(account2), account_get_email_len(account2)) == 0);
    assert(strncmp(confirm, account_get_confirm(account2), account_get_confirm_len(account2)) == 0);
    assert(strncmp(mobile, account_get_mobile(account2), account_get_mobile_len(account2)) == 0);
    
    LOGD("before delte account db\n"); */
    state = account_db_delete_account(account_db, id, pw);
    /*
    LOGD("after account db delte\n");
    LOGD("after account db delte\n");
    assert(account_db_get_account_count(account_db) == count); */

    return 0;
}
