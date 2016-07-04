#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <time.h>

#include "looper.h"
#include "utils.h"
#include "m_boolean.h"

struct _UserData {
    Looper *looper;
    int num;
};

typedef struct _UserData UserData;

static BOOLEAN print_data(void *user_data) {
    UserData *data;

    assert(user_data);

    data = (UserData*) user_data;
    LOGD("num:%d, time=%ld\n\n", data->num, time(NULL));

    return TRUE;
}

int main() {
    int state;
    int num1;
    int num2;
    int num3;

    UserData user_data1;
    UserData user_data2;
    Looper *looper;

    looper = new_looper();
    assert(looper);
    state = looper_run(looper);
    assert(state == 0);

    num1 = 4000;
    user_data1.num = num1;
    user_data1.looper = looper;
    looper_add_timer(looper, 4000, print_data, &user_data1);

    num2 = 2000;
    user_data2.num = num2;
    user_data2.looper = looper;
    looper_add_timer(looper, 2000, print_data, &user_data2);
/*
    num3 = 0;
    user_data = (UserData*) malloc(sizeof(UserData));
    assert(user_data);
    user_data->num = num3;
    user_data->looper = looper;
    looper_add_timer(looper, 0, print_data, user_data); */

    looper_run(looper);
    return 0;
}
