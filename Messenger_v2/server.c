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
#include "mesg_file_db.h"
#include "mesg_file.h"
#include "socket.h"
#include "packet.h"
#include "stream_buf.h"
#include "m_boolean.h"
#include "utils.h"
#include "converter.h"
#include "message.h"

struct _Server {
    int fd;
    DList *client_list;
    DList *mesg_list;
    DList *last_read_mesg_node;
    Looper *looper;
    Mesg_File *mesg_file;
    Mesg_File_DB *mesg_file_db;
};

struct _Client {
    int fd;
    int file_offset;
    READ_REQ_STATE read_state;
    DList *stream_buf_list;
    int packet_len;
};

static void destroy_client(void *client) {
    Client *remove_client;

    if (!client) {
        printf("%s %s There is nothing to point the Client\n", __FILE__, __func__);
        return;
    }
    remove_client = (Client*) client;

    if (close(remove_client->fd) < 0) {
        printf("%s %s Falied to close client fd\n", __FILE__, __func__);
    }
    free(remove_client);
}

static int match_client(void *data1, void *data2) {
    Client *client = (Client*) data1;
    int fd = *((int*) data2);

    if (!client) {
        printf("%s %s There is nothing to point the Client\n", __FILE__, __func__);
    }

    if (client->fd == fd) {
        return 1;
    } else {
        return 0;
    }
}

static Client* find_client(Server *server, int fd) {
    Client *client = NULL;
    client = (Client*) d_list_find_data(server->client_list, match_client, &fd);

    if (client == NULL) {
        printf("%s %s Can't find Client\n", __FILE__, __func__);
        return NULL;
    }
    return client;
}

static ADD_CLIENT_RESULT add_client(Server *server, int fd) {
    Client *client;
    DList *list;

    list = server->client_list;
    client = (Client*) malloc(sizeof(Client));

    if (!client) {
        printf("%s %s Failed to make Client\n", __FILE__, __func__);
        return ADD_CLIENT_FAILURE;
    }

    client->fd = fd;
    client->file_offset = 0;
    client->read_state = READY_TO_READ_REQ;
    client->stream_buf_list = NULL;
    client->packet_len = 0;

    server->client_list = d_list_append(list, client);
    return ADD_CLIENT_SUCCESS;
}

static void remove_client(Server *server, int fd) {
    if (!server && !server->client_list) {
        printf("%s %s Can't remove Client\n", __FILE__, __func__);
        return;
    }

    server->client_list = d_list_remove_with_user_data(server->client_list, &fd, match_client, destroy_client);
}

static void sum_size(void *data, void *user_data) {
    int *size;
    Stream_Buf *stream_buf;

    if (!data || !user_data) {
        LOGD("Can't sum the stream buf list size\n");
        return;
    }

    stream_buf = (Stream_Buf*) data;
    size = (int*) user_data;

    *size += get_position(stream_buf);
}

static int get_read_size(DList *stream_buf_list) {
    int size;

    if (!stream_buf_list) {
        LOGD("There is nothing to point the stream_buf_list\n");
        return -1;
    }

    size = 0;
    d_list_foreach(stream_buf_list, sum_size, &size);
    return size;
}

static void destroy_stream_buf_list(void *data) {
    Stream_Buf *stream_buf;

    stream_buf = (Stream_Buf*) data;

    if (!stream_buf) {
        LOGD("There is nothing to remove the Stream_Buf\n");
        return;
    }
    destroy_stream_buf(stream_buf);
}

