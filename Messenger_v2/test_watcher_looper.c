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
    int data;

    data = *((int *) user_data);

    memset(buf, 0, sizeof(buf));
    size = read(fd, buf, BUF_SIZE);
    LOGD("fd:%d buf:%s size:%d\n", fd, buf, size);
    assert(size == BUF_SIZE);
    assert(strcmp(buf, TEST) == 0);
    assert(data == fd);
}

int main(void) {
    int pipefd1[2];
    int pipefd2[2];
    int pipefd3[2];
    int num1;
    int num2;
    int num3;
    int num4;
    int num5;
    int id;
    int size;

    Looper *looper;

    assert(pipe(pipefd1) != -1);
    assert(pipe(pipefd2) != -1);
    assert(pipe(pipefd3) != -1);

    size = write(pipefd1[1], TEST, strlen(TEST));
    assert(size == strlen(TEST));
    size = write(pipefd1[1], TEST, BUF_SIZE);
    assert(size == BUF_SIZE);
    size = write(pipefd1[1], TEST, BUF_SIZE);
    assert(size == BUF_SIZE);

    size = write(pipefd2[1], TEST, strlen(TEST));
    assert(size == strlen(TEST));

    size = write(pipefd3[1], TEST, strlen(TEST));
    assert(size == strlen(TEST));

    looper = new_looper();
    assert(looper);

    num1 = pipefd1[0];
    id = looper_add_watcher(looper, pipefd1[0], handle_event, &num1, LOOPER_IN_EVENT);
    assert(id);

    num2 = pipefd1[0];
    id = looper_add_watcher(looper, pipefd1[0], handle_event, &num2, LOOPER_IN_EVENT);
    assert(id);

    num3 = pipefd1[0];
    id = looper_add_watcher(looper, pipefd1[0], handle_event, &num3, LOOPER_IN_EVENT);
    assert(id);

    num4 = pipefd2[0];
    id = looper_add_watcher(looper, pipefd2[0], handle_event, &num4, LOOPER_IN_EVENT);
    assert(id);

    num5 = pipefd3[0];
    id = looper_add_watcher(looper, pipefd3[0], handle_event, &num5, LOOPER_IN_EVENT);
    assert(id);
        
    looper_run(looper);

    return 0;
}