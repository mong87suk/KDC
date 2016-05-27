#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <sys/un.h>
#include <string.h>
#include <time.h>

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
    int copy_n;
    Stream_Buf *data_stream_buf;
    Stream_Buf *user_data_stream_buf;

    data_stream_buf = (Stream_Buf*) data;
    user_data_stream_buf = (Stream_Buf*) user_data;

    if (!data_stream_buf || !user_data_stream_buf) {
        LOGD("There is nothing to point Stream_Buf\n");
        return;
    }
    
    dest = get_available_buf(user_data_stream_buf);
    src = get_buf(data_stream_buf);

    if (!src || !dest) {
        LOGD("There is nothing to point buf\n");
        return;
    }

    copy_n = get_position(data_stream_buf);

    memcpy(dest, src, copy_n);
    increase_position(user_data_stream_buf, copy_n);
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

static short  append_data_to_buf(DList *stream_buf_list, Stream_Buf *stream_buf) {
    if (!stream_buf_list || !stream_buf) {
        LOGD("There is no a pointer to Stream_Buf\n");
        return FALSE;
    }
    d_list_foreach(stream_buf_list, append_data, stream_buf);
    return TRUE;
}

static void read_packet(int fd) {

}

static void handle_res_events(Client *client, int fd) {
    read_packet(fd);
}

static void handle_disconnect(Client *client, int fd) {

}

static Body* create_body(char *input_str, int input_strlen, long int *payload_len) {
    int str_len;
    int body_len;
    char *payload, *tmp_dest;
    time_t current_time;
    Body *body;

    current_time = time(NULL);

    if (current_time == ((time_t) - 1)) {
        printf("Failed to obtain the current time.\n");
        return 0;
    }   

    if (!input_str) {
        LOGD("There is nothing to point the input_str\n");
        return NULL;
    }

    str_len = input_strlen - REQ_STR_MIN_LEN - 1;
    body_len = sizeof(long int) + sizeof(int) + str_len;
    *payload_len = body_len;
    payload = (char*) malloc(body_len);

    if (!payload) {
        LOGD("Failed to make payload buf\n");
        return NULL;
    }
    tmp_dest = payload;
    memset(payload, 0, body_len);

    memcpy(tmp_dest, &current_time, sizeof(current_time));
    tmp_dest += sizeof(current_time);

    memcpy(tmp_dest, &str_len, sizeof(str_len));
    tmp_dest += sizeof(str_len);

    memcpy(tmp_dest, input_str + REQ_STR_MIN_LEN, str_len);

    body = new_body(payload);
    if (!body) {
        LOGD("Failed to make the Packet\n");
        return NULL;
    }
    return body;
}

static Packet* create_req_packet(char *input_str, short op_code, int input_strlen) {
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

    switch(op_code) {
        case REQ_ALL_MSG:
            if (input_strlen != REQ_STR_MIN_LEN) {
                LOGD("Request was wrong. Please recommand\n");
                return NULL;
            }
            break;
        case SND_MSG:
            if (input_strlen > REQ_STR_MIN_LEN && (input_str[REQ_STR_MIN_LEN - 1] == ' ')) {
                body = create_body(input_str, input_strlen, &payload_len);
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
                    body = create_body(input_str, input_strlen, &payload_len);
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

    check_sum = create_check_sum(packet, NULL);
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

static short send_packet_to_server(Client *client, Packet *packet) {
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
    if (!buf) {
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
    return TRUE;
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

        result = increase_position(stream_buf, n_byte);
        if (result == FALSE) {
            LOGD("Failed to set the position\n");
            d_list_free(stream_buf_list, destroy_stream_buf_list);
            return;
        }

        position = get_position(stream_buf);
        buf = get_buf(stream_buf);
    } while (buf[position - 1] != '\n');

    input_size = get_buffer_size(stream_buf_list);
    stream_buf = new_stream_buf(input_size);
    LOGD("input_size:%d\n", input_size);

    if (!stream_buf) {
        LOGD("Failed to make the Steam Buf\n");
        return;
    }

    result = append_data_to_buf(stream_buf_list, stream_buf);

    if (result == FALSE) {
        LOGD("Failed to append input data to buf\n");
        return;
    }

    input_str = get_buf(stream_buf);

    d_list_free(stream_buf_list, destroy_stream_buf_list);
    stream_buf_list = NULL;

    input_strlen = input_size;
    if (input_strlen >= REQ_STR_MIN_LEN && (strncasecmp(input_str, REQ_STR, strlen(REQ_STR))) == 0) {
        LOGD("input_str:%s", input_str);
        op_code = input_str[8] - '0';
    } else {
        LOGD("Request was wrong. Please recommand\n");
        return;
    }

    packet = create_req_packet(input_str, op_code, input_strlen);
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
