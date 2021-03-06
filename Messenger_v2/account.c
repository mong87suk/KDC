#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "account.h"
#include "utils.h"

struct _Account {
    int id;
    char *user_id;
    char *pw;
    char *email;
    char *confirm;
    char *mobile;
};

Account *new_account(char *user_id, char *pw, char *email, char *confirm, char *mobile) {
    int str_len;
    Account *account;

    account = (Account*) malloc(sizeof(Account));
    if (!account) {
        LOGD("Failed to make the Account\n");
        return NULL;
    }

    account->id = -1;

    str_len = strlen(user_id);
    if (str_len < ID_MIN_SIZE || str_len > ID_MAX_SIZE) {
        LOGD("Failed to make the Account\n");
        free(account);
        return NULL;
    }
    account->user_id = (char*) malloc(str_len + 1);
    memset(account->user_id, 0, str_len + 1);
    strncpy(account->user_id, user_id, str_len);

    str_len = strlen(pw);
    if (str_len < PW_MIN_SIZE || str_len > PW_MAX_SIZE) {
        LOGD("Failed to make the Account\n");
        free(account);
        return NULL;
    }
    account->pw = (char*) malloc(str_len + 1);
    memset(account->pw, 0, str_len + 1);
    strncpy(account->pw, pw, str_len);

    str_len = strlen(email);
    if (str_len <= 0) {
        LOGD("Failed to make the Account\n");
        free(account);
        return NULL;
    }
    account->email = (char*) malloc(str_len + 1);
    memset(account->email, 0, str_len + 1);
    strncpy(account->email, email, str_len);

    str_len = strlen(confirm);
    if (str_len < CONFIRM_MIN_SIZE || str_len > CONFIRM_MAX_SIZE) {
        LOGD("Failed to make the Account\n");
        free(account);
        return NULL;
    }
    account->confirm = (char*) malloc(str_len + 1);
    memset(account->confirm, 0, str_len + 1);
    strncpy(account->confirm, confirm, str_len);

    str_len = strlen(mobile);
    if (str_len <= 0) {
        LOGD("Failed to make the Account\n");
        free(account);
        return NULL;
    }
    account->mobile = (char*) malloc(str_len + 1);
    memset(account->mobile, 0, str_len + 1);
    strncpy(account->mobile, mobile, str_len);

    return account;
}

void destroy_account(Account *account) {
    if (!account) {
        LOGD("Can't destroy the account\n");
        return;
    }

    if (account->user_id) {
        free(account->user_id);
    }

    if (account->pw) {
        free(account->pw);
    }

    if (account->email) {
        free(account->email);
    }

    if (account->confirm) {
        free(account->confirm);
    }

    if (account->mobile) {
        free(account->mobile);
    }

    free(account);
}

int account_get_id(Account *account) {
    if (!account) {
        LOGD("There is nothing to point the account\n");
        return -1;
    }

    return account->id;
}

char *account_get_user_id(Account *account) {
    if (!account) {
        LOGD("There is noting to point the account\n");
        return NULL;
    }

    return account->user_id;
}

char *account_get_pw(Account *account) {
    if (!account) {
        LOGD("There is noting to point the account\n");
        return NULL;
    }

    return account->pw;
}

char *account_get_email(Account *account) {
    if (!account) {
        LOGD("There is nothing to point the account\n");
        return NULL;
    }

   return account->email;
}

char *account_get_confirm(Account *account) {
    if (!account) {
        LOGD("There is nothing to point the account\n");
        return NULL;
    }

    return account->confirm;
}

char *account_get_mobile(Account *account) {
    if (!account) {
        LOGD("There is nothing to point the account\n");
        return NULL;
    }

    return account->mobile;
}

BOOLEAN account_set_id(Account *account, int id) {
    if (!account || id < 0) {
        LOGD("Can't set id\n");
        return FALSE;
    }
    account->id = id;

    return TRUE;
}
