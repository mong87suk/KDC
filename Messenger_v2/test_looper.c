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

static BOOLEAN print_data(void *user_data, unsigned int id) {
    UserData *data;

    assert(user_data);

    data = (UserData*) user_data;
    LOGD("id:%d num:%d, time=%ld\n\n", id, data->num, time(NULL));
    
    return TRUE;
}

int main() {
    int state;
    int num1;
    int num2;
    int num3;
    int id1;
    int id2;
    int id3;

    UserData user_data1;
    UserData user_data2;
    UserData user_data3;
    Looper *looper;

    looper = new_looper();
    assert(looper);
    state = looper_run(looper);
    assert(state == 0);

    num1 = 4000;
    user_data1.num = num1;
    user_data1.looper = looper;
    id1 = looper_add_timer(looper, 4000, print_data, &user_data1);

    num2 = 2000;
    user_data2.num = num2;
    user_data2.looper = looper;
    id2 = looper_add_timer(looper, 2000, print_data, &user_data2);

    num3 = 0;
    user_data3.num = num3;  
    user_data3.looper = looper;
    id3 = looper_add_timer(looper, 0, print_data, &user_data3);

    looper_remove_timer_with_id(looper, id3);

    looper_run(looper);
    return 0;
}