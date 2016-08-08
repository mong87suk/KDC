#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <sys/un.h>
#include <string.h>
#include <time.h>

#include <kdc/DBLinkedList.h>

#include "client.h"
#include "looper.h"
#include "socket.h"
#include "stream_buf.h"
#include "packet.h"
#include "converter.h"
#include "m_boolean.h"
#include "utils.h"
#include "account.h"

struct _Client
{
    int fd;
    unsigned int input_id;
    unsigned int response_id;
    Looper *looper;
    READ_RES_STATE read_state;
    DList *stream_buf_list;
    DList *stdin_stream_buf_list;
    int packet_len;
};

static void client_sum_size(void *data, void *user_data) {
    int *size;
    Stream_Buf *stream_buf;

    stream_buf = (Stream_Buf*) data;
    size = (int*) user_data;

    *size += stream_buf_get_position(stream_buf);
}

static int client_get_read_size(DList *stream_buf_list) {
    int size;

    if (!stream_buf_list) {
        LOGD("There is nothing to point the stream_buf_list\n");
        return -1;
    }

    size = 0;
    d_list_foreach(stream_buf_list, client_sum_size, &size);
    return size;
}

static void client_append_data(void *data, void *user_data) {
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
    
    dest = stream_buf_get_available(user_data_stream_buf);
    src = stream_buf_get_buf(data_stream_buf);

    if (!src || !dest) {
        LOGD("There is nothing to point buf\n");
        return;
    }

    copy_n = stream_buf_get_position(data_stream_buf);

    memcpy(dest, src, copy_n);
    stream_buf_increase_pos(user_data_stream_buf, copy_n);
}

static void client_destroy_stream_buf(void *data) {
    Stream_Buf *stream_buf;

    stream_buf = (Stream_Buf*) data;

    if (!stream_buf) {
        LOGD("There is nothing to remove the Stream_Buf");
        return;
    }
    destroy_stream_buf(stream_buf);
}

static BOOLEAN client_append_n_data_to_buf(Stream_Buf *stream_buf, char *buf, int n) {
    char *src;

    if (!stream_buf || !buf) {
        LOGD("Can't append n_data to buf\n");
        return FALSE;
    }

    src = stream_buf_get_buf(stream_buf);
    if (!src) {
        LOGD("Failed to get buf\n");
        return FALSE;
    }

    memcpy(buf, src, n);
    return TRUE;
}

static BOOLEAN client_append_data_to_buf(DList *stream_buf_list, Stream_Buf *stream_buf) {
    if (!stream_buf_list || !stream_buf) {
        LOGD("There is no a pointer to Stream_Buf\n");
        return FALSE;
    }
    d_list_foreach(stream_buf_list, client_append_data, stream_buf);
    return TRUE;
}

static void client_destroy_stream_buf_list(Client *client) {
    if (!client) {
        LOGD("There is nothing to point the Client\n");
        return;
    }
    d_list_free(client->stream_buf_list, client_destroy_stream_buf);
    client->stream_buf_list = NULL;
    client->read_state = READY_TO_READ_RES;
}

static void client_destroy_stdin_stream_buf_list(Client *client) {
    if (!client) {
        LOGD("There is nothing to point the Client\n");
        return;
    }
    d_list_free(client->stdin_stream_buf_list, client_destroy_stream_buf);
    client->stdin_stream_buf_list = NULL;
}

static BOOLEAN client_copy_payload_len(Stream_Buf *stream_buf, long int *payload_len) {
    char *buf;
    if (!stream_buf) {
        LOGD("There is nothing to point the stream buf\n");
        return FALSE;
    }

    buf = stream_buf_get_buf(stream_buf);
    if (!buf) {
        LOGD("There is nothing to point the buf\n");
        return FALSE;
    }

    buf = buf + sizeof(char) + sizeof(short);
    memcpy(payload_len, buf, sizeof(long int));
    return TRUE;
}

static BOOLEAN client_is_checksum_true(short check_sum, char *buf, int packet_len) {
    short comp_check_sum;

    if (!buf) {
        LOGD("Failed to check enable check_sum\n");
        return FALSE;
    }

    comp_check_sum = 0;

    memcpy(&comp_check_sum, buf + packet_len - 2, sizeof(comp_check_sum));

    if (comp_check_sum != check_sum) {
        LOGD("Not enabled check sum\n");
        return FALSE;
    }

    return TRUE;
}

static BOOLEAN client_copy_overread_buf(Stream_Buf *c_stream_buf, Stream_Buf *r_stream_buf, int packet_len, int len) {
    char *copy_buf, *buf;

    if (!r_stream_buf || !c_stream_buf) {
        LOGD("Can't check overwrite\n");
        return FALSE;
    }

    buf = stream_buf_get_buf(r_stream_buf);
    if (!buf) {
        LOGD("Failed to get the buf\n");
        return FALSE;
    }

    copy_buf = stream_buf_get_buf(c_stream_buf);
    if (!copy_buf) {
        LOGD("Failed to get the buf\n");
        return FALSE;
    }

    memcpy(copy_buf, buf + packet_len, len);
    return TRUE;
}

