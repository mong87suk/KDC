#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "account.h"
#include "utils.h"

struct _Account {
    char id[20];
    char pw[20];
    char email[40];
    char confirm[20];
    char mobile[20];

    int id_len;
    int pw_len;
    int email_len;
    int confirm_len;
    int mobile_len;
};

Account* new_account(char *id, char *pw, char *email, char *confirm, char *mobile) {
    int str_len;
    Account *account;

    account = (Account*) malloc(sizeof(Account));
    if (!account) {
        LOGD("Failed to make the Account\n");
        return NULL;
    }

    str_len = strlen(id);
    if (str_len < ID_MIN_SIZE || str_len > ID_MAX_SIZE) {
        LOGD("Failed to make the Account\n");
        free(account);
        return NULL;
    }
    account->id_len = str_len;
    memcpy(account->id, id, str_len);

    str_len = strlen(pw);
    if (str_len < PW_MIN_SIZE || str_len > PW_MAX_SIZE) {
        LOGD("Failed to make the Account\n");
        free(account);
        return NULL;
    }
    account->pw_len = str_len;
    memcpy(account->pw, pw, str_len);

    str_len = strlen(email);
    if (str_len <= 0) {
        LOGD("Failed to make the Account\n");
        free(account);
        return NULL;
    }
    account->email_len = str_len;
    memcpy(account->email, email, str_len);

    str_len = strlen(confirm);
    if (str_len < CONFIRM_MIN_SIZE || str_len > CONFIRM_MAX_SIZE) {
        LOGD("Failed to make the Account\n");
        free(account);
        return NULL;
    }
    account->confirm_len = str_len;
    memcpy(account->confirm, confirm, str_len);

    str_len = strlen(mobile);
    if (str_len <= 0) {
        LOGD("Failed to make the Account\n");
        free(account);
        return NULL;
    }
    account->mobile_len = str_len;
    memcpy(account->mobile, mobile, str_len);

    return account;
}

void destroy_account(Account *account) {
    if (!account) {
        LOGD("Can't destroy the account\n");
        return;
    }

    free(account);
}

char* account_get_id(Account *account) {
    if (!account) {
        LOGD("There is noting to point the account\n");
        return NULL;
    }

    return account->id;
}

char* account_get_pw(Account *account) {
    if (!account) {
        LOGD("There is noting to point the account\n");
        return NULL;
    }

    return account->pw;
}

char* account_get_email(Account *account) {
    if (!account) {
        LOGD("There is nothing to point the account\n");
        return NULL;
    }

    return account->email;
}

char* account_get_confirm(Account *account) {
    if (!account) {
        LOGD("There is nothing to point the account\n");
        return NULL;
    }

    return account->confirm;
}

char* account_get_mobile(Account *account) {
    if (!account) {
        LOGD("There is nothing to point the account\n");
        return NULL;
    }

    return account->mobile;
}

int account_get_id_len(Account *account) {
    if (!account) {
        LOGD("There is noting to point the account\n");
        return -1;
    }

    return account->id_len;
}

int account_get_pw_len(Account *account) {
    if (!account) {
        LOGD("There is nothing to point the account\n");
        return -1;
    }

    return account->pw_len;
}

int account_get_email_len(Account *account) {
    if (!account) {
        LOGD("There is nothing to point the account\n");
        return -1;
    }

    return account->email_len;
}

int account_get_confirm_len(Account *account) {
    if (!account) {
        LOGD("There is nothing to point the account\n");
        return -1;
    }

    return account->confirm_len;
}

int account_get_mobile_len(Account *account) {
    if (!account) {
        LOGD("There is noting to point the account\n");
        return -1;
    }

    return account->mobile_len;
}
