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

static BOOLEAN print_data1(void *user_data, unsigned int id) {
    UserData *data;

    assert(user_data);

    data = (UserData*) user_data;
    LOGD("id:%d num:%d, time=%ld\n\n", id, data->num, time(NULL));
    
    return TRUE;
}

static BOOLEAN print_data2(void *user_data, unsigned int id) {
    UserData *data;

    assert(user_data);

    data = (UserData*) user_data;
    LOGD("id:%d num:%d, time=%ld\n\n", id, data->num, time(NULL));
    
    return FALSE;
}


int main() {
    int state;
    int num1;
    int num2;
    int num3;
    int num4;
    int num5;
    int id;
   
    UserData user_data1;
    UserData user_data2;
    UserData user_data3;
    UserData user_data4;
    UserData user_data5;
    
    Looper *looper;

    looper = new_looper();
    assert(looper);
    state = looper_run(looper);
    assert(state == 0);

    num1 = 1000;
    user_data1.num = num1;
    user_data1.looper = looper;
    id = looper_add_timer(looper, num1, print_data1, &user_data1);
    assert(id);

    num2 = 2000;
    user_data2.num = num2;
    user_data2.looper = looper;
    id = looper_add_timer(looper, num2, print_data1, &user_data2);
    assert(id);

    num3 = 3000;
    user_data3.num = num3;  
    user_data3.looper = looper;
    id = looper_add_timer(looper, num3, print_data2, &user_data3);
    assert(id);

    num4 = 4000;
    user_data4.num = num4;  
    user_data4.looper = looper;
    id = looper_add_timer(looper, num4, print_data2, &user_data4);
    assert(id);

    num5 = 5000;
    user_data5.num = num5;  
    user_data5.looper = looper;
    id = looper_add_timer(looper, num5, print_data1, &user_data5);
    assert(id);

    looper_run(looper);
    return 0;
}