static int client_check_overread(Stream_Buf *r_stream_buf, int packet_len) {
    int pos;

    if (!r_stream_buf) {
        LOGD("There is nothing to point the Stream_Buf\n");
        return 0;
    }

    pos = stream_buf_get_position(r_stream_buf);
    if (!pos) {
        LOGD("Stream Buffer Position is zero\n");
        return 0;
    }
    return (pos - packet_len);
}

static void client_print_result(char *payload) {
    short op_code = 0;
    int result_type = 0;

    memcpy(&op_code, payload, OP_CODE_SIZE);
    payload += OP_CODE_SIZE;
    memcpy(&result_type, payload, RESULT_TYPE_SIZE);

    LOGD("op_code:%x result_type:%d \n", op_code, result_type);
}

static void client_print_id(char *payload, int len) {
    for (int i = 0; i <len; i++) {
        printf("%c", payload[i]);
    }
}

static void client_print_loggin_list(char *payload) {
    int count = 0;
    int len = 0;
    memcpy(&count, payload, sizeof(count));
    payload += sizeof(count);
    if (!count) {
        if (payload[0]) {
            LOGD("Packet was wrong\n");
            return;
        }
    }
    LOGD("LOGIN COUNT:%d\n", count);
    for (int i = 0; i < count; i++) {
        memcpy(&len, payload, LEN_SIZE);
        payload += LEN_SIZE;
        printf("%d ", i);
        client_print_id(payload, len);
        printf(" ");
        payload += len;
    }
    printf("\n");
}

