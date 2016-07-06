#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "account.h"
#include "utils.h"

struct _Account {
    char id[20];
    char pw[40];
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
    LOGD("ACCOUNT ADDRESS %x\n", account);
    LOGD("account size:%d\n", sizeof(Account));
    if (!account) {
        LOGD("Failed to make the Account\n");
        return NULL;
    }

    str_len = strlen(id);
    LOGD("id:%d\n", str_len);   
    if (str_len < ID_MIN_SIZE || str_len > ID_MAX_SIZE) {
        LOGD("Failed to make the Account\n");
        free(account);
        return NULL;
    }
    memset(account->id, 0, sizeof(account->id));
    strncpy(account->id,id,str_len);
    account->id_len=str_len;
    /*
    malloc
    strcmp(account->id, id, str_len);

    */

    str_len = strlen(pw);
    LOGD("pw:%d\n", str_len);   
    if (str_len < PW_MIN_SIZE || str_len > PW_MAX_SIZE) {
        LOGD("Failed to make the Account\n");
        free(account);
        return NULL;
    }
    account->pw_len = str_len;
    memset(account->pw, 0, 40);
    strncpy(account->pw, pw, str_len);

    str_len = strlen(email);
    LOGD("email:%d\n", str_len);    
    if (str_len <= 0) {
        LOGD("Failed to make the Account\n");
        free(account);
        return NULL;
    }
    account->email_len = str_len;
    memset(account->email, 0, sizeof(account->email));
    strncpy(account->email, email, str_len);

    str_len = strlen(confirm);
    LOGD("confirm:%d\n", str_len);
    if (str_len < CONFIRM_MIN_SIZE || str_len > CONFIRM_MAX_SIZE) {
        LOGD("Failed to make the Account\n");
        free(account);
        return NULL;
    }
    account->confirm_len = str_len;
    memset(account->confirm, 0, sizeof(account->confirm));
    strncpy(account->confirm, confirm, str_len);

    str_len = strlen(mobile);
    LOGD("mobile:%d\n", str_len);
    if (str_len <= 0) {
        LOGD("Failed to make the Account\n");
        free(account);
        return NULL;
    }
    account->mobile_len = str_len;
    memset(account->mobile, 0, sizeof(account->mobile));
    strncpy(account->mobile, mobile, str_len);

    LOGD("account id:%s\n", account->id);
    LOGD("account pw:%s\n", account->pw);
    LOGD("account email:%s\n", account->email);
    LOGD("account confirm:%s\n", account->confirm);
    LOGD("account mobile:%s\n", account->mobile);
    return account;
}

void destroy_account(Account *account) {
    if (!account) {
        LOGD("Can't destroy the account\n");
        return;
    }

    LOGD("account:%0x\n", account);
    LOGD("id:%s pw:%s email:%s confirm:%s mobile:%s \n", account_get_id(account), account_get_pw(account), account_get_email(account), account_get_confirm(account), account_get_mobile(account));
    LOGD("id size:%ld\n", sizeof(account->id));
    LOGD("pw size:%ld\n", sizeof(account->pw));
    LOGD("email size:%ld\n", sizeof(account->email));
    LOGD("confirm size:%ld\n", sizeof(account->confirm));
    LOGD("mobile size:%ld\n", sizeof(account->mobile));
    LOGD("id_len:%ld\n", sizeof(account->id_len));
    LOGD("pw_len:%ld\n", sizeof(account->pw_len));
    LOGD("email_len:%ld\n", sizeof(account->email_len));
    LOGD("confirm_len:%ld\n", sizeof(account->confirm_len));
    LOGD("account_len:%ld\n", sizeof(account->mobile_len));


    LOGD("before destroy_account\n");
    LOGD("size:%d\n", sizeof(Account));
    free(account);
    LOGD("after destroy_account\n");
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
