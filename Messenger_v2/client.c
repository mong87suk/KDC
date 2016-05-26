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
#include "converter.h"
#include "m_boolean.h"
#include "utils.h"

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
        LOGD("There is nothing to point Stream_Buf\n");
        return;
    }

    src = get_buf(stream_buf);

    if (!src) {
        LOGD("There is nothing to point buf\n");
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
        LOGD("There is nothing to remove the Stream_Buf");
        return;
    }
    destroy_stream_buf(stream_buf);
}

static short  append_data_to_buf(DList *stream_buf_list, char *buf) {
    if (!stream_buf_list) {
        LOGD("There is no a pointer to Stream_Buf\n");
        return FALSE;
    }
    d_list_foreach(stream_buf_list, append_data, buf);
    return TRUE;
}

static void read_packet(int fd) {

}

static void handle_res_events(Client *client, int fd) {
    read_packet(fd);
}

static void handle_disconnect(Client *client, int fd) {

}

static Body* create_body(char *input_str, long int *payload_len) {
    int len;
    char *payload;
    Body *body;

    if (!input_str) {
        LOGD("There is nothing to point the input_str\n");
        return NULL;
    }

    len = strlen(input_str + REQ_STR_MIN_LEN) - 1;
    *payload_len = len;
    payload = (char*) malloc(len);

    if (!payload) {
        LOGD("Failed to make payload buf\n");
        return NULL;
    }

    memset(payload, 0, len);
    memcpy(payload, input_str + REQ_STR_MIN_LEN, len);

    body = new_body(payload);
    if (!body) {
        LOGD("Failed to make the Packet\n");
        return NULL;
    }
    return body;
}

static Packet* create_req_packet(char *input_str, short op_code) {
    int input_strlen;
    Packet *packet;
    Header *header;
    Body *body;
    Tail *tail;
    int long payload_len;
    short check_sum, result;

    header = new_header(SOP, 0, 0);
    tail = new_tail(EOP, 0);
    packet = new_packet(header, NULL, tail);
    payload_len = 0;

    if (!header || !tail || !packet) {
        LOGD("Can't make the Packet\n");
        return NULL;
    }

    if (!input_str) {
        LOGD("There is nothing to point input_str\n");
        return NULL;
    }

    input_strlen = strlen(input_str);

    switch(op_code) {
        case REQ_ALL_MSG:
            if (input_strlen != REQ_STR_MIN_LEN) {
                LOGD("Request was wrong. Please recommand\n");
                return NULL;
            }
            break;
        case SND_MSG:
            if (input_strlen > REQ_STR_MIN_LEN && (input_str[REQ_STR_MIN_LEN - 1] == ' ')) {
                body = create_body(input_str, &payload_len);
                if (!body) {
                    LOGD("Failed to make the Body\n");
                    return NULL;
                }
                result = set_body(packet, body);
                if (result == FALSE) {
                    LOGD("Failed to set the Body\n");
                    return NULL;
                }

                result = set_payload_len(packet, payload_len);
                if (result == FALSE) {
                    LOGD("Failed to set the payload_len\n");
                    return NULL;
                }

            } else {
                LOGD("Request was wrong. Please recommand\n");
                return NULL;
            }
            break;
        case REQ_FIRST_OR_LAST_MSG:
            if (input_strlen == REQ_FIRST_OR_LAST_MESG_PACKET_SIZE && (input_str[REQ_STR_MIN_LEN - 1] == ' ')) {
                if (*(input_str + REQ_STR_MIN_LEN) == '0' || *(input_str + REQ_STR_MIN_LEN) == '1') {
                    body = create_body(input_str, &payload_len);
                    if (!body) {
                        LOGD("Failed to make the Body\n");
                        return NULL;
                    }

                    result = set_body(packet,body);
                    if (result == FALSE) {
                        LOGD("Failed to set the Body\n");
                        return NULL;
                    }

                    result = set_payload_len(packet, payload_len);
                    if (result == FALSE) {
                        LOGD("Failed to set the payload_len\n");
                        return NULL;
                    }

                } else {
                    LOGD("Request was wrong. Please recommand\n");
                    return NULL;
                }
            } else {
                LOGD("Request was wrong. Please recommand\n");
                return NULL;
            }
            break;
        default:
            LOGD("Request number is %c Please recommand\n", op_code);
            return NULL;
    }

    result = set_op_code(packet, op_code);
    if (result == FALSE) {
        LOGD("Failed to set the op_code\n");
        return NULL;
    }

    check_sum = create_check_sum(packet);
    if (check_sum == -1) {
        LOGD("Failed to do check_sum\n");
        return NULL;
    }

    result = set_check_sum(packet, check_sum);
    if (result == FALSE) {
        LOGD("Failed to set the check_sum\n");
        return NULL;
    }

    return packet;
}