static void client_handle_res_events(Client *client, int fd) {
    char *buf, *payload;
    Stream_Buf *stream_buf, *r_stream_buf, *c_stream_buf;
    int read_len, n_byte, packet_len, len, buf_size, n_mesg, mesg_len;
    BOOLEAN result;
    int i;
    long int payload_len;
    Packet *packet;
    short checksum, op_code;
    Message *mesg, *tmp;

    payload_len = 0;
    packet_len = 0;
    buf_size = 0;
    buf = NULL;
    r_stream_buf = NULL;

    LOGD("Start client_handle_res_events()\n");

    if (!client) {
        LOGD("There is nothing to point he Client\n");
        return;
    }

    if (client->read_state == READY_TO_READ_RES) {
        stream_buf = d_list_get_data(d_list_last(client->stream_buf_list));

        if (stream_buf == NULL || stream_buf_get_available_size(stream_buf) == 0) {
            stream_buf = new_stream_buf(MAX_BUF_LEN);
            client->stream_buf_list = d_list_append(client->stream_buf_list, stream_buf);
        }

        n_byte = read(fd, stream_buf_get_available(stream_buf), stream_buf_get_available_size(stream_buf));
        if (n_byte < 0) {
            LOGD("Failed to read\n");
            client_destroy_stream_buf_list(client);
            return;
        }

        result = stream_buf_increase_pos(stream_buf, n_byte);
        if (result == FALSE) {
            LOGD("Failed to set the position\n");
            client_destroy_stream_buf_list(client);
            return;
        }

        read_len = client_get_read_size(client->stream_buf_list);
        if (read_len < HEADER_SIZE) {
            LOGD("Not enough\n");
            return;
        }

        r_stream_buf = new_stream_buf(read_len);
        if (!r_stream_buf) {
            LOGD("Failed to make the Stream Buf\n");
            return;
        }

        result = client_append_data_to_buf(client->stream_buf_list, r_stream_buf);
        if (result == FALSE) {
            LOGD("Failed to append data of stream buf list to buf\n");
            destroy_stream_buf(r_stream_buf);
            return;
        }

        result = client_copy_payload_len(r_stream_buf, &payload_len);
        if (result == FALSE) {
            LOGD("Failed to  copy payload_len\n");
            return;
       }

        packet_len = HEADER_SIZE + payload_len + TAIL_SIZE;
        client->packet_len = packet_len;

        if (read_len >= packet_len) {
            client->read_state = FINISH_TO_READ_RES;
            buf_size = packet_len;
        } else {
            client->read_state = START_TO_READ_RES;
            buf_size = HEADER_SIZE;
        }

        buf = (char*) malloc(buf_size);
        if (!buf) {
            LOGD("Failed to make buf to check Header\n");
            client_destroy_stream_buf_list(client);
            destroy_stream_buf(r_stream_buf);
            return;
        }

        result = client_append_n_data_to_buf(r_stream_buf, buf, buf_size);
        if (result == FALSE) {
            LOGD("Failed to append n_data to buf\n");
            client_destroy_stream_buf_list(client);
            destroy_stream_buf(r_stream_buf);
            free(buf);
            return;
        }

        if (buf[0] != SOP) {
            LOGD("Failed to get binary\n");
            client_destroy_stream_buf_list(client);
            destroy_stream_buf(r_stream_buf);
            free(buf);
            return;
        }

        if (result == FALSE) {
            LOGD("Failed to copy payload\n");
            client_destroy_stream_buf_list(client);
            return;
        }
    }

    if (client->read_state == START_TO_READ_RES) {
        LOGD("START_TO_READ_RES\n");
        packet_len = client->packet_len;
        if (buf) {
            free(buf);
        }

        if (r_stream_buf) {
            destroy_stream_buf(r_stream_buf);
        }

        stream_buf = d_list_get_data(d_list_last(client->stream_buf_list));
        if (stream_buf == NULL || stream_buf_get_available_size(stream_buf) == 0) {
            stream_buf = new_stream_buf(MAX_BUF_LEN);
            client->stream_buf_list = d_list_append(client->stream_buf_list, stream_buf);
        }

        n_byte = read(fd, stream_buf_get_available(stream_buf), stream_buf_get_available_size(stream_buf));

        if (n_byte < 0) {
            LOGD("Failed to read\n");
            client_destroy_stream_buf_list(client);
            return;
        }

        result = stream_buf_increase_pos(stream_buf, n_byte);
        if (result == FALSE) {
            LOGD("Failed to set the position\n");
            client_destroy_stream_buf_list(client);
            return;
        }

        read_len = client_get_read_size(client->stream_buf_list);

        if (read_len < (client->packet_len)) {
            LOGD("Not enough\n");
            return;
        }
        client->read_state = FINISH_TO_READ_RES;

        r_stream_buf = new_stream_buf(read_len);
        if (!r_stream_buf) {
            LOGD("Failed to make the Stream Buf\n");
            client_destroy_stream_buf_list(client);
            return;
        }

        result = client_append_data_to_buf(client->stream_buf_list, r_stream_buf);
        if (result == FALSE) {
            LOGD("Failed to append data of stream buf list to buf\n");
            client_destroy_stream_buf_list(client);
            destroy_stream_buf(r_stream_buf);
            return;
        }

        buf = (char*) malloc(packet_len);
        if (!buf) {
            LOGD("Failed to make buf to check Header\n");
            client_destroy_stream_buf_list(client);
            destroy_stream_buf(r_stream_buf);
            return;
        }
        LOGD("packet_len:%d\n", packet_len);
        result = client_append_n_data_to_buf(r_stream_buf, buf, packet_len);
        if (result == FALSE) {
            LOGD("Failed to append n_data to buf\n");
            client_destroy_stream_buf_list(client);
            destroy_stream_buf(r_stream_buf);
            free(buf);
            return;
        }
    }


    if (client->read_state == FINISH_TO_READ_RES) {
        LOGD("FINISH_TO_READ_REQ\n");
        if (buf[packet_len -3] != EOP) {
            LOGD("Packet is wrong\n");
            client_destroy_stream_buf_list(client);
            destroy_stream_buf(r_stream_buf);
            free(buf);
            return;
        }

        checksum = packet_create_checksum(NULL, buf, packet_len);
        result = client_is_checksum_true(checksum, buf, packet_len);
        if (result == FALSE) {
            LOGD("check_sum is wrong\n");
            client_destroy_stream_buf_list(client);
            destroy_stream_buf(r_stream_buf);
            free(buf);
            return;
        }

        client_destroy_stream_buf_list(client);
        len = client_check_overread(r_stream_buf, packet_len);
        if (len > 0) {
            LOGD("Packet is overread\n");

            c_stream_buf = new_stream_buf(len);
            if (!c_stream_buf) {
                LOGD("Failed to make the Stream_Buf\n");
                return;
            }
            result = client_copy_overread_buf(c_stream_buf, r_stream_buf, packet_len, len);
            if (result == FALSE) {
                LOGD("Failed to copy overread buf\n");
            }
            client->stream_buf_list = d_list_append(client->stream_buf_list, c_stream_buf);
        }

        destroy_stream_buf(r_stream_buf);
    }

    packet = new_packet(buf);
    if (!packet) {
        LOGD("Failed to make the Packet\n");
        return;
    }
    op_code = packet_get_op_code(packet, NULL);
    if (op_code == -1) {
        LOGD("Failed to get the OPCODE\n");
        return;
    }

    free(buf);

    payload = packet_get_payload(packet, NULL);

    switch (op_code) {
        case RES_ALL_MSG: case RCV_FIRST_OR_LAST_MSG:
            LOGD("RES_ALL_MSG\n");
            mesg =  convert_payload_to_mesgs(payload, &n_mesg);
            LOGD("FINISH CONVERT\n");
            tmp = mesg;
            for (i = 0; i < n_mesg; i++) {
                mesg = message_next(tmp, i);
                utils_print_mesg(mesg);
            }
            break;
        case RCV_MSG:
            LOGD("RCV_MSG\n");
            mesg = convert_payload_to_mesg(payload, &mesg_len);
            utils_print_mesg(mesg);
            break;
        
        case RES_RESULT:
            LOGD("RES_RESULT\n");
            client_print_result(payload);
            break;
        case RES_LOG_IN_LIST:
            LOGD("RES_LOG_IN_LIST\n");
            client_print_loggin_list(payload);
            break;
        default:
            LOGD("OP_CODE was wrong\n");
    }

    free(payload);
    LOGD("Finish client_handle_res_events()\n");
}

