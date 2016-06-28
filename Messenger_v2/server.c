#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <sys/un.h>
#include <string.h>
#include <sys/types.h>

#include "server.h"
#include "looper.h"
#include "DBLinkedList.h"
#include "socket.h"
#include "packet.h"
#include "stream_buf.h"
#include "m_boolean.h"
#include "utils.h"
#include "converter.h"
#include "message.h"
#include "message_db.h"

struct _Server {
    int fd;
    DList *client_list;
    DList *last_read_mesg_node;
    MessageDB *mesg_db;
    Looper *looper;
};

struct _Client {
    int fd;
    int last_read_pos;
    READ_REQ_STATE read_state;
    DList *stream_buf_list;
    int packet_len;
};

static void server_destroy_client(void *client) {
    Client *remove_client;

    if (!client) {
        LOGD("There is nothing to point the Client\n");
        return;
    }
    remove_client = (Client*) client;

    if (close(remove_client->fd) < 0) {
        LOGD("Falied to close client fd\n");
    }
    free(remove_client);
}

static int server_match_client(void *data1, void *data2) {
    Client *client = (Client*) data1;
    int fd = *((int*) data2);

    if (!client) {
        LOGD("There is nothing to point the Client\n");
    }

    if (client->fd == fd) {
        return 1;
    } else {
        return 0;
    }
}

static Client* server_find_client(Server *server, int fd) {
    Client *client = NULL;
    client = (Client*) d_list_find_data(server->client_list, server_match_client, &fd);

    if (client == NULL) {
        LOGD("Can't find Client\n");
        return NULL;
    }
    return client;
}

static int server_add_client(Server *server, int fd) {
    Client *client;
    DList *list;

    list = server->client_list;
    client = (Client*) malloc(sizeof(Client));

    if (!client) {
        LOGD("Failed to make Client\n");
        return -1;
    }

    client->fd = fd;
    client->last_read_pos = 1;
    client->read_state = READY_TO_READ_REQ;
    client->stream_buf_list = NULL;
    client->packet_len = 0;

    server->client_list = d_list_append(list, client);
    return 1;
}

static void server_remove_client(Server *server, int fd) {
    if (!server && !server->client_list) {
        printf("%s %s Can't remove Client\n", __FILE__, __func__);
        return;
    }

    server->client_list = d_list_remove_with_user_data(server->client_list, &fd, server_match_client, server_destroy_client);
}

static void server_sum_size(void *data, void *user_data) {
    int *size;
    Stream_Buf *stream_buf;

    if (!data || !user_data) {
        LOGD("Can't sum the stream buf list size\n");
        return;
    }

    stream_buf = (Stream_Buf*) data;
    size = (int*) user_data;

    *size += stream_buf_get_position(stream_buf);
}

static int server_get_read_size(DList *stream_buf_list) {
    int size;

    if (!stream_buf_list) {
        LOGD("There is nothing to point the stream_buf_list\n");
        return -1;
    }

    size = 0;
    d_list_foreach(stream_buf_list, server_sum_size, &size);
    return size;
}

static void server_destroy_stream_buf_list(void *data) {
    Stream_Buf *stream_buf;

    stream_buf = (Stream_Buf*) data;

    if (!stream_buf) {
        LOGD("There is nothing to remove the Stream_Buf\n");
        return;
    }
    destroy_stream_buf(stream_buf);
}

static void server_destroy_client_stream_buf_list(Client* client) {
    if (!client) {
        LOGD("There is nothing to point the Client\n");
        return;
    }
    d_list_free(client->stream_buf_list, server_destroy_stream_buf_list);
    client->stream_buf_list = NULL;
    client->read_state = READY_TO_READ_REQ;
}