static void destroy_client_stream_buf_list(Client* client) {
    if (!client) {
        LOGD("There is nothing to point the Client\n");
        return;
    }
    d_list_free(client->stream_buf_list, destroy_stream_buf_list);
    client->stream_buf_list = NULL;
    client->read_state = READY_TO_READ_REQ;
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

static char append_data_to_buf(DList *stream_buf_list, Stream_Buf *stream_buf) {
    if (!stream_buf_list || !stream_buf) {
        LOGD("Can't append data to Stream_Buf\n");
        return FALSE;
    }
    d_list_foreach(stream_buf_list, append_data, stream_buf);
    return TRUE;
}

static int copy_payload(char *payload, long int payload_len, char *buf) {
    if (!payload || !buf) {
        LOGD("Can't copy payload\n");
        return FALSE;
    }

    memcpy(buf, payload, payload_len);
    return TRUE;
}

static void sum_mesgs_size(void *data, void *user_data) {
    int *size;
    Message *mesg;

    if (!data || !user_data) {
        LOGD("Can't sum the mesgs size\n");
        return;
    }

    mesg = (Message *) data;
    size = (int*) user_data;

    *size += get_str_len(mesg);
}

static int get_all_mesgs_size(DList *mesg_list) {
    int size;

    if (!mesg_list) {
        LOGD("There is nothing to point the mesg list\n");
        return -1;
    }

    d_list_foreach(mesg_list, sum_mesgs_size, &size);
    return size;
}

static int copy_mesgs(DList *mesg_list, Message *mesgs, int len) {
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
        mesg2 = next_mesg(mesgs, i);
        if (!mesg1 || !mesg2) {
            LOGD("Can't copy mesgs\n");
            return FALSE;
        }

        time = get_time(mesg1);
        if (time == -1) {
            LOGD("Failed to get the time\n");
            return FALSE;
        }
        result = set_time(mesg2, time);
        if (result == FALSE) {
            LOGD("Failed to set time\n");
            return FALSE;
        }

        str_len = get_str_len(mesg1);
        if (str_len == -1) {
            LOGD("Failed to get the str_len\n");
            return FALSE;
        }
        result = set_str_len(mesg2, str_len);
        if (result == FALSE) {
            LOGD("Failed to set str len\n");
            return FALSE;
        }

        str = get_str(mesg1);
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
        result = set_str(mesg2, buf);
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

static Packet* create_res_packet(Server *server, Packet *req_packet) {
    short op_code, check_sum;
    char *payload, *buf;
    Packet *res_packet;
    Header *header;
    Body *body;
    Tail *tail;
    int result; 
    int payload_len;
    int mesgs_size, list_len; 
    Message *mesg, *mesgs;

    LOGD("Start create_res_packet()\n");

    if (!req_packet) {
        LOGD("There is nothing to point the Packet\n");
        return NULL;
    }

    header = new_header(SOP, 0, 0);
    if (!header) {
        LOGD("Failed to make the Header\n");
        return NULL;
    }

    body = new_body(NULL);
    if (!body) {
        LOGD("Failed to make the Body\n");
        return NULL;
    }

    tail = new_tail(EOP, 0);
    if (!tail) {
        LOGD("Failed to make the Tail\n");
        return NULL;
    }

    res_packet = new_packet(header, body, tail);

    if (!res_packet) {
        LOGD("There is nothing to point the Packet\n");
        return NULL;
    }

    op_code = get_op_code(req_packet, NULL);

    switch(op_code) {
    case REQ_ALL_MSG:
        LOGD("REQ_ALL_MSG\n");
        mesgs_size = get_all_mesgs_size(server->mesg_list);
        if (mesgs_size == -1) {
            LOGD("Failed to get all mesgs size\n");
            return NULL;
        }

        list_len = d_list_length(server->mesg_list);
        payload_len =  sizeof(int) + mesgs_size + (list_len * (sizeof(long int) + sizeof(int)));
        LOGD("payload_len:%d\n", payload_len);
        result = set_payload_len(res_packet, payload_len);
        if (result == FALSE) {
            LOGD("Failed to set the payload_len\n");
            return NULL;
        }
        
        payload = (char*) malloc(payload_len);
        if (!payload) {
            LOGD("Failed to make buf\n");
            return NULL;
        }

        mesgs = create_mesg_array(list_len);
        if (!mesgs) {
            LOGD("Failed to create mesg array\n");
            return NULL;
        }

        result = copy_mesgs(server->mesg_list, mesgs, list_len);
        if (result == FALSE) {
            LOGD("Failed to copy the mesg list\n");
            return NULL;
        }
        result = convert_mesgs_to_payload(mesgs, payload, list_len);
        if (result == FALSE) {
            LOGD("Failed to convert mesg to payload\n");
            return NULL;
        }

        result = set_payload(res_packet, payload);
        if (result == FALSE) {
            LOGD("Failed to set the payload\n");
            destroy_packet(res_packet);
            return NULL;
        }
        break;
    case SND_MSG:
        payload_len = get_payload_len(req_packet, NULL);
        if (payload_len == -1) {
            LOGD("Failed to get the payload_len\n");
            destroy_packet(res_packet);
            return NULL;
        }

        result = set_payload_len(res_packet, payload_len);
        if (result == FALSE) {
            LOGD("Failed to set payload_len\n");
            destroy_packet(res_packet);
            return NULL;
        }

        payload = get_payload(req_packet, NULL);
        if (!payload) {
            LOGD("Failed to get the payload\n");
            destroy_packet(res_packet);
            return NULL;
        }

        buf = (char*) malloc(payload_len);
        if (!buf) {
            LOGD("Failed to make the buf for payload\n");
            destroy_packet(res_packet);
            return NULL;
        }

        result = copy_payload(payload, payload_len, buf);
        if (result == FALSE) {
            LOGD("Failed to copy the payload\n");
            destroy_packet(res_packet);
            return NULL;
        }

        result = set_payload(res_packet, buf);
        if (result == FALSE) {
            LOGD("Failed to set the payload\n");
            destroy_packet(res_packet);
            return NULL;
        }

        mesg = new_mesg(0, 0, 0);
        if (!mesg) {
            LOGD("Failed to make the Message\n");
            destroy_packet(res_packet);
            return NULL;
        }

        server->mesg_list = d_list_append(server->mesg_list, mesg);
        if (!(server->mesg_list)) {
            LOGD("Failed to add the Message\n");
            destroy_packet(res_packet);
            return NULL;
        }

        op_code = RCV_MSG;

        result = convert_payload_to_mesg(payload, mesg);
        if (result == FALSE) {
            LOGD("Failed to convert payload to the Message\n");
            destroy_packet(req_packet);
            return NULL;
        }
        break;
    case REQ_FIRST_OR_LAST_MSG:
        LOGD("REQ_FIRST_OR_LAST_MSG\n");
        payload_len = get_payload_len(req_packet, NULL);
        LOGD("payload_len:%d\n", payload_len);
        payload = get_payload(req_packet, NULL);
        if (!payload) {
            LOGD("Failed to get the payload\n");
            return NULL;
        }
        break;
    default:
        return NULL;
    }

    result = set_op_code(res_packet, op_code);
    if (result == FALSE) {
        LOGD("Failed to set the op_code\n");
        destroy_packet(res_packet);
        return NULL;
    }

    check_sum = create_check_sum(res_packet, NULL);
    if (check_sum == -1) {
        LOGD("Failed to do check_sum\n");
        destroy_packet(res_packet);
        return NULL;
    }

    result = set_check_sum(res_packet, check_sum);
    if (result == FALSE) {
        LOGD("Failed to set the check_sum\n");
        destroy_packet(res_packet);
        return NULL;
    }

    LOGD("Finished create_res_packet()\n"); 
    return res_packet;
}

static short copy_payload_len(char *buf, long int *payload_len) {
    if (!buf) {
        LOGD("There is nothing to point the buf\n");
        return FALSE;
    }
    buf = buf + sizeof(char) + sizeof(short);

    memcpy(payload_len, buf, sizeof(long int));
    return TRUE;
}

static int is_check_sum_true(short check_sum, Stream_Buf *stream_buf) {
    int position;
    char *buf;
    int i;
    short comp_check_sum;

    if (!stream_buf) {
        LOGD("Failed to check enable check_sum\n");
        return FALSE;
    }

    position = get_position(stream_buf);
    buf = get_buf(stream_buf);

    for (i = position -1 ; i < position; i++) {
        comp_check_sum += buf[i];
    }

    if (comp_check_sum == check_sum) {
        LOGD("Not enabled check sum\n");
        return FALSE;
    }

    return TRUE;
}

static int send_packet_to_all_clients(Server *server, Client *comp_client, Packet *packet) {
    DList *list;
    Client *client;
    int fd, result, len;
    char *buf;

    if (!server || !comp_client || !packet) {
        LOGD("Can't send the packet to all clients");
        return FALSE;
    }

    len = get_packet_len(packet);
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

static int send_packet_to_client(Packet *packet, int fd) {
    int len, result;
    char *buf;

    if (!packet) {
        LOGD("There is nothing to point the Packet\n");
        return FALSE;
    }

    len = get_packet_len(packet);
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
    
    if (write(fd, buf, len) < 0) {
        LOGD("Failed to send the Packet to server\n");
        return FALSE;
    }

    return TRUE;
}

static void handle_disconnect_event(Server *server, int fd) {
    if (fd != server->fd) {
        remove_client(server, fd);
        remove_watcher(server->looper, fd);
    }
    printf("%s %s disconnected:%d\n", __FILE__, __func__, fd);
}

static void handle_req_event(Server *server, int fd) {
    char *buf;
    Client *client;
    short check_sum, op_code;
    Stream_Buf *stream_buf;
    int n_byte, read_len, packet_len, position, result;
    long int payload_len;
    Packet *packet, *res_packet;

    payload_len = 0;

    if (!server) {
        LOGD("There is nothing to point the Server\n");
        return;
    }

    client = find_client(server, fd);
    if (!client) {
        LOGD("There is nothing to point the Client\n");
        return;
    }

    LOGD("Start handle_req_event()\n");

    if (client->read_state == READY_TO_READ_REQ) {

        stream_buf = d_list_get_data(d_list_last(client->stream_buf_list));
        if (stream_buf == NULL || get_available_size(stream_buf) == 0) {
            stream_buf = new_stream_buf(1);
            client->stream_buf_list = d_list_append(client->stream_buf_list, stream_buf);
        }

        n_byte = read(fd, get_available_buf(stream_buf), get_available_size(stream_buf));
        LOGD("stream_buf = %p, n_byte = %d\n", stream_buf, n_byte);
        if (n_byte < 0) {
            LOGD("Failed to read\n");
            destroy_client_stream_buf_list(client);
            return;
        }

        result = increase_position(stream_buf, n_byte);
        if (result == FALSE) {
            LOGD("Failed to set the position\n");
            destroy_client_stream_buf_list(client);
            return;
        }

        if (get_read_size(client->stream_buf_list) < HEADER_SIZE) {
            LOGD("not enough\n");
            return;
        }

        client->read_state = START_TO_READ_REQ;
        Stream_Buf *r_stream_buf = new_stream_buf(HEADER_SIZE);
        append_data_to_buf(client->stream_buf_list, r_stream_buf);

        buf = get_buf(r_stream_buf);
        if (buf[0] != SOP) {
            LOGD("Failed to get binary\n");
            destroy_client_stream_buf_list(client);
            return;
        }
        result = copy_payload_len(buf, &payload_len);
        if (result == FALSE) {
            LOGD("Failed to copy payload\n");
            destroy_client_stream_buf_list(client);
            return;
        }
        client->packet_len = HEADER_SIZE + payload_len + TAIL_SIZE;
    }

    if (client->read_state == START_TO_READ_REQ) {
        stream_buf = new_stream_buf(MAX_BUF_LEN);
        client->stream_buf_list = d_list_append(client->stream_buf_list, stream_buf);
        packet_len = client->packet_len;
        read_len = get_read_size(client->stream_buf_list);

        do {
            position = get_position(stream_buf);
            if (position >= MAX_BUF_LEN) {
                stream_buf = new_stream_buf(MAX_BUF_LEN);
                client->stream_buf_list = d_list_append(client->stream_buf_list, stream_buf);
            }

            n_byte = read(fd, get_available_buf(stream_buf), get_available_size(stream_buf));
            if (n_byte < 0) {
                LOGD("Failed to read\n");
                destroy_client_stream_buf_list(client);
                return;
            }
            read_len += n_byte;
            result = increase_position(stream_buf, n_byte);
            if (result == FALSE) {
                LOGD("Failed to set the position\n");
                destroy_client_stream_buf_list(client);
                return;
            }
        } while ((read_len - packet_len));
    }

    read_len = get_read_size(client->stream_buf_list);

    if (!buf) {
        LOGD("Failed to make buf\n");
        return;
    }

    stream_buf = new_stream_buf(read_len);
    if (!stream_buf) {
        LOGD("Failed to make the Stream buf");
        return;
    }

    result = append_data_to_buf(client->stream_buf_list, stream_buf);
    destroy_client_stream_buf_list(client);
    if (result == FALSE) {
        LOGD("Failed to append input data to buf\n");
        destroy_stream_buf(stream_buf);
        return;
    }

    buf = get_buf(stream_buf);
    if (!buf) {
        LOGD("There is nothing to point the buf in the Stream Buf\n");
        destroy_stream_buf(stream_buf);
        return;
    }

    if (buf[read_len -3] != EOP) {
        LOGD("Packet is wrong\n");
        destroy_stream_buf(stream_buf);
        return;
    }

    check_sum = create_check_sum(NULL, stream_buf);
    result = is_check_sum_true(check_sum, stream_buf);

    if (result == FALSE) {
        LOGD("check_sum is wrong\n");
        destroy_stream_buf(stream_buf);
        return;
    }

    packet = new_packet(NULL, NULL, NULL);
    if (!packet) {
        LOGD("Failed to make the Packet\n");
        return;
    }

    result = convert_buf_to_packet(buf, packet);
    destroy_stream_buf(stream_buf);
    if (result == FALSE) {
        LOGD("Faield to convert buf to packet\n");
        return;
    }

    res_packet = create_res_packet(server, packet);
    if (!res_packet) {
        LOGD("Failed to create the res packet\n");
        destroy_packet(packet);
        return;
    }

    op_code = get_op_code(res_packet, NULL);

    if (op_code == RCV_MSG) {
        result = send_packet_to_all_clients(server, client, res_packet);
    } else {
        result = send_packet_to_client(res_packet, fd);
    }

    if (result == FALSE) {
        LOGD("Failed to send packet to client\n");
        destroy_packet(res_packet);
        return;
    }

    destroy_packet(packet);
    destroy_packet(res_packet);
    LOGD("Finished to handle_req_event()\n");
}

static int handle_accept_event(Server *server) {
    int client_fd;
    ADD_CLIENT_RESULT result;

    client_fd = accept(server->fd, NULL, NULL);
    if (client_fd < 0) {
        LOGD("Failed to accept socket\n");
        return -1;
    }

    result = add_client(server, client_fd);
    if (result == ADD_CLIENT_FAILURE) {
        LOGD("Failed to add the Client\n");
        return -1;
    }

    LOGD("connected:%d\n", client_fd);
    return client_fd;
}

static void handle_events(int fd, void *user_data, int looper_event) {
    int client_fd;
    Server *server;
    server = (Server*) user_data;

    if (!server || (fd < 0)) {
        LOGD("Can't handle events\n");
        return;
    }

    if (looper_event & LOOPER_HUP_EVENT) {
         handle_disconnect_event(server, fd);
    } else if (looper_event & LOOPER_IN_EVENT) {
        if (fd == server->fd) {
            client_fd = handle_accept_event(server);
            if (client_fd != -1) {
                add_watcher(server->looper, client_fd, handle_events, server, LOOPER_IN_EVENT | LOOPER_HUP_EVENT);
            }
        } else {
            handle_req_event(server, fd);
        }
    }
}

Server* new_server(Looper *looper) {
    Server *server;
    struct sockaddr_un addr;
    int server_fd;
    Mesg_File *mesg_file;
    Mesg_File_DB *mesg_file_db;

    if (!looper) {
        printf("%s %s Looper is empty\n", __FILE__, __func__);
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

    mesg_file = new_mesg_file();
    mesg_file_db = new_mesg_file_db();

    if (!mesg_file && !mesg_file_db) {
        printf("%s %s There is no a pointer to Mesg_File or Mesg_File_DB\n", __FILE__, __func__);
        return NULL;
    }

    server->looper = looper;
    server->fd = server_fd;
    server->client_list = NULL;
    server->mesg_list = NULL;
    server->mesg_file = mesg_file;
    server->mesg_file_db = mesg_file_db;

    add_watcher(server->looper, server_fd, handle_events, server, LOOPER_IN_EVENT | LOOPER_HUP_EVENT);

    return server;
}

void destroy_server(Server *server) {
    DList *list;

    if (!server) {
        printf("%s %s There is no pointer to Server\n", __FILE__, __func__);
        return;
    }

    stop(server->looper);

    if (close(server->fd) < 0) {
        printf("%s %s Failed to close server\n", __FILE__, __func__);
        return;
    }

    list = server->client_list;
    d_list_free(list, destroy_client);

    remove_all_watchers(server->looper);
    destroy_mesg_file_db(server->mesg_file_db);
    destroy_mesg_file(server->mesg_file);

    free(server);
}