static void client_handle_disconnect(Client *client, int fd, unsigned int id) {
    if (client->response_id == id) {
        LOGD("client_handle_disconnect\n");
        looper_remove_all_watchers(client->looper);
    }
}

static Stream_Buf *client_new_mesg_data(char *dest, int *size) {
    int i = 0;
    int buf_size = 0;
    char c;
    do {
        c = dest[i++];
    } while (c != NEW_LINE);

    if (i == 0) {
        LOGD("Can't new account info\n");
        return NULL;
    }    
    int str_len = i - 1;
    buf_size = STR_LEN_SIZE + str_len;

    Stream_Buf *stream_buf = new_stream_buf(buf_size);
    char *buf = stream_buf_get_buf(stream_buf);
    if (!buf) {
        LOGD("Faild to get the buf\n");
        destroy_stream_buf(stream_buf);
        return NULL;
    }

    memcpy(buf, &str_len, STR_LEN_SIZE);
    BOOLEAN result = stream_buf_increase_pos(stream_buf, STR_LEN_SIZE);
    if (result == FALSE) {
        LOGD("Failed to increase position\n");
        destroy_stream_buf(stream_buf);
        return NULL;
    }

    buf = stream_buf_get_available(stream_buf);
    if (!buf) {
        LOGD("Faield to get the buf\n");
        destroy_stream_buf(stream_buf);
        return NULL;
    }
    memcpy(buf, dest, str_len);
    result = stream_buf_increase_pos(stream_buf, str_len);
    if (result == FALSE) {
        LOGD("Failed to increase position\n");
        destroy_stream_buf(stream_buf);
        return NULL;
    }
    *size = i;
    return stream_buf;
}

static Stream_Buf *client_new_input_data(char *dest, int *size) {
    int i = 0;
    int buf_size = 0;
    char c;
    do {
        c = dest[i++];
    } while (c != SPACE && c != NEW_LINE);

    if (i == 0) {
        LOGD("Can't new account info\n");
        return NULL;
    }    
    int str_len = i - 1;
    buf_size = STR_LEN_SIZE + str_len;

    Stream_Buf *stream_buf = new_stream_buf(buf_size);
    char *buf = stream_buf_get_buf(stream_buf);
    if (!buf) {
        LOGD("Faild to get the buf\n");
        destroy_stream_buf(stream_buf);
        return NULL;
    }

    memcpy(buf, &str_len, STR_LEN_SIZE);
    BOOLEAN result = stream_buf_increase_pos(stream_buf, STR_LEN_SIZE);
    if (result == FALSE) {
        LOGD("Failed to increase position\n");
        destroy_stream_buf(stream_buf);
        return NULL;
    }

    buf = stream_buf_get_available(stream_buf);
    if (!buf) {
        LOGD("Faield to get the buf\n");
        destroy_stream_buf(stream_buf);
        return NULL;
    }
    memcpy(buf, dest, str_len);
    result = stream_buf_increase_pos(stream_buf, str_len);
    if (result == FALSE) {
        LOGD("Failed to increase position\n");
        destroy_stream_buf(stream_buf);
        return NULL;
    }
    *size = i;
    return stream_buf;
}

