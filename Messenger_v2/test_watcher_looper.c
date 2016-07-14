#include <string.h>
#include <assert.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <fcntl.h>
#include <unistd.h>

#include "looper.h"
#include "utils.h"

#define BUF_SIZE 4
#define TEST     "Test"

void handle_event(int fd, void *user_data,unsigned int id, int looper_event) {
    int size;
    char buf[5];

    memset(buf, 0, sizeof(buf));
    size = read(fd, buf, BUF_SIZE);
    LOGD("buf:%s size:%d\n", buf, size);
    assert(size == BUF_SIZE);
    assert(strcmp(buf, TEST) == 0);
    LOGD("watcher_id:%d data:%d\n", id, *((int *) user_data));
}

int main(void) {
    int pipefd[2];
    int num1;
    int num2;
    int num3;
    int id;
    int size;

    Looper *looper;

    assert(pipe(pipefd) != -1);
    size = write(pipefd[1], TEST, strlen(TEST));
    assert(size == strlen(TEST));
    size = write(pipefd[1], TEST, BUF_SIZE);
    assert(size == BUF_SIZE);
    size = write(pipefd[1], TEST, BUF_SIZE);
    assert(size == BUF_SIZE);

    looper = new_looper();
    assert(looper);

    num1 = 1;
    id = looper_add_watcher(looper, pipefd[0], handle_event, &num1, LOOPER_IN_EVENT);
    assert(id);

    num2 = 2;
    id = looper_add_watcher(looper, pipefd[0], handle_event, &num2, LOOPER_IN_EVENT);
    assert(id);

    num3 = 3;
    id = looper_add_watcher(looper, pipefd[0], handle_event, &num3, LOOPER_IN_EVENT);
    assert(id);
        
    looper_run(looper);

    return 0;
}