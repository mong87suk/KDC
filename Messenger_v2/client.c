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
#include "stream_buf.h"
#include "packet.h"

struct _Client
{
    int fd;
    Looper *looper;
};

static void sum_size(void *data, void *user_data) {
    int *size;
    Stream_Buf *stream_buf;

    stream_buf = (Stream_Buf*) data;
    size = (int*) user_data;

    *size += get_position(stream_buf);
}

static int get_buffer_size(DList *stream_buf_list) {
    int size;

    size = 0;
    d_list_foreach(stream_buf_list, sum_size, &size);
    return size;
}

static void append_data(void *data, void *user_data) {
    char *dest, *src;
    int beginIndex, n;
    Stream_Buf *stream_buf;

    stream_buf = (Stream_Buf*) data;
    dest = (char*) user_data;

    if (!stream_buf) {
        printf("There is nothing to point Stream_Buf\n");
        return;
    }

    src = get_buf(stream_buf);

    if (!src) {
        printf("There is nothing to point buf\n");
        return;
    }

    beginIndex = strlen(user_data);
    n = get_position(stream_buf);

    memcpy(dest + beginIndex, src, n);
}

static void destroy_stream_buf_list(void *data) {
    Stream_Buf *stream_buf;

    stream_buf = (Stream_Buf*) data;

    if (!stream_buf) {
        printf("There is nothing to remove the Stream_Buf\n");
        return;
    }
    destroy_stream_buf(stream_buf);
}

static void append_data_to_buffer(DList *stream_buf_list, char *buf) {
    if (!stream_buf_list) {
        printf("There is no a pointer to Stream_Buf\n");
        return;
    }
    d_list_foreach(stream_buf_list, append_data, buf);
}

static void read_packet(int fd) {

}

static void handle_res_events(Client *client, int fd) {
    read_packet(fd);
}

static void handle_disconnect(Client *client, int fd) {

}

static Packet* create_req_all_mesg_packet() {
    Packet *packet;
    Header *header;
    Body *body;
    Tail *tail;
    short check_sum;

    header = new_header(SOP, REQ_ALL_MSG, 0);
    body = new_body(NULL);
    tail = new_tail(EOP, 0);

    packet = new_packet(header, body, tail);
    check_sum = do_check_sum(packet);
    set_check_sum(packet, check_sum);

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

static char* create_req_packet(char *input_str) {
    int input_strlen;
    char request_num;
    Packet *packet;

    if (!input_str) {
        printf("There is nothing to point input_str\n");
        return NULL;
    }

    input_strlen = strlen(input_str);

    if (input_strlen >= REQ_STR_MIN_LEN && (strncasecmp(input_str, REQ_STR, strlen(REQ_STR))) == 0) {
        printf("input_str:%s", input_str);
        request_num = input_str[8] - '0';

        switch(request_num) {
        case REQ_ALL_MSG:
            if (input_strlen == REQ_STR_MIN_LEN) {
                packet = create_req_all_mesg_packet();
            }
            break;
        case SND_MSG:
            if (input_strlen > REQ_STR_MIN_LEN && (input_str[REQ_STR_MIN_LEN - 1] == ' ')) {
                packet = create_req_snd_mesg_packet(input_str);
            }
            break;
        case REQ_FIRST_OR_LAST_MSG:
            if (input_strlen == REQ_FIRST_OR_LAST_MESG_PACKET_SIZE && (input_str[REQ_STR_MIN_LEN - 1] == ' ')) {
                packet = create_req_first_or_last_mesg_packet(input_str);
            }
            break;
        default:
            printf("Your request number is %c Please recommand\n", request_num);
            return NULL;
        }
    }
}


static char* handle_stdin_event(Client* client, int fd) {
    char *buf, *input_str;
    int n_byte;
    int input_size;
    int position;
    DList *stream_buf_list;
    Stream_Buf *stream_buf;

    stream_buf_list = NULL;
    stream_buf = new_stream_buf(2);
    buf = get_buf(stream_buf);

    while ((n_byte = read(fd, buf, get_available_size(stream_buf)))) {
        if (n_byte < 0) {
            printf("Failed to read\n");
            return NULL;
        }

        set_position(stream_buf, n_byte);
        set_available_size(stream_buf, n_byte);

        stream_buf_list = d_list_append(stream_buf_list, stream_buf);
        position = get_position(stream_buf);

        if (buf[position - 1] == '\n') {
            break;
        } else if (position >= get_len(stream_buf)) {
            stream_buf = new_stream_buf(2);
            buf = get_buf(stream_buf);
        }
    }

    input_size = get_buffer_size(stream_buf_list);
    input_str = (char*) malloc(input_size + 1);
    printf("input_size:%d\n", input_size);

    if (!input_str) {
        printf("Failed to make input str\n");
        return NULL;
    }

    memset(input_str, 0, input_size + 1);
    append_data_to_buffer(stream_buf_list, input_str);
    printf("%s\n", input_str);
    d_list_free(stream_buf_list, destroy_stream_buf_list);
    stream_buf_list = NULL;

    return input_str;
}

static void handle_events(int fd, void *user_data, int revents) {
    Client *client = (Client*) user_data;
    char *input_str;

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
            input_str = handle_stdin_event(client, fd);
            if (!input_str) {
                printf("There is nothing to pointer input_str\n");
                return;
            }
            create_req_packet(input_str);
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