static Stream_Buf *client_new_interval_msg_payload(char *input_str, int input_strlen) {
    int body_len;
    time_t current_time;
    Stream_Buf *stream_buf;
    DList *stream_buf_list;
    int i, num;
    BOOLEAN result;
    unsigned int interval;

    i = 0;
    interval = 0;
    body_len = 0;
    input_str += REQ_STR_MIN_LEN;
    LOGD("input_str:%s\n", input_str);
    input_strlen -= (REQ_STR_MIN_LEN);
    stream_buf_list = NULL;

    while(1) {
        num = input_str[i];
        if (num == SPACE) {
            break;
        }

        if ('0' <= num && num <= '9') {
            interval = (interval) * 10 + (num -'0');
            i++;
        } else {
            LOGD("Your command was wrong\n");
            return NULL;
        }
    }

    if (i == 0) {
        LOGD("Your command was wrong\n");
        return NULL;
    }

    stream_buf = new_stream_buf(INTERVAL_SIZE);
    if (!stream_buf) {
        LOGD("Failed to new stream buf\n");
        return NULL;
    }

    memcpy(stream_buf_get_available(stream_buf), &interval, INTERVAL_SIZE);
    stream_buf_increase_pos(stream_buf, INTERVAL_SIZE);
    stream_buf_list = d_list_append(stream_buf_list, stream_buf);

    body_len += INTERVAL_SIZE;
    input_str += (i + 1);
    input_strlen -= (i + 1);

    Stream_Buf *user_id = new_stream_buf(NO_USER);
    if (!user_id) {
        LOGD("Failed to new stream buf\n");
        utils_destroy_stream_buf_list(stream_buf_list);
        return NULL;
    }
    stream_buf_increase_pos(user_id, NO_USER);
    stream_buf_list = d_list_append(stream_buf_list, user_id);
    body_len += NO_USER;

    current_time = time(NULL);
    if (current_time == ((time_t) - 1)) {
        LOGD("Failed to obtain the current time.\n");
        utils_destroy_stream_buf_list(stream_buf_list);
        return NULL;
    }

    stream_buf = new_stream_buf(TIME_SIZE);
    if (!stream_buf) {
        LOGD("Failed to new stream buf\n");
        utils_destroy_stream_buf_list(stream_buf_list);
        return NULL;
    }

    memcpy(stream_buf_get_available(stream_buf), &current_time, TIME_SIZE);
    stream_buf_increase_pos(stream_buf, TIME_SIZE);
    stream_buf_list = d_list_append(stream_buf_list, stream_buf);
    body_len += TIME_SIZE;
    
    int read_size = 0;
    LOGD("input_str:%s\n", input_str);
    Stream_Buf *mesg = client_new_mesg_data(input_str, &read_size);
    input_strlen -= read_size;
    if (input_strlen) {
        LOGD("Failed to new send data\n");
        destroy_stream_buf(mesg);
        utils_destroy_stream_buf_list(stream_buf_list);
        return NULL;    
    }
    body_len += stream_buf_get_position(mesg);
    stream_buf_list = d_list_append(stream_buf_list, mesg);

    stream_buf = new_stream_buf(body_len);
    if (!stream_buf) {
        LOGD("Failed to new stream buf\n");
        utils_destroy_stream_buf_list(stream_buf_list);
        return NULL;
    }

    result = utils_append_data_to_buf(stream_buf_list, stream_buf);
    utils_destroy_stream_buf_list(stream_buf_list);
    if (!result) {
        LOGD("Append data to buf\n");
        utils_destroy_stream_buf_list(stream_buf_list);
        return NULL;
    }

    return stream_buf;
}

static Stream_Buf *client_new_account_data(char *input_str, int input_strlen, int info_num) {
    DList *stream_buf_list = NULL;
    Stream_Buf *stream_buf =  NULL;
    int read_size = 0;
    int count = 0;

    input_str += REQ_STR_MIN_LEN;
    input_strlen -= (REQ_STR_MIN_LEN);

    do {
        stream_buf = client_new_input_data(input_str, &read_size);
        if (!stream_buf) {
            if (!stream_buf_list) {
                utils_destroy_stream_buf_list(stream_buf_list);
            }
            return NULL;
        }
        stream_buf_list = d_list_append(stream_buf_list, stream_buf);
        count++;
        input_strlen -= read_size;
        input_str += read_size;
    } while (input_strlen > 0);

    LOGD("count:%d info_num%d\n", count, info_num);    
    if (count > info_num || count < info_num) {
        LOGD("Your command was wrong\n");
        utils_destroy_stream_buf_list(stream_buf_list);
        return NULL;
    }

    read_size = client_get_read_size(stream_buf_list);
    if (read_size <= 0) {
        LOGD("Your command was wrong\n");
        utils_destroy_stream_buf_list(stream_buf_list);
        return NULL;
    }
    LOGD("read_size:%d\n", read_size);
    stream_buf = new_stream_buf(read_size);
    if (!stream_buf) {
        LOGD("Failed to make the buf\n");
        utils_destroy_stream_buf_list(stream_buf_list);
        return NULL;
    }
    
    BOOLEAN result = utils_append_data_to_buf(stream_buf_list, stream_buf);
    utils_destroy_stream_buf_list(stream_buf_list);
    if (result == FALSE) {
        LOGD("Failed to append data to buf\n");
        destroy_stream_buf(stream_buf);
        return NULL;
    }

    return stream_buf; 
}