static short  send_packet_to_server(Client *client, Packet *packet) {
    int len;
    char *buf;
    short result;

    if (!client || !packet) {
        LOGD("Can't send the Packet to server\n");
        return FALSE;
    }

    len = get_packet_len(packet);
    if (len == -1) {
        LOGD("Failed to get the packet len\n");
        return FALSE;
    }

    buf = (char*) malloc(len);
    if(!buf) {
        LOGD("Failed to make buf to copy the Packet\n");
        return FALSE;
    }

    result = convert_packet_to_buf(packet, buf);
    if (result == FALSE) {
        LOGD("Failed to convert the Packet to buf\n");
        return FALSE;
    }

    if (write(client->fd, buf, len) < 0) {
        LOGD("Failed to send the Packet to server\n");
        return FALSE;
    }
    return FALSE;
}

static void handle_stdin_event(Client* client, int fd) {
    char *buf, *input_str;
    int n_byte, input_size, position, input_strlen;
    DList *stream_buf_list;
    Stream_Buf *stream_buf;

    Packet *packet;
    short result, op_code;

    stream_buf_list = NULL;

    do {
        if (position >= MAX_BUF_LEN || stream_buf_list == NULL) {
            stream_buf = new_stream_buf(MAX_BUF_LEN);
            stream_buf_list = d_list_append(stream_buf_list, stream_buf);
        }

        n_byte = read(fd, get_available_buf(stream_buf), get_available_size(stream_buf));
        if (n_byte < 0) {
            LOGD("Failed to read\n");
            return;
        }

        result = set_position(stream_buf, n_byte);
        if (result == FALSE) {
            LOGD("Failed to set the position\n");
            d_list_free(stream_buf_list, destroy_stream_buf_list);
            return;
        }

        position = get_position(stream_buf);
        buf = get_buf(stream_buf);
    } while (buf[position - 1] != '\n');

    input_size = get_buffer_size(stream_buf_list);
    input_str = (char*) malloc(input_size);
    LOGD("input_size:%d\n", input_size);

    if (!input_str) {
        LOGD("Failed to make input str\n");
        return;
    }

    memset(input_str, 0, input_size);
    result = append_data_to_buf(stream_buf_list, input_str);

    if (result == FALSE) {
        LOGD("Failed to append input data to buf\n");
        return;
    }

    d_list_free(stream_buf_list, destroy_stream_buf_list);
    stream_buf_list = NULL;

    input_strlen = strlen(input_str);
    if (input_strlen >= REQ_STR_MIN_LEN && (strncasecmp(input_str, REQ_STR, strlen(REQ_STR))) == 0) {
        LOGD("input_str:%s", input_str);
        op_code = input_str[8] - '0';
    } else {
        LOGD("Request was wrong. Please recommand\n");
        return;
    }

    packet = create_req_packet(input_str, op_code);
    if (!packet) {
        LOGD("Failed to make the packet\n");
        return;
    }

    result = send_packet_to_server(client, packet);
    if (result == FALSE) {
        LOGD("Failed to send the packet to server");
    }

    return;
}

static void handle_events(int fd, void *user_data, int looper_event) {
    Client *client = (Client*) user_data;

    if (!client) {
        LOGD("There is no a pointer to Client\n");
        return;
    }

    if (looper_event & LOOPER_HUP_EVENT) {
        handle_disconnect(client, fd);
    } else if (looper_event & LOOPER_IN_EVENT) {
        if (fd == client->fd) {
            handle_res_events(client, fd);
        } else if (fd == STDIN_FILENO) {
            handle_stdin_event(client, fd);
        } else {
            LOGD("There is no fd to handle event\n");
            return;
        }
    } else {
        LOGD("There is no event to handle\n");
    }
}

Client* new_client(Looper *looper) {
    Client *client;

    struct sockaddr_un addr;
    int client_fd;

    if (!looper) {
        LOGD("There is no a pointer to Looper\n");
        return NULL;
    }

    client = (Client*) malloc(sizeof(Client));

    if (!client) {
        LOGD("Failed to make client\n");
        return NULL;
    }

    client->looper = looper;

    if ((client_fd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
        LOGD("socket error\n");
        return NULL;
    }

    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, SOCKET_PATH, sizeof(addr.sun_path) - 1);

    if (connect(client_fd, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
        LOGD("connect error\n");
        close(client_fd);
        return NULL;
    }

    client->fd = client_fd;

    add_watcher(looper, STDIN_FILENO, handle_events, client, LOOPER_IN_EVENT | LOOPER_HUP_EVENT);
    add_watcher(looper, client_fd, handle_events, client, LOOPER_IN_EVENT | LOOPER_HUP_EVENT);

    return client;
}

void destroy_client(Client *client) {
    if (!client) {
        LOGD("There is nothing to point the Client\n");
        return;
    }

    if (close(client->fd) < 0) {
        LOGD("Failed to close\n");
        return;
    }
    remove_all_watchers(client->looper);

    free(client);
}
