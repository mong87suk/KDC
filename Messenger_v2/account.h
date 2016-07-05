#ifndef __ACCOUNT_H__
#define __ACCOUNT_H__

#define ID_MAX_SIZE         20
#define ID_MIN_SIZE         4
#define PW_MAX_SIZE         20
#define PW_MIN_SIZE         4
#define EMAIL_SIZE          40
#define CONFIRM_MAX_SIZE    20
#define CONFIRM_MIN_SIZE    4
#define MOBILE_SIZE         20

typedef struct _Account Account;

Account* new_account(char *id, char *pw, char *email, char *confirm, char *mobile);
void destroy_account(Account *account);
char* account_get_id(Account *account);
char* account_get_pw(Account *account);
char* account_get_email(Account *account);
char* account_get_confirm(Account *account);
char* account_get_mobile(Account *account);
int account_get_id_len(Account *account);
int account_get_pw_len(Account *account);
int account_get_email_len(Account *account);
int account_get_confirm_len(Account *account);
int account_get_mobile_len(Account *account);

#endif