static Stream_Buf *client_new_send_msg_data(char *input_str, int input_strlen) {
    DList *stream_buf_list = NULL;
    int read_size = 0;
    int data_len = 0;

    input_str += REQ_STR_MIN_LEN;
    input_strlen -= (REQ_STR_MIN_LEN);

    Stream_Buf *interval = new_stream_buf(INTERVAL_SIZE);
    if (!interval) {
        LOGD("Failed to new stream\n");
        return NULL;
    }
    BOOLEAN result = stream_buf_increase_pos(interval, INTERVAL_SIZE);
    if (!result) {
        LOGD("Failed to increase_pos\n");
        destroy_stream_buf(interval);
        return NULL;
    }
    stream_buf_list = d_list_append(stream_buf_list, interval);
    data_len += stream_buf_get_position(interval);
    
    Stream_Buf *user_id = client_new_input_data(input_str, &read_size);
    if (!user_id) {
        utils_destroy_stream_buf_list(stream_buf_list);
        return NULL;
    }
    stream_buf_list = d_list_append(stream_buf_list, user_id);
    input_strlen -= read_size;
    if (!input_strlen) {
        LOGD("Faild to new send_mesg_data\n");
        destroy_stream_buf(user_id);
        utils_destroy_stream_buf_list(stream_buf_list);
        return NULL;
    }
    data_len += stream_buf_get_position(user_id);
    LOGD("input_strlen:%d\n", input_strlen);
    input_str += read_size;

    time_t current_time = time(NULL);
    if (current_time == ((time_t) - 1)) {
        LOGD("Failed to obtain the current time.\n");
        utils_destroy_stream_buf_list(stream_buf_list);
        return NULL;
    }

    Stream_Buf *cur_time = new_stream_buf(TIME_SIZE);
    if (!cur_time) {
        LOGD("Failed to new stream buf\n");
        utils_destroy_stream_buf_list(stream_buf_list);
        return NULL;
    }
    memcpy(stream_buf_get_available(cur_time), &cur_time, TIME_SIZE);
    result = stream_buf_increase_pos(cur_time, TIME_SIZE);
    if (!result) {
        LOGD("Failed to increase_pos\n");
        destroy_stream_buf(cur_time);
        utils_destroy_stream_buf_list(stream_buf_list);
        return NULL;
    }
    stream_buf_list = d_list_append(stream_buf_list, cur_time);
    data_len += stream_buf_get_position(cur_time);
    
    //input_str has new_line so delete -1(length of new line)
    Stream_Buf *mesg = client_new_mesg_data(input_str, &read_size);
    input_strlen -= read_size;
    if (input_strlen) {
        LOGD("Failed to new send data\n");
        destroy_stream_buf(mesg);
        utils_destroy_stream_buf_list(stream_buf_list);
        return NULL;    
    }
    stream_buf_list = d_list_append(stream_buf_list, mesg);
    data_len += stream_buf_get_position(mesg);
    
    Stream_Buf *payload = new_stream_buf(data_len);
    result = utils_append_data_to_buf(stream_buf_list, payload); 
    utils_destroy_stream_buf_list(stream_buf_list);
    if (result == FALSE) {
        LOGD("Failed to new send data\n");
        destroy_stream_buf(payload);
        return NULL;
    }
    return payload;
}

