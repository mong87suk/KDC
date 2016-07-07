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
    Account *account3;
    AccountDB *account_db;
    int state;
    int count;
    char *data_format = "sssss";
    char *id = "hjeong";
    char *pw = "qwer1234";
    char *email = "mmm@hanmail.net";
    char *confirm = "1111";
    char *mobile = "01088254687";

    char *test_data_format;
    char *test_id;
    char *test_pw;
    char *test_email;
    char *test_confirm;
    char *test_mobile;


    test_id = (char*) malloc(strlen(id) + 1);
    memset(test_id, 0, strlen(id) + 1);
    strncpy(test_id, id, strlen(id));
     
    test_pw = (char*) malloc(strlen(pw) + 1);
    memset(test_pw, 0, strlen(pw) + 1);
    strncpy(test_pw, pw, strlen(pw));

    test_email = (char*) malloc(strlen(email) + 1);
    memset(test_email, 0, strlen(email) + 1);
    strncpy(test_email, pw, strlen(email));
    
    test_confirm = (char*) malloc(strlen(confirm) + 1);
    memset(test_confirm, 0, strlen(confirm) + 1);
    strncpy(test_confirm, confirm, strlen(confirm));

    test_mobile = (char*) malloc(strlen(mobile) + 1);
    memset(test_mobile, 0, strlen(mobile) + 1);
    strncpy(test_mobile, mobile, strlen(mobile));

    account1 = new_account(test_id, test_pw, test_email, test_confirm, test_mobile);
    assert(account1);
    account_db = account_db_open(data_format);
    assert(account_db);
    count = account_db_get_account_count(account_db);
    state = account_db_add_account(account_db, account1);
    LOGD("account db add\n");
    assert(state);
    assert(account_db_get_account_count(account_db) == (count + 1));
    account3 = account_db_nth_account(account_db, 0);
    LOGD("count:%d\n", count);
/*
    account2 = account_db_nth_account(account_db, count);
    assert(account2);
    assert(strncmp(id, account_get_id(account2), strlen(id)) == 0);
    assert(strncmp(pw, account_get_pw(account2), strlen(pw)) == 0);

    assert(strncmp(email, account_get_email(account2), strlen(email)) == 0);
    assert(strncmp(confirm, account_get_confirm(account2), strlen(confirm)) == 0);
    assert(strncmp(mobile, account_get_mobile(account2), strlen(mobile)) == 0); 
*/
    LOGD("before delte account db\n");
    state = account_db_delete_account(account_db, test_id, test_pw);

    /*
    LOGD("after account db delte\n");
    LOGD("after account db delte\n");
    assert(account_db_get_account_count(account_db) == count); */

    return 0;
}
