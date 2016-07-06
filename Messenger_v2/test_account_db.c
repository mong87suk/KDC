#include <assert.h>

#include "account.h"
#include "account_db.h"

int main() {
    Account *account;
    AccountDB *account_db;
    int state;
    char *data_format = "sssss";
    char *id = "hjeong";
    char *pw = "qwer1234";
    char *email = "mmm@hanmail.net";
    char *confirm = "1111";
    char *mobile = "01088254687";

    account = new_account(id, pw, email, confirm, mobile);
    assert(account);

    account_db = account_db_open(data_format);
    assert(account_db);

    state = account_db_add_account(account_db, account);
    assert(state);

    return 0;
}