static Packet *client_create_req_packet(char *input_str, short op_code, int input_strlen) {
    Packet *req_packet;
    Stream_Buf *payload_buf;
    char *payload, *packet_buf, *tmp;
    long int payload_len;
    short checksum;
    char sop, eop, more;
    int packet_len;

    sop = SOP;
    eop = EOP;

    payload_len = 0;
    payload = NULL;
    payload_buf = NULL;

    if (!input_str) {
        LOGD("There is nothing to point input_str\n");
        return NULL;
    }

    switch(op_code) {
        case REQ_ALL_MSG: case REQ_LOG_IN_LIST:
            if (input_strlen != REQ_STR_MIN_LEN) {
                LOGD("Request was wrong. Please recommand\n");
                return NULL;
            }
            break;

        case REQ_INTERVAL_MSG: 
            if (input_strlen > REQ_STR_MIN_LEN && (input_str[REQ_STR_MIN_LEN - 1] == ' ')) {
                payload_buf = client_new_interval_msg_payload(input_str, input_strlen);
                if (!payload_buf) {
                    LOGD("Failed to new the payload\n");
                    return NULL;
                }
                payload = stream_buf_get_buf(payload_buf);
                payload_len = stream_buf_get_position(payload_buf);
                op_code = SND_MSG;
            } else {
                LOGD("Request was wrong. Please recommand\n");
                return NULL;
            }
            break;

        case REQ_FIRST_OR_LAST_MSG:
            if (input_strlen == REQ_FIRST_OR_LAST_MESG_PACKET_SIZE && (input_str[REQ_STR_MIN_LEN - 1] == ' ')) {
                more = *(input_str + REQ_STR_MIN_LEN);
                if (more == '0' || more == '1') {
                    payload_len = sizeof(more);
                    payload = (char*) malloc(payload_len);

                    if (!payload) {
                        LOGD("Failed to make the Body\n");
                        return NULL;
                    }
                    memcpy(payload, &more, payload_len);

                } else {
                    LOGD("Request was wrong. Please recommand\n");
                    return NULL;
                }
            } else {
                LOGD("Request was wrong. Please recommand\n");
                return NULL;
            }
            break;

        case REQ_MAKE_ACCOUNT:
            if (input_strlen > REQ_STR_MIN_LEN && (input_str[REQ_STR_MIN_LEN - 1] == ' ')) {
                LOGD("REQ_MAKE_ACCOUNT\n");
                payload_buf = client_new_account_data(input_str, input_strlen, ACCOUNT_INFO_NUM);

                if (!payload_buf) {
                    LOGD("Failed to new the payload\n");
                    return NULL;
                }
                payload = stream_buf_get_buf(payload_buf);
                if (!payload) {
                    LOGD("Failed to get the buf\n");
                    destroy_stream_buf(payload_buf);
                    return NULL;
                }
                payload_len = stream_buf_get_position(payload_buf);
                if (payload_len == 0) {
                    LOGD("The buf was wrong\n");
                    destroy_stream_buf(payload_buf);
                    return NULL;
                }
            } else {
                LOGD("Request was wrong. Please recommand\n");
                return NULL;
            }
            break;

        case REQ_LOG_IN: case REQ_LOG_OUT: case REQ_DELETE_ACCOUNT:
            if (input_strlen > REQ_STR_MIN_LEN && (input_str[REQ_STR_MIN_LEN - 1] == ' ')) {
                LOGD("REQ_MAKE_ACCOUNT\n");
                payload_buf = client_new_account_data(input_str, input_strlen, LOG_IN_INFO_NUM);

                if (!payload_buf) {
                    LOGD("Failed to new the payload\n");
                    return NULL;
                }
                payload = stream_buf_get_buf(payload_buf);
                if (!payload) {
                    LOGD("Failed to get the buf\n");
                    destroy_stream_buf(payload_buf);
                    return NULL;
                }
                payload_len = stream_buf_get_position(payload_buf);
                if (payload_len == 0) {
                    LOGD("The buf was wrong\n");
                    destroy_stream_buf(payload_buf);
                    return NULL;
                }
            } else {
                LOGD("Request was wrong. Please recommand\n");
                return NULL;
            }
            break;

        case SND_MSG:
            if (input_strlen > REQ_STR_MIN_LEN && (input_str[REQ_STR_MIN_LEN - 1] == ' ')) {
                payload_buf = client_new_send_msg_data(input_str, input_strlen);
                if (!payload_buf) {
                    LOGD("Failed to new the payload\n");
                    return NULL;
                }
                payload = stream_buf_get_buf(payload_buf);
                if (!payload) {
                    LOGD("Failed to get the buf\n");
                    destroy_stream_buf(payload_buf);
                    return NULL;
                }
                payload_len = stream_buf_get_position(payload_buf);
                if (payload_len == 0) {
                    LOGD("The buf was wrong\n");
                    destroy_stream_buf(payload_buf);
                    return NULL;
                }
            } else {
                LOGD("Request was wrong. Please recommand\n");
                return NULL;
            }
            break;
        default:
            LOGD("Request number is 0x%02X Please recommand\n", op_code);
            return NULL;
    }

    packet_len = HEADER_SIZE + payload_len + TAIL_SIZE;

    packet_buf = (char*) malloc(packet_len);
    if (!packet_buf) {
        LOGD("Failed to make the buf for packet\n");
        destroy_stream_buf(payload_buf);
        return NULL;
    }

    tmp = packet_buf;

    memcpy(tmp, &sop, sizeof(sop));
    tmp += sizeof(sop);

    memcpy(tmp, &op_code, sizeof(op_code));
    tmp += sizeof(op_code);

    memcpy(tmp, &payload_len, sizeof(payload_len));
    tmp += sizeof(payload_len);

    if (payload_len > 0) {
        memcpy(tmp, payload, payload_len);
        tmp += payload_len;
    }

    destroy_stream_buf(payload_buf);

    memcpy(tmp, &eop, sizeof(eop));
    tmp += sizeof(eop);

    checksum = packet_create_checksum(NULL, packet_buf, packet_len);
    if (checksum == -1) {
        LOGD("Failed to do check_sum\n");
        free(packet_buf);
        return NULL;
    }
    memcpy(tmp, &checksum, sizeof(checksum));

    req_packet = new_packet(packet_buf);
    free(packet_buf);
 
    if (!req_packet) {
        LOGD("Failed to make the Packet\n");
        return NULL;
    }

    return req_packet;
}

static BOOLEAN send_packet_to_server(Client *client, Packet *packet) {
    int len;
    char *buf;
    short result;

    if (!client || !packet) {
        LOGD("Can't send the Packet to server\n");
        return FALSE;
    }

    len = packet_get_len(packet);
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

    if (write_n_byte(client->fd, buf, len) != len) {
        LOGD("Failed to send the Packet to server\n");
        return FALSE;
    }
    return TRUE;
}

