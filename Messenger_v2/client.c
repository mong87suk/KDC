#include <stdio.h>
#include <poll.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <sys/un.h>
#include <string.h>

#include "client.h"
#include "looper.h"
#include "socket.h"
#include "DBLinkedList.h"

struct _Client
{
    int fd;
    Looper *looper;
};

static int get_buffer_size(Client *client) {
    DList *list, *next;
    int size;
    char *buf;

    size = 0;
    list = client->buffer_list;

    while (list) {
        next = d_list_next(list);
        buf = (char*) d_list_get_data(list);
        if (!buf) {
            printf("There is no a pointer to buf\n");
            break;
        }
        size += strlen(buf);
        list = next;
    }
    return size;
}

static void copy_data(void *user_data, void *data) {
    char *dest, *src;
    int beginIndex, n;

    dest = (char*) user_data;
    src = (char*) data;

    beginIndex = strlen(user_data);
    n = strlen(src);

    memcpy(dest + beginIndex, src, n);
}

static void copy_buffer_list_data(Client *client, char *buf) {
    DList *list;
    list = client->buffer_list;

    if (!list) {
        printf("There is no a pointer to buffer\n");
        return;
    }
    d_list_foreach(list, copy_data, buf);
    d_list_free(list, free);
}

static void read_packet(int fd) {

}

static void handle_res_events(Client *client, int fd) {
    read_packet(fd);
}

static void handle_disconnect(Client *client, int fd) {

}

static char* create_req_all_mesg_packet() {
    printf("create_req_all_mesg_packet\n");
    return NULL;
}

static char* create_req_snd_mesg_packet(char *input_str) {
    printf("create_req_snd_mesg_packet\n");
    return NULL;
}

static char* create_req_first_or_last_mesg_packet(char *input_str) {
    printf("create_req_first_or_last_mesg_packet\n");
    return NULL;
}

static void handle_stdin_event(Client* client, int fd) {
    char *buf, *input_str;
    int n_byte;
    int input_size;
    char request_num;
    DList *stream_buf_list;

    buf = (char*) malloc(BUF_SIZE);
    if (!buf) {
        printf("There is no a pointer to buf");
        return;
    }
    memset(buf, 0, BUF_SIZE);
    client->buffer_list = NULL;

    while ((n_byte = read(fd, buf, BUF_SIZE - 1))) {
        client->buffer_list = d_list_append(client->buffer_list, buf);
        if (buf[n_byte - 1] == '\n') {
            break;
        }

        buf = (char*) malloc(BUF_SIZE);
        if (!buf) {
            printf("There is no a pointer to buf");
            return;
        }
        memset(buf, 0, BUF_SIZE);
    }

    input_size = get_buffer_size(client);
    input_str = (char*) malloc(input_size);
    printf("input_size:%d\n", input_size);

    if (!input_str) {
        printf("Failed to make input str\n");
        return;
    }

    memset(input_str, 0, input_size);
    copy_buffer_list_data(client, input_str);

    if (input_size >= REQ_STR_MIN_LEN && (strncasecmp(input_str, REQ_STR, strlen(REQ_STR))) == 0) {
        printf("input_str:%s", input_str);
        request_num = input_str[8];

        switch(request_num) {
            case '1':
                if (input_size == REQ_STR_MIN_LEN) {
                    create_req_all_mesg_packet();
                }
                break;
            case '3':
                if (input_size > REQ_STR_MIN_LEN && (input_str[REQ_STR_MIN_LEN - 1] == ' ')) {
                    create_req_snd_mesg_packet(input_str);
                }
                break;
            case '5':
                if (input_size == REQ_FIRST_OR_LAST_MESG_PACKET_SIZE && (input_str[REQ_STR_MIN_LEN - 1] == ' ')) {
                    create_req_first_or_last_mesg_packet(input_str);
                }
                break;
            default:
                printf("Your request number is %c Please recommand\n", request_num);
        }
    }
}

static void handle_events(int fd, void *user_data, int revents) {
    Client *client = (Client*) user_data;

    if (!client) {
        printf("There is no a pointer to Client\n");
        return;
    }

    if (revents & POLLHUP) {
        handle_disconnect(client, fd);
    } else if (revents & POLLIN) {
        if (fd == client->fd) {
            handle_res_events(client, fd);
        } else if (fd == STDIN_FILENO) {
            handle_stdin_event(client, fd);
        } else {
            printf("There is no fd to handle event\n");
            return;
        }
    } else {
        printf("There is no event to handle\n");
    }
}

Client* new_client(Looper *looper) {
    Client *client;

    struct sockaddr_un addr;
    int client_fd;

    if (!looper) {
        printf("There is no a pointer to Looper\n");
        return NULL;
    }

    client = (Client*) malloc(sizeof(Client));

    if (!client) {
        printf("Failed to make client\n");
        return NULL;
    }

    client->looper = looper;

    if ((client_fd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
        perror("socket error");
        exit(-1);
    }

    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, SOCKET_PATH, sizeof(addr.sun_path) - 1);

    if (connect(client_fd, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
        perror("connect error");
        close(client_fd);
        exit(-1);
    }

    client->fd = client_fd;

    add_watcher(looper, STDIN_FILENO, handle_events, client, POLLIN);
    add_watcher(looper, client_fd, handle_events, client, POLLIN);

    return client;
}

void destroy_client(Client *client) {
    if (!client) {
        printf("There is nothing to point the Client\n");
        return;
    }

    if (close(client->fd) < 0) {
        printf("Failed to close\n");
        return;
    }
    remove_all_watchers(client->looper);

    free(client);
}