static void server_append_data(void *data, void *user_data) {
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

static int server_append_data_to_buf(DList *stream_buf_list, Stream_Buf *stream_buf) {
    if (!stream_buf_list || !stream_buf) {
        LOGD("Can't append data to Stream_Buf\n");
        return FALSE;
    }
    d_list_foreach(stream_buf_list, server_append_data, stream_buf);
    return TRUE;
}

static int server_append_n_data_to_buf(Stream_Buf *stream_buf, char *buf, int n) {
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

static void server_sum_mesgs_size(void *data, void *user_data) {
    int *size;
    Message *mesg;

    if (!data || !user_data) {
        LOGD("Can't sum the mesgs size\n");
        return;
    }

    mesg = (Message *) data;
    size = (int*) user_data;

    *size += message_get_str_len(mesg);
}

static int server_get_all_mesgs_size(DList *mesg_list) {
    int size;

    if (!mesg_list) {
        LOGD("There is nothing to point the mesg\n");
        return 0;
    }

    d_list_foreach(mesg_list, server_sum_mesgs_size, &size);
    return size;
}

static int server_copy_mesgs(DList *mesg_list, Message *mesgs, int len) {
    int i, str_len, result;
    long int time; 
    Message *mesg1, *mesg2;
    char *buf, *str;

    if (!mesg_list || !mesgs) {
        LOGD("Can't copy the mesgs\n");
        return FALSE;
    }

    for (i = 0; i < len; i++) {
        mesg1 = (Message*) d_list_get_data(mesg_list);
        mesg2 = message_next(mesgs, i);
        if (!mesg1 || !mesg2) {
            LOGD("Can't copy mesgs\n");
            return FALSE;
        }

        time = message_get_time(mesg1);
        if (time == -1) {
            LOGD("Failed to get the time\n");
            return FALSE;
        }
        result = message_set_time(mesg2, time);
        if (result == FALSE) {
            LOGD("Failed to set time\n");
            return FALSE;
        }

        str_len = message_get_str_len(mesg1);
        if (str_len == -1) {
            LOGD("Failed to get the str_len\n");
            return FALSE;
        }
        result = message_set_str_len(mesg2, str_len);
        if (result == FALSE) {
            LOGD("Failed to set str len\n");
            return FALSE;
        }

        str = message_get_str(mesg1);
        if (!str) {
            LOGD("Failed to get the str\n");
            return FALSE;
        }
        buf = (char*) malloc(str_len);
        if (!buf) {
            LOGD("Failed to make the buf\n");
            return FALSE;
        }
        memcpy(buf, str, str_len);
        result = message_set_str(mesg2, buf);
        if (result == FALSE) {
            LOGD("Failed to set the str\n");
            return FALSE;
        }

        mesg_list = d_list_next(mesg_list);
        if (!mesg_list) {
            continue;
        }
    }

    return TRUE;
}

static Packet* server_create_res_packet(Server *server, Packet *req_packet, Client *client, int *read_pos) {
    short op_code, check_sum;
    char *payload, *packet_buf, *tmp, *req_payload;
    Packet *res_packet;
    int result;
    long int payload_len;
    int mesgs_size, count;
    int packet_len, mesg_len;
    Message *mesg, *mesgs;
    char sop, eop;
    DList *mesg_list;
    int pos;
    int read_count;
    char more;

    LOGD("Start server_create_res_packet()\n");

    if (!req_packet || !server) {
        LOGD("There is nothing to point req_packet or server\n");
        return NULL;
    }

    op_code = packet_get_op_code(req_packet, NULL);
    sop = SOP;
    eop = EOP;

    switch(op_code) {
    case REQ_ALL_MSG:
        count = message_db_get_message_count(server->mesg_db);
        LOGD("count:%d\n", count);
        if (count < 0) {
            LOGD("Faield to get message\n");
            return NULL;
        }

        if (count == 0) {
            LOGD("Thre is no message\n");
            return NULL;
        }

        mesg_list = message_db_get_messages(server->mesg_db, 1, count);
        if (!mesg_list) {
            LOGD("There is no message\n");
            return NULL;
        }

        mesgs_size = server_get_all_mesgs_size(mesg_list);
        if (!mesgs_size) {
            LOGD("There is no message\n");
            return NULL;
        }
        payload_len =  sizeof(int) + mesgs_size + (count * (sizeof(long int) + sizeof(int)));
        if (payload_len <= 0) {
            LOGD("payload_len was wrong\n");
            return NULL;
        }

        payload = (char*) malloc(payload_len);
        if (!payload) {
            LOGD("Failed to make buf\n");
            return NULL;
        }

        mesgs = message_create_array(count);
        if (!mesgs) {
            LOGD("Failed to create mesg array\n");
            return NULL;
        }

        result = server_copy_mesgs(mesg_list, mesgs, count);
        if (result == FALSE) {
            LOGD("Failed to copy the mesg list\n");
            return NULL;
        }
        result = convert_mesgs_to_payload(mesgs, payload, count);
        if (result == FALSE) {
            LOGD("Failed to convert mesg to payload\n");
            return NULL;
        }

        *read_pos = count;
        op_code = RES_ALL_MSG;

        break;

    case SND_MSG:
        payload_len = packet_get_payload_len(req_packet, NULL);
        LOGD("payload_len:%ld\n", payload_len);
        if (payload_len == -1) {
            LOGD("Failed to get the payload_len\n");
            return NULL;
        }

        req_payload = packet_get_payload(req_packet, NULL);
        if (!req_payload) {
            LOGD("Failed to get the payload\n");
            return NULL;
        }

        payload = (char*) malloc(payload_len);
        if (!payload) {
            LOGD("Failed to make buf for payload\n");
            return NULL;
        }

        memcpy(payload, req_payload, payload_len);

        mesg = convert_payload_to_mesg(payload, NULL);
        if (!mesg) {
            LOGD("Failed to convert payload to the Message\n");
            return NULL;
        }

        result = message_db_add_mesg(server->mesg_db, mesg);
        if (!result) {
            LOGD("Failed to add the Message\n");
            return NULL;
        }

        op_code = RCV_MSG;
        break;

    case REQ_FIRST_OR_LAST_MSG:
        LOGD("REQ_FIRST_OR_LAST_MSG\n");
        if (!read_pos) {
            LOGD("Can't set last read pos\n");
            return NULL;
        }

        payload_len = packet_get_payload_len(req_packet, NULL);
        LOGD("payload_len:%ld\n", payload_len);
        payload = packet_get_payload(req_packet, NULL);
        if (!payload) {
            LOGD("Failed to get the payload\n");
            return NULL;
        }
        memcpy(&more, payload, sizeof(char));
        LOGD("more:%c\n", more);
        if (more == '0') {
            pos = 1;
        } else {
            pos = client->last_read_pos;
        }
        LOGD("pos:%d\n", pos);

        if (pos < 0) {
            LOGD("Can't read message\n");
            return NULL;
        }

        count = message_db_get_message_count(server->mesg_db);
        LOGD("count:%d\n", count);
        if (count < 0) {
            LOGD("Faield to get message\n");
            return NULL;
        }

        if (count == 0) {
            LOGD("Thre is no message\n");
            return NULL;
        }

        if (count > 10) {
            read_count = 10;
        } else {
            read_count = count;
        }

        mesg_list = message_db_get_messages(server->mesg_db, pos, read_count);
        if (!mesg_list) {
            LOGD("There is no message\n");
            return NULL;
        }

        mesg_len = d_list_length(mesg_list);
        LOGD("mesg_len:%d\n", mesg_len);
        if (!mesg_len) {
            LOGD("There is no message\n");
            return NULL;
        }

        *read_pos = pos + mesg_len - 1;

        mesgs_size = server_get_all_mesgs_size(mesg_list);
        if (!mesgs_size) {
            LOGD("There is no message\n");
            return NULL;
        }
        payload_len =  sizeof(int) + mesgs_size + (mesg_len * (sizeof(long int) + sizeof(int)));
        if (payload_len <= 0) {
            LOGD("payload_len was wrong\n");
            return NULL;
        }

        payload = (char*) malloc(payload_len);
        if (!payload) {
            LOGD("Failed to make buf\n");
            return NULL;
        }

        mesgs = message_create_array(mesg_len);
        if (!mesgs) {
            LOGD("Failed to create mesg array\n");
            return NULL;
        }

        result = server_copy_mesgs(mesg_list, mesgs, mesg_len);
        if (result == FALSE) {
            LOGD("Failed to copy the mesg list\n");
            return NULL;
        }
        result = convert_mesgs_to_payload(mesgs, payload, mesg_len);
        if (result == FALSE) {
            LOGD("Failed to convert mesg to payload\n");
            return NULL;
        }

        op_code = RCV_FIRST_OR_LAST_MSG; 
        break;

    default:
        LOGD("The OPCODE is wrong\n");
        return NULL;
    }

    packet_len = HEADER_SIZE + payload_len + TAIL_SIZE;
    packet_buf = (char*) malloc(packet_len);
    if (!packet_buf) {
        LOGD("Failed to maked the packet_buf\n");
        return NULL;
    }

    tmp = packet_buf;

    memcpy(tmp, &sop, sizeof(sop));
    tmp += sizeof(sop);

    memcpy(tmp, &op_code, sizeof(op_code));
    tmp += sizeof(op_code);

    memcpy(tmp, &payload_len, sizeof(payload_len));
    tmp += sizeof(payload_len);

    memcpy(tmp, payload, payload_len);
    tmp += payload_len;

    memcpy(tmp, &eop, sizeof(eop));
    tmp += sizeof(eop);

    check_sum = packet_create_checksum(NULL, packet_buf, packet_len);
    if (check_sum == -1) {
        LOGD("Failed to do check_sum\n");
        free(packet_buf);
        return NULL;
    }
    memcpy(tmp, &check_sum, sizeof(check_sum));

    res_packet = new_packet(packet_buf);
    if (!res_packet) {
        LOGD("Failed to make the Packet\n");
        return NULL;
    }

    free(packet_buf);
    free(payload);

    LOGD("Finished server_create_res_packet()\n"); 
    return res_packet;
}

static int server_copy_payload_len(Stream_Buf *stream_buf, long int *payload_len) {
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

static int server_is_checksum_true(short check_sum, char *buf, int packet_len) {
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

static int server_send_packet_to_all_clients(Server *server, Client *comp_client, Packet *packet) {
    DList *list;
    Client *client;
    int fd, result, len;
    char *buf;

    if (!server || !comp_client || !packet) {
        LOGD("Can't send the packet to all clients");
        return FALSE;
    }

    len = packet_get_len(packet);
    if (len == -1) {
        LOGD("Failed to get the packet len\n");
        return FALSE;
    }

    buf = (char*) malloc(len);
    if (!buf) {
        LOGD("Failed to make the buf to copy the Packet\n");
        return FALSE;
    }

    result = convert_packet_to_buf(packet, buf);
    if (result == FALSE) {
        LOGD("Failed to convert the Packet to buf\n");
        return FALSE;
    }
  
    list = server->client_list;

    while(list) {
        client = (Client*) d_list_get_data(list);
        if (!comp_client) {
            continue;
        }

        if (comp_client != client) {
            fd = client->fd;
            if (fd < 0) {
                LOGD("The fd is wrong\n");
                return FALSE;
            }

            LOGD("Send write the Packet to the Client fd:%d\n", fd);
            if (write(fd, buf, len) < 0) {
                LOGD("Failed to send the Packet to server\n");
                return FALSE;
            }
        }
        list = d_list_next(list);
    }
    return TRUE;
}

static int server_send_packet_to_client(Packet *packet, Client *client, int read_pos) {
    int len, result;
    char *buf;

    if (!packet || !client) {
        LOGD("There is nothing to point the pocket or client\n");
        return FALSE;
    }

    if (client->fd < 0) {
        LOGD("the fd was wrong\n");
        return FALSE;
    }

    len = packet_get_len(packet);
    if (len == -1) {
        LOGD("Failed to get the packet len\n");
        return FALSE;
    }

    buf = (char*) malloc(len);
    if (!buf) {
        LOGD("Failed to make the buf to copy the Packet\n");
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

    client->last_read_pos = read_pos;

    return TRUE;
}

static void server_handle_disconnect_event(Server *server, int fd) {
    if (fd != server->fd) {
        server_remove_client(server, fd);
        looper_remove_watcher(server->looper, fd);
    }
    LOGD("disconnected:%d\n", fd);
}

static int server_copy_overread_buf(Stream_Buf *c_stream_buf, Stream_Buf *r_stream_buf, int packet_len, int len) {
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

static int server_check_overread(Stream_Buf *r_stream_buf, int packet_len) {
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

static void server_handle_req_packet(Server *server, Client *client, Packet *req_packet) {
    short op_code;
    int count;
    int read_pos;
    Packet *res_packet;

    if (!req_packet || !server || !client) {
        LOGD("There is nothing to point the Packet\n");
        return;
    }
    op_code = packet_get_op_code(req_packet, NULL);
    if (op_code == -1) {
        LOGD("Failed to get the op_code\n");
        return;
    }
    read_pos = 0;
    count = message_db_get_message_count(server->mesg_db);
    LOGD("count:%d\n", count);

    switch (op_code) {
    case REQ_ALL_MSG:
        if (!count) {
            LOGD("There is no message\n");
            return;
        }
        res_packet = server_create_res_packet(server, req_packet, client, &read_pos);
        if (!res_packet) {
            LOGD("Failed to create the res packet\n");
            return;
        }
        server_send_packet_to_client(res_packet, client, read_pos);
        break;

    case SND_MSG:
        res_packet = server_create_res_packet(server, req_packet, NULL, NULL);
        if (!res_packet) {
            LOGD("Failed to create the res packet\n");
            return;
        }
        server_send_packet_to_all_clients(server, client, res_packet);
        break;

    case REQ_FIRST_OR_LAST_MSG:
        if (!count) {
            LOGD("There is no message\n");
            return;
        }
        res_packet = server_create_res_packet(server, req_packet, client, &read_pos);
        if (!res_packet) {
            LOGD("Failed to create the res packet\n");
            return;
        }
        server_send_packet_to_client(res_packet, client, read_pos);

        break;

    default:
        LOGD("OPCODE is wrong\n");
        return;
    }
    destroy_packet(res_packet);
}

static void server_handle_req_event(Server *server, int fd) {
    char *buf;
    Client *client;
    short check_sum;
    Stream_Buf *stream_buf, *r_stream_buf, *c_stream_buf;
    int n_byte, read_len, packet_len, result, len, buf_size;
    long int payload_len;
    Packet *packet;

    payload_len = 0;
    packet_len = 0;
    buf_size = 0;
    buf = NULL;
    r_stream_buf = NULL;

    LOGD("Start server_handle_req_event()\n");

    if (!server) {
        LOGD("There is nothing to point the Server\n");
        return;
    }

    client = server_find_client(server, fd);
    if (!client) {
        LOGD("There is nothing to point the Client\n");
        return;
    }

    if (client->read_state == READY_TO_READ_REQ) {
        LOGD("READY_TO_READ_REQ\n");
        packet_len = client->packet_len;
        if (buf) {
            free(buf);
        }

        stream_buf = d_list_get_data(d_list_last(client->stream_buf_list));
        if (stream_buf == NULL || stream_buf_get_available_size(stream_buf) == 0) {
            stream_buf = new_stream_buf(MAX_BUF_LEN);
            client->stream_buf_list = d_list_append(client->stream_buf_list, stream_buf);
        }

        n_byte = read(fd, stream_buf_get_available(stream_buf), stream_buf_get_available_size(stream_buf));
        if (n_byte < 0) {
            LOGD("Failed to read\n");
            server_destroy_client_stream_buf_list(client);
            return;
        }

        result = stream_buf_increase_pos(stream_buf, n_byte);
        if (result == FALSE) {
            LOGD("Failed to set the position\n");
            server_destroy_client_stream_buf_list(client);
            return;
        }

        read_len = server_get_read_size(client->stream_buf_list);
        if (read_len < HEADER_SIZE) {
            LOGD("Not enough\n");
            return;
        }

        r_stream_buf = new_stream_buf(read_len);
        if (!r_stream_buf) {
            LOGD("Failed to make the Stream Buf\n");
            return;
        }

        result = server_append_data_to_buf(client->stream_buf_list, r_stream_buf);
        if (result == FALSE) {
            LOGD("Failed to append data of stream buf list to buf\n");
            destroy_stream_buf(r_stream_buf);
            return;
        }

        result = server_copy_payload_len(r_stream_buf, &payload_len);
        if (result == FALSE) {
            LOGD("Failed to  copy payload_len\n");
            return;
        }

        packet_len = HEADER_SIZE + payload_len + TAIL_SIZE;
        client->packet_len = packet_len;

        if (read_len >= packet_len) {
            client->read_state = FINISH_TO_READ_REQ;
            buf_size = packet_len;
        } else {
            client->read_state = START_TO_READ_REQ;
            buf_size = HEADER_SIZE;
        }

        buf = (char*) malloc(buf_size);
        if (!buf) {
            LOGD("Failed to make buf to check Header\n");
            server_destroy_client_stream_buf_list(client);
            destroy_stream_buf(r_stream_buf);
            return;
        }

        result = server_append_n_data_to_buf(r_stream_buf, buf, buf_size);
        if (result == FALSE) {
            LOGD("Failed to append n_data to buf\n");
            server_destroy_client_stream_buf_list(client);
            destroy_stream_buf(r_stream_buf);
            free(buf);
            return;
        }

        if (buf[0] != SOP) {
            LOGD("Failed to get binary\n");
            server_destroy_client_stream_buf_list(client);
            destroy_stream_buf(r_stream_buf);
            free(buf);
            return;
        }

        if (result == FALSE) {
            LOGD("Failed to copy payload\n");
            server_destroy_client_stream_buf_list(client);
            return;
        }
    }

    if (client->read_state == START_TO_READ_REQ) {
        LOGD("START_TO_READ_REQ\n");
        packet_len = client->packet_len;

        if (r_stream_buf) {
            destroy_stream_buf(r_stream_buf);
        }

        stream_buf = d_list_get_data(d_list_last(client->stream_buf_list));
        if (stream_buf == NULL || stream_buf_get_available_size(stream_buf) == 0) {
            stream_buf = new_stream_buf(MAX_BUF_LEN);
            client->stream_buf_list = d_list_append(client->stream_buf_list, stream_buf); 
        }
        LOGD("read\n");
        n_byte = read(fd, stream_buf_get_available(stream_buf), stream_buf_get_available_size(stream_buf));
        if (n_byte < 0) {
            LOGD("Failed to read\n");
            server_destroy_client_stream_buf_list(client);
            return;
        }

        result = stream_buf_increase_pos(stream_buf, n_byte);
        if (result == FALSE) {
            LOGD("Failed to set the position\n");
            server_destroy_client_stream_buf_list(client);
            return;
        }

        read_len = server_get_read_size(client->stream_buf_list);
        LOGD("read_len:%d\n", read_len);
        if (read_len < (client->packet_len)) {
            LOGD("Not enough\n");
            return;
        }

        client->read_state = FINISH_TO_READ_REQ;

        r_stream_buf = new_stream_buf(read_len);
        if (!r_stream_buf) {
            LOGD("Failed to make the Stream Buf\n");
            server_destroy_client_stream_buf_list(client);
            return;
        }

        result = server_append_data_to_buf(client->stream_buf_list, r_stream_buf);
        if (result == FALSE) {
            LOGD("Failed to append data of stream buf list to buf\n");
            server_destroy_client_stream_buf_list(client);
            destroy_stream_buf(r_stream_buf);
            return;
        }

        buf = (char*) malloc(packet_len);
        if (!buf) {
            LOGD("Failed to make buf to check Header\n");
            server_destroy_client_stream_buf_list(client);
            destroy_stream_buf(r_stream_buf);
            return;
        }

        result = server_append_n_data_to_buf(r_stream_buf, buf, packet_len);
        if (result == FALSE) {
            LOGD("Failed to append n_data to buf\n");
            server_destroy_client_stream_buf_list(client);
            destroy_stream_buf(r_stream_buf);
            free(buf);
            return;
        }
    }

    if (client->read_state == FINISH_TO_READ_REQ) {
        LOGD("FINISH_TO_READ_REQ\n");
        packet_len = client->packet_len;
        LOGD("packet_len:%d\n", packet_len);
        if (buf[packet_len -3] != EOP) {
            LOGD("Packet is wrong\n");
            server_destroy_client_stream_buf_list(client);
            destroy_stream_buf(r_stream_buf);
            free(buf);
            return;
        }

        check_sum = packet_create_checksum(NULL, buf, packet_len);
        LOGD("check_sum:%d\n", check_sum);
        result = server_is_checksum_true(check_sum, buf, packet_len);
        if (result == FALSE) {
            LOGD("Check_Sum is wrong\n");
            free(buf);
            server_destroy_client_stream_buf_list(client);
            destroy_stream_buf(r_stream_buf);
            return;
        }

        server_destroy_client_stream_buf_list(client);
        len = server_check_overread(r_stream_buf, packet_len);
        if (len > 0) {
            LOGD("Packet is overread\n");

            c_stream_buf = new_stream_buf(len);
            if (!c_stream_buf) {
                LOGD("Failed to make the Stream_Buf\n");
                return;
            }
            server_copy_overread_buf(c_stream_buf, r_stream_buf, packet_len, len);
            client->stream_buf_list = d_list_append(client->stream_buf_list, c_stream_buf);
        }

        destroy_stream_buf(r_stream_buf);
    }

    packet = new_packet(buf);
    if (!packet) {
        LOGD("Failed to make the Packet\n");
        return;
    }

    free(buf);

    server_handle_req_packet(server, client, packet);
    destroy_packet(packet);
    LOGD("Finished to server_handle_req_event()\n");
}

static int server_handle_accept_event(Server *server) {
    int client_fd;
    int result;

    client_fd = accept(server->fd, NULL, NULL);
    if (client_fd < 0) {
        LOGD("Failed to accept socket\n");
        return -1;
    }

    result = server_add_client(server, client_fd);
    if (result == -1) {
        LOGD("Failed to add the Client\n");
        return -1;
    }

    LOGD("connected:%d\n", client_fd);
    return client_fd;
}

static void server_handle_events(int fd, void *user_data, int looper_event) {
    int client_fd;
    Server *server;
    server = (Server*) user_data;

    if (!server || (fd < 0)) {
        LOGD("Can't handle events\n");
        return;
    }

    if (looper_event & LOOPER_HUP_EVENT) {
         server_handle_disconnect_event(server, fd);
    } else if (looper_event & LOOPER_IN_EVENT) {
        if (fd == server->fd) {
            client_fd = server_handle_accept_event(server);
            if (client_fd != -1) {
                looper_add_watcher(server->looper, client_fd, server_handle_events, server, LOOPER_IN_EVENT | LOOPER_HUP_EVENT);
            }
        } else {
            server_handle_req_event(server, fd);
        }
    }
}

Server* new_server(Looper *looper) {
    Server *server;
    MessageDB *mesg_db;
    char *data_format, *str;
    struct sockaddr_un addr;
    int server_fd;

    if (!looper) {
        LOGD("Looper is empty\n");
        return NULL;
    }

    server = (Server*) malloc(sizeof(Server));

    if (!server) {
        printf("%s %s Failed to make Server\n", __FILE__, __func__);
        return NULL;
    }

    if (access(SOCKET_PATH, F_OK) == 0) {
        if (unlink(SOCKET_PATH) < 0) {
            printf("%s %s Failed to unlink\n", __FILE__, __func__);
            return NULL;
        }
    }

    if ((server_fd = socket(AF_UNIX, SOCK_STREAM,0)) == -1) {
        printf("%s %s socket error", __FILE__, __func__);
        return NULL;
    }

    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, SOCKET_PATH, sizeof(addr.sun_path)-1);

    if (bind(server_fd, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
        printf("%s %s bind error", __FILE__, __func__);
        close(server_fd);
        return NULL;
    }

    if (listen(server_fd, 0) == -1) {
        printf("%s %s listen error", __FILE__, __func__);
        close(server_fd);
        return NULL;
    }
    
    data_format = (char*) malloc(2);
    if (!data_format) {
        LOGD("Failed to make data_format\n");
        return NULL;
    }

    str = "is";
    memcpy(data_format, str, strlen(str));
    mesg_db = message_db_open(data_format);
    if (!mesg_db) {
        LOGD("Failed to make the MessageDB\n");
        return NULL;
    }

    server->looper = looper;
    server->fd = server_fd;
    server->client_list = NULL;
    server->mesg_db = mesg_db;

    looper_add_watcher(server->looper, server_fd, server_handle_events, server, LOOPER_IN_EVENT | LOOPER_HUP_EVENT);

    return server;
}

void destroy_server(Server *server) {
    DList *list;

    if (!server) {
        printf("%s %s There is no pointer to Server\n", __FILE__, __func__);
        return;
    }

    looper_stop(server->looper);

    if (close(server->fd) < 0) {
        printf("%s %s Failed to close server\n", __FILE__, __func__);
        return;
    }

    list = server->client_list;
    d_list_free(list, server_destroy_client);

    looper_remove_all_watchers(server->looper);
    free(server);
}
