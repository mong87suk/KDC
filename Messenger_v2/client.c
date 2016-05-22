#include <stdio.h>
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
        printf("%s %s There is nothing to point Stream_Buf\n", __FILE__, __func__);
        return;
    }

    src = get_buf(stream_buf);

    if (!src) {
        printf("%s %s There is nothing to point buf\n", __FILE__, __func__);
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
        printf("%s %s There is nothing to remove the Stream_Buf\n", __FILE__, __func__);
        return;
    }
    destroy_stream_buf(stream_buf);
}

static void append_data_to_buffer(DList *stream_buf_list, char *buf) {
    if (!stream_buf_list) {
        printf("%s %s There is no a pointer to Stream_Buf\n", __FILE__, __func__);
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

static char* create_req_packet(char *input_str) {
    int input_strlen;
    short request_num;
    int len;
    char *payload;

    Packet *packet;
    Header *header;
    Body *body;
    Tail *tail;
    short check_sum;

    header = new_header(SOP, 0, 0);
    tail = new_tail(EOP, 0);
    packet = new_packet(header, NULL, tail);

    if (!input_str) {
        printf("%s %s There is nothing to point input_str\n", __FILE__, __func__);
        return NULL;
    }

    input_strlen = strlen(input_str);

    if (input_strlen >= REQ_STR_MIN_LEN && (strncasecmp(input_str, REQ_STR, strlen(REQ_STR))) == 0) {
        printf("%s %s input_str:%s", __FILE__, __func__, input_str);
        request_num = input_str[8] - '0';

        switch(request_num) {
        case REQ_ALL_MSG:
            if (input_strlen != REQ_STR_MIN_LEN) {
                printf("%s %s Request was wrong. Please recommand\n", __FILE__, __func__);
                return NULL;
            }
            break;
        case SND_MSG:
            if (input_strlen > REQ_STR_MIN_LEN && (input_str[REQ_STR_MIN_LEN - 1] == ' ')) {
                len = strlen(input_str + REQ_STR_MIN_LEN);
                printf("%d %s", len, input_str + REQ_STR_MIN_LEN);
                payload = (char*) malloc(len);
                memset(payload, 0, len);
                memcpy(payload, input_str + REQ_STR_MIN_LEN, len);
                body = new_body(payload);
                set_payload_len(packet, len);
                set_body(packet, body);
            }
            break;
        case REQ_FIRST_OR_LAST_MSG:
            if (input_strlen == REQ_FIRST_OR_LAST_MESG_PACKET_SIZE && (input_str[REQ_STR_MIN_LEN - 1] == ' ')) {
                len = strlen(input_str + REQ_STR_MIN_LEN);
                printf("%d %s", len, input_str + REQ_STR_MIN_LEN);
                payload = (char*) malloc(len);
                memset(payload, 0, len);
                memcpy(payload, input_str + REQ_STR_MIN_LEN, len);
                body = new_body(payload);
                set_payload_len(packet, len);
                set_body(packet, body);
            }
            break;
        default:
            printf("%s %s Request number is %c Please recommand\n", __FILE__, __func__, request_num);
            return NULL;
        }

        printf("check\n");
        set_op_code(packet, request_num);
        printf("request_num\n");
        check_sum = do_check_sum(packet);
        printf("check_sum\n");
        set_check_sum(packet, check_sum);
        printf("set_checK_sum\n");
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

    while ((n_byte = read(fd, get_available_buf(stream_buf), get_available_size(stream_buf)))) {
        if (n_byte < 0) {
            printf("%s %s Failed to read\n", __FILE__, __func__);
            return NULL;
        }

        set_position(stream_buf, n_byte);
        set_available_size(stream_buf, n_byte);

        stream_buf_list = d_list_append(stream_buf_list, stream_buf);
        position = get_position(stream_buf);
        buf = get_buf(stream_buf);

        if (buf[position - 1] == '\n') {
            break;
        } else if (position >= get_len(stream_buf)) {
            stream_buf = new_stream_buf(2);
        }
    }

    input_size = get_buffer_size(stream_buf_list);
    input_str = (char*) malloc(input_size + 1);
    printf("%s %s input_size:%d\n", __FILE__, __func__, input_size);

    if (!input_str) {
        printf("%s %s Failed to make input str\n", __FILE__, __func__);
        return NULL;
    }

    memset(input_str, 0, input_size + 1);
    append_data_to_buffer(stream_buf_list, input_str);
    d_list_free(stream_buf_list, destroy_stream_buf_list);
    stream_buf_list = NULL;

    create_req_packet(input_str);

    return input_str;
}

static void handle_events(int fd, void *user_data, int looper_event) {
    Client *client = (Client*) user_data;

    if (!client) {
        printf("%s %s There is no a pointer to Client\n", __FILE__, __func__);
        return;
    }

    if (looper_event == LOOPER_HUP_EVENT) {
        handle_disconnect(client, fd);
    } else if (looper_event == LOOPER_IN_EVENT) {
        if (fd == client->fd) {
            handle_res_events(client, fd);
        } else if (fd == STDIN_FILENO) {
            handle_stdin_event(client, fd);
        } else {
            printf("%s %s There is no fd to handle event\n", __FILE__, __func__);
            return;
        }
    } else {
        printf("%s %s There is no event to handle\n", __FILE__, __func__);
    }
}

Client* new_client(Looper *looper) {
    Client *client;

    struct sockaddr_un addr;
    int client_fd;

    if (!looper) {
        printf("%s %s There is no a pointer to Looper\n", __FILE__, __func__);
        return NULL;
    }

    client = (Client*) malloc(sizeof(Client));

    if (!client) {
        printf("%s %sFailed to make client\n", __FILE__, __func__);
        return NULL;
    }

    client->looper = looper;

    if ((client_fd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
        printf("%s %s socket error\n", __FILE__, __func__);
        return NULL;
    }

    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, SOCKET_PATH, sizeof(addr.sun_path) - 1);

    if (connect(client_fd, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
        printf("%s %s connect error", __FILE__, __func__);
        close(client_fd);
        return NULL;
    }

    client->fd = client_fd;

    add_watcher(looper, STDIN_FILENO, handle_events, client, LOOPER_IN_EVENT);
    add_watcher(looper, client_fd, handle_events, client, LOOPER_IN_EVENT);

    return client;
}

void destroy_client(Client *client) {
    if (!client) {
        printf("%s %s There is nothing to point the Client\n", __FILE__, __func__);
        return;
    }

    if (close(client->fd) < 0) {
        printf("%s %s Failed to close\n", __FILE__, __func__);
        return;
    }
    remove_all_watchers(client->looper);

    free(client);
}
