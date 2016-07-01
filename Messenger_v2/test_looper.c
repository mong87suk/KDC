#include <stdio.h>
#include <assert.h>
#include <stdlib.h>

#include "looper.h"
#include "utils.h"
struct _UserData {
    Looper *looper;
    int num;
};

typedef struct _UserData UserData;

static void print_data(void *user_data) {
    UserData *data;

    assert(user_data);

    data = (UserData*) user_data;
    LOGD("num:%d\n", data->num);
    looper_remove_timer(data->looper, user_data);
}

int main() {
    int state;
    int num1;
    int num2;

    UserData *user_data;
    Looper *looper;

    looper = new_looper();
    assert(looper);
    state = looper_run(looper);
    assert(state == 0);

    num1 = 10000;
    user_data = (UserData*) malloc(sizeof(UserData));
    assert(user_data);
    user_data->num = num1;
    user_data->looper = looper;
    looper_add_timer(looper, 10000, print_data, user_data);

    num2 = 5000;
    user_data = (UserData*) malloc(sizeof(UserData));
    assert(user_data);
    user_data->num = num2;
    user_data->looper = looper;
    looper_add_timer(looper, 5000, print_data, user_data);

    looper_run(looper);
    return 0;
}
