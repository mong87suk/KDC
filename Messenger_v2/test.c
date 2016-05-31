#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <string.h>
#include <unistd.h>

#include "looper.h"
#include "utils.h"
#include "socket.h"

struct _Test
{
    int fd;
};

typedef struct _Test Test;

void handle_stdin_event(Test *test, int fd) {
    int max = 100;
    char a[max];
    char t[10];
    int i;

    LOGD("handle_stdin_event\n");

    for(i = 0; i < max; i++) {
        a[i] = 'a' + i;
    }

    read(fd, t, sizeof(t));

    write(test->fd, a, sizeof(a));
}

static void handle_events(int fd, void *user_data, int looper_event) {
    Test *test = (Test*) user_data;

    if (!test) {
        LOGD("There is no a pointer to Client\n");
        return;
    }

    if (looper_event & LOOPER_HUP_EVENT) {
    } else if (looper_event & LOOPER_IN_EVENT) {
        if (fd == STDIN_FILENO) {
            handle_stdin_event(test, fd);
        } else {
            LOGD("There is no fd to handle event\n");
            return;
        }
    } else {
        LOGD("There is no event to handle\n");
    }
}

int main() {
    Looper *looper;
    struct sockaddr_un addr;
    int fd;
    Test *test;

    looper = new_looper();
    test = (Test*) malloc(sizeof(Test));
    if ((fd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
        LOGD("socket error\n");
        return;
    }

    test->fd = fd;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, SOCKET_PATH, sizeof(addr.sun_path) -1);

    if (connect(fd, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
        LOGD("Connect error\n");
        return;
    }

    add_watcher(looper, STDIN_FILENO, handle_events, test, LOOPER_IN_EVENT | LOOPER_HUP_EVENT);
    run(looper);
}