static void client_handle_req_input_str(Client *client, char *input_str, int input_strlen) {
    Packet *req_packet;
    BOOLEAN result;
    short op_code;

    if (input_strlen >= REQ_STR_MIN_LEN && (strncasecmp(input_str, REQ_STR, strlen(REQ_STR))) == 0) {
        LOGD("input_str:%s", input_str);
        op_code = input_str[8];
        if (op_code >= '0' && op_code <= '9') {
            op_code -= OP_CODE_NUM;
        } else if (op_code >= 'A' && op_code <= 'F') {
            op_code -= OP_CODE_STR;
        } else {
            LOGD("Request was wrong. Please recommand\n");
            return;
        }
    } else {
        LOGD("Request was wrong. Please recommand\n");
        return;
    }

    if (!input_str) {
        LOGD("Can't handle req input str\n");
        return;
    }

    req_packet = client_create_req_packet(input_str, op_code, input_strlen);
    if (!req_packet) {
        LOGD("Failed to create the req packet\n");
        return;
    }

    result = send_packet_to_server(client, req_packet);
    if (result == FALSE) {
        LOGD("Failed to send the packet to server");
    }
}

static void client_handle_stdin_event(Client *client, int fd) {
    char *buf, *input_str;
    int n_byte, input_size, position, input_strlen;
    DList *stream_buf_list;
    Stream_Buf *stream_buf;
    BOOLEAN result;

    stream_buf_list = NULL;

    stream_buf = d_list_get_data(d_list_last(client->stdin_stream_buf_list));
    if (stream_buf == NULL || stream_buf_get_available_size(stream_buf) == 0) {
        stream_buf = new_stream_buf(MAX_BUF_LEN);
        client->stdin_stream_buf_list = d_list_append(client->stdin_stream_buf_list, stream_buf);
    }

    n_byte = read(fd, stream_buf_get_available(stream_buf), stream_buf_get_available_size(stream_buf));

    if (n_byte < 0) {
        LOGD("Failed to read\n");
        client_destroy_stdin_stream_buf_list(client);
        return;
    }

    result = stream_buf_increase_pos(stream_buf, n_byte);
    if (result == FALSE) {
        LOGD("Failed to set the position\n");
        client_destroy_stdin_stream_buf_list(client);
        return;
    }

    buf = stream_buf_get_buf(stream_buf);
    if (!buf) {
        LOGD("Failed to get the buf\n");
        client_destroy_stdin_stream_buf_list(client);
        return;
    }
    position = stream_buf_get_position(stream_buf);
    if (position == 0) {
        LOGD("Position is zero\n");
        client_destroy_stdin_stream_buf_list(client);
        return;
    }

    if (buf[position -1] != '\n') {
        LOGD("Not read new line\n");
        return;
    }

    stream_buf_list = client->stdin_stream_buf_list;
    input_size = client_get_read_size(stream_buf_list);
    stream_buf = new_stream_buf(input_size);
    LOGD("input_size:%d\n", input_size);

    if (!stream_buf) {
        LOGD("Failed to make the Steam Buf\n");
        return;
    }

    result = client_append_data_to_buf(stream_buf_list, stream_buf);
    client_destroy_stdin_stream_buf_list(client);
    if (result == FALSE) {
        LOGD("Failed to append input data to buf\n");
        destroy_stream_buf(stream_buf);
        return;
    }

    input_str = stream_buf_get_buf(stream_buf);
    input_strlen = input_size;

    client_handle_req_input_str(client, input_str, input_strlen);
    destroy_stream_buf(stream_buf);
    return;
}

static void client_handle_events(int fd, void *user_data, unsigned int id, int looper_event) {
    Client *client = (Client*) user_data;

    LOGD("handle_event\n");
    if (looper_event & LOOPER_HUP_EVENT) {
        client_handle_disconnect(client, fd, id);
    } else if (looper_event & LOOPER_IN_EVENT) {
        if (fd == client->fd && client->response_id == id) {
            client_handle_res_events(client, fd);
        } else if (fd == STDIN_FILENO && client->input_id == id) {
            client_handle_stdin_event(client, fd);
        } else {
            LOGD("There is no fd to handle event\n");
        }
    } else {
        LOGD("There is no event to handle\n");
    }
}

Client *new_client(Looper *looper) {
    Client *client;

    struct sockaddr_un addr;
    int client_fd;
    unsigned int id;

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

    id = looper_add_watcher(looper, STDIN_FILENO, client_handle_events, client, LOOPER_IN_EVENT | LOOPER_HUP_EVENT);
    if (id < 0) {
        return NULL;
    }
    client->input_id = id;

    id = looper_add_watcher(looper, client_fd, client_handle_events, client, LOOPER_IN_EVENT | LOOPER_HUP_EVENT);
    if (id < 0) {
        return NULL;
    }
    client->response_id = id;

    client->fd = client_fd;
    client->read_state = READY_TO_READ_RES;
    client->stream_buf_list = NULL;
    client->stdin_stream_buf_list = NULL;
    client->packet_len = 0;

    return client;
}

void destroy_client(Client *client) {
    LOGD("destory client\n");
    if (!client) {
        LOGD("There is nothing to point the Client\n");
        return;
    }

    if (close(client->fd) < 0) {
        LOGD("Failed to close\n");
        return;
    }
    looper_remove_all_watchers(client->looper);

    free(client);
}
