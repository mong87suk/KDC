#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <sys/un.h>
#include <string.h>
#include <sys/types.h>

#include <kdc/DBLinkedList.h>

#include "server.h"
#include "looper.h"
#include "socket.h"
#include "packet.h"
#include "stream_buf.h"
#include "m_boolean.h"
#include "utils.h"
#include "converter.h"
#include "message.h"
#include "message_db.h"
#include "account_db.h"
#include "account.h"
#include "result_type.h"

struct _Server {
    int fd;
    unsigned int id;
    DList *client_list;
    DList *online_list;
    DList *last_read_mesg_node;
    MessageDB *mesg_db;
    AccountDB *account_db;
    Looper *looper;
};

struct _Client {
    int id;
    int fd;
    int last_read_pos;
    READ_REQ_STATE read_state;
    DList *stream_buf_list;
    int packet_len;
};

struct _Online {
    int id;
    int fd;
};

struct _UserData {
    unsigned int id;
    Server *server;
    Packet *packet;
    int snd_type;
    int fd;
};

struct _LoginData {
    DList *login_list;
    Server *server;
    int len;
};

static void server_free_message(void *data) {
    Message *mesg;

    mesg = (Message *) data;

    if (!mesg) {
        LOGD("There is nothing to point the Message\n");
        return;
    }

    destroy_mesg(mesg);
}

static void server_destroy_mesg_list(DList *mesg_list) {
    d_list_free(mesg_list, server_free_message);
}

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
    unsigned int id = *((int*) data2);

    if (!client) {
        LOGD("There is nothing to point the Client\n");
    }

    if (client->id == id) {
        return 1;
    } else {
        return 0;
    }
}

static Client *server_find_client(Server *server, unsigned int id) {
    Client *client = NULL;
    client = (Client*) d_list_find_data(server->client_list, server_match_client, &id);

    if (client == NULL) {
        LOGD("Can't find Client\n");
        return NULL;
    }
    return client;
}

static int server_match_client_with_fd(void *data1, void *data2) {
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

static Client *server_find_client_with_fd(Server *server, int fd) {
    Client *client = NULL;
    client = (Client*) d_list_find_data(server->client_list, server_match_client_with_fd, &fd);

    if (client == NULL) {
        LOGD("Can't find Client\n");
        return NULL;
    }
    return client;
}

static int server_add_client(Server *server, int fd, int id) {
    Client *client;
    DList *list;

    list = server->client_list;
    client = (Client*) malloc(sizeof(Client));

    if (!client) {
        LOGD("Failed to make Client\n");
        return -1;
    }

    client->id = id;
    client->fd = fd;
    client->last_read_pos = 1;
    client->read_state = READY_TO_READ_REQ;
    client->stream_buf_list = NULL;
    client->packet_len = 0;

    server->client_list = d_list_append(list, client);
    return 1;
}

static void server_remove_client(Server *server, unsigned int id) {
    server->client_list = d_list_remove_with_user_data(server->client_list, &id, server_match_client, server_destroy_client);
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

static BOOLEAN server_append_data_to_buf(DList *stream_buf_list, Stream_Buf *stream_buf) {
    if (!stream_buf_list || !stream_buf) {
        LOGD("Can't append data to Stream_Buf\n");
        return FALSE;
    }
    d_list_foreach(stream_buf_list, server_append_data, stream_buf);
    return TRUE;
}

static BOOLEAN server_append_n_data_to_buf(Stream_Buf *stream_buf, char *buf, int n) {
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

static BOOLEAN server_copy_mesgs(DList *mesg_list, Message *mesgs, int len) {
    int i, str_len;
    long int time;
    Message *mesg1, *mesg2;
    char *buf, *str;
    BOOLEAN result;

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

static char *server_new_account_info(char **payload) {
    int str_len = 0;

    memcpy(&str_len, *payload, STR_LEN_SIZE);
    if (str_len < 0) {
        LOGD("The Payload was wrong\n");
        return NULL;
    }
    *payload += STR_LEN_SIZE;

    char *info = (char*) malloc(str_len + 1);
    if (!info) {
        LOGD("Failed to make the buf\n");
        return NULL;
    }
    memset(info, 0, str_len + 1);
    memcpy(info, *payload, str_len);

    *payload += str_len;

    return info;    
}

static Account *server_new_account(char *payload) {
    char *user_id = server_new_account_info(&payload);
    char *pw = server_new_account_info(&payload);
    char *email = server_new_account_info(&payload);
    char *confirm = server_new_account_info(&payload);
    char *mobile = server_new_account_info(&payload);
    
    Account *account = new_account(user_id, pw, email, confirm, mobile);
    free(user_id);
    free(pw);
    free(email);
    free(confirm);
    free(mobile);

    if (!account) {
        LOGD("Failed to new account\n");
        return NULL;
    }

    return account;
}

static void free_online(Online *online) {
    free(online);
}

static void destroy_online(void *online) {
    free_online(online);
}

static Online *new_online(int fd, int id) {
    Online *online = (Online*) malloc(sizeof(Online));
    if (!online) {
        LOGD("Failed to new Online\n");
        return NULL;
    }

    online->fd = fd;
    online->id = id;

    return online;
}

static int server_match_online(void *data1, void *data2) {
    if (!data1) {
        return 0;
    }
    Online *online = (Online*) data1;

    int id = *((int *) data2);
    int cmp_id = online->id;

    if (id == cmp_id) {
        return 1;
    } else {
        return 0;
    }
}

static int server_add_online(Server *server, int id, int fd) {
    if (server->online_list && d_list_find_data(server->online_list, server_match_online, &id)) {
        LOGD("Failed to add online\n");
        return ERROR_LOGIN;
    }

    Online *online = new_online(fd, id);
    if (!online) {
        LOGD("Failed to new online\n");
        return ERROR_LOGIN;
    }

    server->online_list = d_list_append(server->online_list, online);
    return SUCCESS_LOG_IN;
}

static int server_login(Server *server, char *payload, int fd) {
    char *user_id = server_new_account_info(&payload);
    char *pw = server_new_account_info(&payload);
    LOGD("user_id:%s pw:%s\n", user_id, pw);
    int id = account_db_identify_account(server->account_db, user_id, pw);
    LOGD("id:%d\n", id);
    free(user_id);
    free(pw);

    if (id < 0) {
        LOGD("Failed to login\n");
        return ERROR_LOGIN;
    }

    int result = server_add_online(server, id, fd);
    return result;
}

static BOOLEAN server_delete_online(Server *server, int id, int fd) {
    if (!server->online_list) {
        LOGD("Can't delete the Online\n");
        return FALSE;
    }

    Online *online = (Online *) d_list_find_data(server->online_list, server_match_online, &id);
    if (!online) {
        LOGD("Can't delete the Online\n");
        return FALSE;
    }

    if (online->id != id || online->fd != fd) {
        LOGD("Can't delete the Online\n");
        return FALSE;
    }

    destroy_online(online);
    return TRUE;  
}

static int server_logout(Server *server, char *payload, int fd) {
    char *user_id = server_new_account_info(&payload);
    char *pw = server_new_account_info(&payload);

    int id = account_db_identify_account(server->account_db, user_id, pw);
    free(user_id);
    free(pw);

     if (id < 0) {
        LOGD("Failed to logout\n");
        return ERROR_LOGOUT;
    }
    
    BOOLEAN result = server_delete_online(server, id, fd);
    if (result == FALSE) {
        return ERROR_LOGOUT;
    }

    return SUCCESS_LOG_OUT;
}
 
static int server_delete_account(Server *server, char *payload, int fd) {
    char *user_id = server_new_account_info(&payload);
    char *pw = server_new_account_info(&payload);

    int id = account_db_delete_account(server->account_db, user_id, pw);
    free(user_id);
    free(pw);
    if (id < 0) {
        LOGD("Failed to delete the account\n");
        return ERROR_DELETE_ACCOUNT;
    }
    
    if (server->online_list) {
        Online *online = (Online *) d_list_find_data(server->online_list, server_match_online, &id);
        if (online) {
            destroy_online(online);
        }
    }
    return SUCCESS_DELETE_ACCOUNT;
}

static char *server_new_result(short op_code, int result_type) {
    char *result = (char*) malloc(RESULT_SIZE);
    if (!result) {
        LOGD("Failed to new reulst\n");
        return NULL;
    }
    char *tmp = result;

    memcpy(tmp, &op_code, OP_CODE_SIZE);
    tmp += OP_CODE_SIZE;
    memcpy(tmp, &result_type, RESULT_TYPE_SIZE);

    return result;
}

static Packet *server_create_res_packet(Server *server, Packet *req_packet, Client *client, int *read_pos, unsigned int *interval) {
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
    unsigned int micro_sec;
    int id;
    Account *account;

    LOGD("Start server_create_res_packet()\n");

    mesg = NULL;

    op_code = packet_get_op_code(req_packet, NULL);
    sop = SOP;
    eop = EOP;
    payload = NULL;
    payload_len = 0;
    micro_sec = 0;

    switch(op_code) {
        case REQ_ALL_MSG:
            count = message_db_get_message_count(server->mesg_db);
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
                server_destroy_mesg_list(mesg_list);
                return NULL;
            }
            payload_len =  sizeof(int) + mesgs_size + (count * (sizeof(long int) + sizeof(int)));
            if (payload_len <= 0) {
                LOGD("payload_len was wrong\n");
                server_destroy_mesg_list(mesg_list);
                return NULL;
            }

            payload = (char*) malloc(payload_len);
            if (!payload) {
                LOGD("Failed to make buf\n");
                server_destroy_mesg_list(mesg_list);
                return NULL;
            }

            mesgs = message_create_array(count);
            if (!mesgs) {
                LOGD("Failed to create mesg array\n");
                free(payload);
                server_destroy_mesg_list(mesg_list);
                return NULL;
            }

            result = server_copy_mesgs(mesg_list, mesgs, count);
            server_destroy_mesg_list(mesg_list);
            if (result == FALSE) {
                destroy_mesg(mesgs);
                free(payload);
                LOGD("Failed to copy the mesg list\n");
                return NULL;
            }
            result = convert_mesgs_to_payload(mesgs, payload, count);
            destroy_mesg(mesgs);
            if (result == FALSE) {
                free(payload);
                LOGD("Failed to convert mesg to payload\n");
                return NULL;
            }

            *read_pos = count;
            op_code = RES_ALL_MSG;

            break;

        case REQ_INTERVAL_MSG:
            payload_len = packet_get_payload_len(req_packet, NULL);
            if (payload_len == -1) {
                LOGD("Failed to get the payload_len\n");
                return NULL;
            }

            req_payload = packet_get_payload(req_packet, NULL);
            if (!req_payload) {
                LOGD("Failed to get the payload\n");
                return NULL;
            }

            memcpy(&micro_sec, req_payload, INTERVAL_SIZE);
            *interval = micro_sec;

            payload_len -= INTERVAL_SIZE;
            req_payload += INTERVAL_SIZE;

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

            id = message_db_add_mesg(server->mesg_db, mesg);
            destroy_mesg(mesg);
            if (id < 0) {
                LOGD("Failed to add mesg\n");
            }
            op_code = RCV_MSG;
            break;

        case REQ_FIRST_OR_LAST_MSG:
            if (!read_pos) {
                LOGD("Can't set last read pos\n");
                return NULL;
            }

            payload_len = packet_get_payload_len(req_packet, NULL);
            payload = packet_get_payload(req_packet, NULL);
            if (!payload) {
                LOGD("Failed to get the payload\n");
                return NULL;
            }
            memcpy(&more, payload, sizeof(char));
            if (more == '0') {
                pos = 1;
            } else {
                pos = client->last_read_pos;
            }
            
            if (pos < 0) {
                LOGD("Can't read message\n");
                return NULL;
            }

            count = message_db_get_message_count(server->mesg_db);
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
            if (!mesg_len) {
                server_destroy_mesg_list(mesg_list);
                LOGD("There is no message\n");
                return NULL;
            }

            *read_pos = pos + mesg_len - 1;

            mesgs_size = server_get_all_mesgs_size(mesg_list);
            if (!mesgs_size) {
                LOGD("There is no message\n");
                server_destroy_mesg_list(mesg_list);
                return NULL;
            }
            payload_len =  sizeof(int) + mesgs_size + (mesg_len * (sizeof(long int) + sizeof(int)));
            if (payload_len <= 0) {
                LOGD("payload_len was wrong\n");
                server_destroy_mesg_list(mesg_list);
                return NULL;
            }

            payload = (char*) malloc(payload_len);
            if (!payload) {
                LOGD("Failed to make buf\n");
                server_destroy_mesg_list(mesg_list);
                return NULL;
            }

            mesgs = message_create_array(mesg_len);
            if (!mesgs) {
                LOGD("Failed to create mesg array\n");
                free(payload);
                server_destroy_mesg_list(mesg_list);
                return NULL;
            }

            result = server_copy_mesgs(mesg_list, mesgs, mesg_len);
            server_destroy_mesg_list(mesg_list);
            if (result == FALSE) {
                free(payload);
                LOGD("Failed to copy the mesg list\n");
                return NULL;
            }
            result = convert_mesgs_to_payload(mesgs, payload, mesg_len);
            destroy_mesg(mesgs);
            if (result == FALSE) {
                free(payload);
                LOGD("Failed to convert mesg to payload\n");
                return NULL;
            }

            op_code = RCV_FIRST_OR_LAST_MSG; 
            break;

        case REQ_MAKE_ACCOUNT:
            req_payload = packet_get_payload(req_packet, NULL);
            if (!req_payload) {
                LOGD("Failed to get the payload\n");
                return NULL;
            }
            
            account = server_new_account(req_payload);
            if (!account) {
                LOGD("Failed new account\n");
                return NULL;
            }
            id = account_db_add_account(server->account_db, account);
            if (id < 0) {
                LOGD("Failed to add account\n");
                destroy_account(account);
                result = ERROR_MAKE_ACCOUNT;
                return NULL;
            }

            result = account_set_id(account, id);
            if (result == FALSE) {
                LOGD("Failed to set the id\n");
                return NULL;
            }

            result = SUCCESS_MAKE_ACCOUNT;
            payload_len = RESULT_SIZE;
            payload = server_new_result(op_code, result);
            if (!payload) {
                LOGD("Failed to new result\n");
                return NULL;
            }
            op_code = RES_RESULT;
            break;
        
        case REQ_LOG_IN:
            req_payload = packet_get_payload(req_packet, NULL);
            if (!req_payload) {
                LOGD("Failed to get the payload\n");
                return NULL;
            }
            result = server_login(server, req_payload, client->fd);
            payload_len = RESULT_SIZE;
            payload = server_new_result(op_code, result);
            if (!payload) {
                LOGD("Failed to new result\n");
                return NULL;
            }
            op_code = RES_RESULT;
            break;
        
        case REQ_LOG_OUT:
            req_payload = packet_get_payload(req_packet, NULL);
            if (!req_payload) {
                LOGD("Failed to get the payload\n");
                return NULL;
            }
            result = server_logout(server, req_payload, client->fd);
            payload_len = RESULT_SIZE;
            payload = server_new_result(op_code, result);
            if (!payload) {
                LOGD("Failed to new result\n");
                return NULL;
            }
            op_code = RES_RESULT;
            break;
        
        case REQ_DELETE_ACCOUNT:
            req_payload = packet_get_payload(req_packet, NULL);
            if (!req_payload) {
                LOGD("Failed to get the payload\n");
                return NULL;
            }
            result = server_delete_account(server, req_payload, client->fd);
            payload_len = RESULT_SIZE;
            payload = server_new_result(op_code, result);
            if (!payload) {
                LOGD("Failed to new result\n");
                return NULL;
            }
            op_code = RES_RESULT;
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

    if (!payload || !payload_len) {
        LOGD("There is noting to point payload\n");
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

static BOOLEAN server_copy_payload_len(Stream_Buf *stream_buf, long int *payload_len) {
    char *buf;
    
    buf = stream_buf_get_buf(stream_buf);
    if (!buf) {
        LOGD("There is nothing to point the buf\n");
        return FALSE;
    }

    buf = buf + sizeof(char) + sizeof(short);
    memcpy(payload_len, buf, sizeof(long int));
    return TRUE;
}

static BOOLEAN server_is_checksum_true(short check_sum, char *buf, int packet_len) {
    short comp_check_sum;

    comp_check_sum = 0;
    memcpy(&comp_check_sum, buf + packet_len - 2, sizeof(comp_check_sum));

    if (comp_check_sum != check_sum) {
        LOGD("Not enabled check sum\n");
        return FALSE;
    }

    return TRUE;
}

static BOOLEAN server_send_packet_to_all_clients(Server *server, Client *comp_client, Packet *packet) {
    DList *list;
    Client *client;
    int fd, len;
    BOOLEAN result;
    char *buf;

    len = packet_get_len(packet);
    LOGD("len:%d\n", len);
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
            if (write_n_byte(fd, buf, len) != len) {
                LOGD("Failed to send the Packet to server\n");
                return FALSE;
            }
        }
        list = d_list_next(list);
    }
    return TRUE;
}

static BOOLEAN server_send_packet_to_client(Packet *packet, Client *client, int read_pos) {
    int len, result;
    char *buf;

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

static void destroy_user_data(UserData *user_data) {
    free(user_data);
}

static UserData *new_user_data(Server *server, Packet *packet, int fd, int snd_type) {
    UserData *user_data;

    user_data = (UserData*) malloc(sizeof(UserData));
    if (!user_data) {
        LOGD("Failed to make UserData\n");
        return NULL;
    }
    user_data->id = -1;
    user_data->server = server;
    user_data->packet = packet;
    user_data->fd = fd;
    user_data->snd_type = snd_type;

    return user_data;
}

static BOOLEAN server_interval_send_packet(void *user_data, unsigned int id) {
    UserData *data;
    int result;

    if (!user_data) {
        LOGD("There is nothing to point the user_data\n");
        return FALSE;
    }

    data = (UserData*) user_data;
    if (data->id != id) {
        destroy_packet(data->packet);
        destroy_user_data(user_data);
        return FALSE;
    }

    LOGD("data->id:%d id:%d,\n", data->id, id);
    Client *client = server_find_client_with_fd(data->server, data->fd);
    if (!client) {
        LOGD("There is nothing to point the Client\n");
        destroy_packet(data->packet);
        destroy_user_data(user_data);
        return FALSE;
    }
    
    switch (data->snd_type) {
        case SND_ONE:
            result = server_send_packet_to_client(data->packet, client, client->last_read_pos);
            if (result == FALSE) {
                LOGD("Failed to sedn packet to all client\n");
            }
        break;

        case SND_ALL:
            result = server_send_packet_to_all_clients(data->server, client, data->packet);
            if (result == FALSE) {
                LOGD("Failed to sedn packet to all client\n");
            }
        break;

        default:
            LOGD("SND TYPE is wrong\n");
        break;
        
    } 
    
    destroy_packet(data->packet);
    destroy_user_data(user_data);
    return FALSE;
}

static void server_handle_disconnect_event(Server *server, int fd, unsigned int id) {
    if (id != server->id) {
        server_remove_client(server, id);
    }

    if (close(fd) < 0) {
        LOGD("Failed to close\n");
    }
    LOGD("disconnected:%d\n", fd);
}

static BOOLEAN server_copy_overread_buf(Stream_Buf *c_stream_buf, Stream_Buf *r_stream_buf, int packet_len, int len) {
    char *copy_buf, *buf;

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

    pos = stream_buf_get_position(r_stream_buf);
    if (!pos) {
        LOGD("Stream Buffer Position is zero\n");
        return 0;
    }
    return (pos - packet_len);
}

static Packet *server_new_res_packet(Stream_Buf *stream_buf, short op_code) {
    long int payload_len = stream_buf_get_position(stream_buf);

    if (!payload_len) {
        LOGD("Can't make res_packet\n");
    } 
    int packet_len = HEADER_SIZE + payload_len + TAIL_SIZE;
    LOGD("packet_len:%d\n", packet_len);
    char *packet_buf = (char *) malloc(packet_len);
    memset(packet_buf, 0, packet_len);
    if (!packet_buf) {
        LOGD("Failed to maked the packet_buf\n");
        return NULL;
    }

    char *payload = stream_buf_get_buf(stream_buf);
    char *tmp = packet_buf;
    char sop = SOP;
    char eop = EOP;

    memcpy(tmp, &sop, sizeof(sop));
    tmp += sizeof(sop);

    memcpy(tmp, &op_code, sizeof(op_code));
    tmp += sizeof(op_code);

    LOGD("payload_len:%ld\n", payload_len);
    LOGD("payload_size:%ld\n", sizeof(payload_len));
    memcpy(tmp, &payload_len, sizeof(payload_len));
    tmp += sizeof(payload_len);

    memcpy(tmp, payload, payload_len);
    tmp += payload_len;

    memcpy(tmp, &eop, sizeof(eop));
    tmp += sizeof(eop);

    short check_sum = packet_create_checksum(NULL, packet_buf, packet_len);
    if (check_sum == -1) {
        LOGD("Failed to do check_sum\n");
        free(packet_buf);
        return NULL;
    }
    memcpy(tmp, &check_sum, sizeof(check_sum));

    Packet *res_packet = new_packet(packet_buf);
    if (!res_packet) {
        LOGD("Failed to make the Packet\n");
        return NULL;
    }

    free(packet_buf);
    destroy_stream_buf(stream_buf);
    
    LOGD("Finished server_create_res_packet()\n");
    return res_packet;
}

static void new_login_list_buf(void *data, void *user_data) {
    Online *online = (Online *) data;
    struct _LoginData *loginData = (struct _LoginData *) user_data;

    Stream_Buf *stream_buf = account_db_get_data(loginData->server->account_db, online->id, USER_ID);
    if (!stream_buf) {
        LOGD("Failed to new login list\n");
        return;
    }

    int len = stream_buf_get_position(stream_buf);
    Stream_Buf *id = new_stream_buf(len + LEN_SIZE);
    if (!id) {
        LOGD("Failed to new buf\n");
        destroy_stream_buf(stream_buf);
        return;
    }
    
    memcpy(stream_buf_get_available(id), &len, LEN_SIZE);
    stream_buf_increase_pos(id, LEN_SIZE);
    strncpy(stream_buf_get_available(id), stream_buf_get_buf(stream_buf), len);
    stream_buf_increase_pos(id, len);
    destroy_stream_buf(stream_buf);
    loginData->len += (len + LEN_SIZE);
    loginData->login_list = d_list_append(loginData->login_list, id);
}

static Packet *new_res_login_list_packet(Server *server, short op_code) { 
    struct _LoginData loginData;

    loginData.server = server;
    loginData.login_list = NULL;
    loginData.len = 0;

    d_list_foreach(server->online_list, new_login_list_buf, &loginData);
    int count = d_list_length(loginData.login_list);
    Stream_Buf *stream_buf = NULL;
    BOOLEAN result;
    LOGD("count:%d\n", count);
    if (count) {
        stream_buf = new_stream_buf(loginData.len + LEN_SIZE);
        if (!stream_buf) {
            LOGD("Failed to new login list\n");
            utils_destroy_stream_buf_list(loginData.login_list);
            return NULL;
        }
        memcpy(stream_buf_get_available(stream_buf), &count, LEN_SIZE);
        result = stream_buf_increase_pos(stream_buf, LEN_SIZE);
        if (result == FALSE) {
            LOGD("Failed to new login list\n");
            destroy_stream_buf(stream_buf);
        }
        LOGD("pos:%d\n", stream_buf_get_position(stream_buf));
        result = utils_append_data_to_buf(loginData.login_list, stream_buf);
        utils_destroy_stream_buf_list(loginData.login_list);
    } else {
        stream_buf = new_stream_buf(NO_USER);
        if (!stream_buf) {
            LOGD("Failed to new login list\n");
            return NULL;
        }

        result = stream_buf_increase_pos(stream_buf, NO_USER);
        if (result == FALSE) {
            LOGD("Failed to new login list\n");
            destroy_stream_buf(stream_buf);
        } 
    }

    return server_new_res_packet(stream_buf, op_code);
}

static Packet* new_snd_msg_packet(Server* server, Packet *req_packet, unsigned int *interval, char **user_id, Message **mesg) {
    long int payload_len = packet_get_payload_len(req_packet, NULL);
    if (payload_len <= 0) {
        LOGD("Can't new snd msg_packet\n");
        return NULL;
    }

    char *req_payload = packet_get_payload(req_packet, NULL);
    if (!req_payload) {
        LOGD("Failed to get the payload\n");
        return NULL;
    }
    memcpy(interval, req_payload, INTERVAL_SIZE);

    LOGD("interval:%d\n", *interval);
    payload_len -= INTERVAL_SIZE;
    req_payload += INTERVAL_SIZE;

    int len = 0;
    memcpy(&len, req_payload, LEN_SIZE);
    LOGD("len:%d\n", len);
    req_payload += LEN_SIZE;
    payload_len -= LEN_SIZE;
    if (len == 0) {
        if (req_payload[0]) {
            LOGD("Failed to new_snd_msg_packet\n");
            return NULL;
        } else {
            req_payload += NULL_SIZE;
        } 
    } else {
        *user_id = strndup(req_payload, len);
        LOGD("user_id:%s\n", *user_id);
        req_payload += len;
        payload_len -= len;
    }

    Stream_Buf *payload = new_stream_buf(payload_len);
    if (!payload) {
        LOGD("Failed to make buf for payload\n");
        return NULL;
    }
    memcpy(stream_buf_get_available(payload), req_payload, payload_len);
    BOOLEAN result = stream_buf_increase_pos(payload, payload_len);
    if (result == FALSE) {
        LOGD("Failed to new payload\n");
        destroy_stream_buf(payload);
        return NULL;
    }

    *mesg = convert_payload_to_mesg(stream_buf_get_buf(payload), NULL);
    if (!(*mesg)) {
        LOGD("Failed to convert payload to the Message\n");
        return NULL;
    }
    
    return server_new_res_packet(payload, RCV_MSG);
}

static void server_handle_snd_msg_event(Server *server, Packet *req_packet, Client *client) {
    unsigned int interval = 0;
    char *user_id = NULL;
    int snd_type = 0;
    int fd;
    Message *mesg = NULL;
    
    Packet *res_packet = new_snd_msg_packet(server, req_packet, &interval, &user_id, &mesg); 
    if (!res_packet) {
        LOGD("Failed to new snd_msg_packet\n");
        if (!user_id) {
            free(user_id);
        }
        return;
    }
    if (user_id) {
        int id = account_db_get_id(server->account_db, user_id);
        if (id < 0) {
            LOGD("Failed to get id\n");
            free(user_id);
            destroy_packet(res_packet);
            return;
        }
        LOGD("id:%d\n", id);
        Online *online = (Online *) d_list_find_data(server->online_list, server_match_online, &id);
        if (!online) {
            LOGD("Failed to find id\n");
            free(user_id);
            destroy_packet(res_packet);
            return;
        }
        snd_type = SND_ONE;
        fd = online->fd;
    } else {
        snd_type = SND_ALL;
        fd = client->fd;
    }

    int mesg_id = message_db_add_mesg(server->mesg_db, mesg);
    destroy_mesg(mesg);
    if (mesg_id < 0) {
        LOGD("Failed to add mesg\n");
        if (user_id) {
            free(user_id);
        }
        destroy_packet(res_packet);    
        return;
    }

    UserData *user_data = new_user_data(server, res_packet, fd, snd_type);
    if (!user_data) {
        LOGD("Failed to create the user data\n"); 
        destroy_packet(res_packet);
        return;
    }

    unsigned int id = looper_add_timer(server->looper, interval, server_interval_send_packet, user_data);
    LOGD("id:%d\n", id);
    if (id < 0) {
        LOGD("Faield to add timer\n");
    }
    user_data->id = id;
}

static void server_handle_req_packet(Server *server, Client *client, Packet *req_packet) {
    short op_code;
    int count;
    int read_pos;
    int result;
 
    Packet *res_packet;

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
            res_packet = server_create_res_packet(server, req_packet, client, &read_pos, NULL);
            if (!res_packet) {
                LOGD("Failed to create the res packet\n");
                return;
            }
            result = server_send_packet_to_client(res_packet, client, read_pos);
            if (result == FALSE) {
                LOGD("Failed to send packet to client\n");
            }
            destroy_packet(res_packet);
            break;

        case REQ_FIRST_OR_LAST_MSG:
            if (!count) {
                LOGD("There is no message\n");
                return;
            }
            res_packet = server_create_res_packet(server, req_packet, client, &read_pos, NULL);
            if (!res_packet) {
                LOGD("Failed to create the res packet\n");
                return;
            }
            result = server_send_packet_to_client(res_packet, client, read_pos);
            if (result == FALSE) {
                LOGD("Failed to send packet to client\n");
            }
            destroy_packet(res_packet);
            break;

        case REQ_MAKE_ACCOUNT:
            res_packet = server_create_res_packet(server, req_packet, client, &read_pos, NULL);
            if (!res_packet) {
                LOGD("Failed to create the res packet\n");
                return;
            }
            result = server_send_packet_to_client(res_packet, client, read_pos);
            if (result == FALSE) {
                LOGD("Failed to send packet to client\n");
            }
            destroy_packet(res_packet);
            break;
        
        case REQ_LOG_IN: case REQ_LOG_OUT: case REQ_DELETE_ACCOUNT:
            res_packet = server_create_res_packet(server, req_packet, client, &read_pos, NULL);
            if (!res_packet) {
                LOGD("Failed to create the res packet\n");
                return;
            }
            result = server_send_packet_to_client(res_packet, client, read_pos);
            if (result == FALSE) {
                LOGD("Failed to send packet to client\n");
            }
            destroy_packet(res_packet);
            break;

        case REQ_LOG_IN_LIST:
            LOGD("REQ_LOG_IN_LIST\n");
            res_packet = new_res_login_list_packet(server, RES_LOG_IN_LIST);
            result = server_send_packet_to_client(res_packet, client, read_pos);
            if (result == FALSE) {
                LOGD("Failed to send packet to client\n");
            }
            destroy_packet(res_packet);
            break;

        case SND_MSG:
            LOGD("SND_MSG\n");
            server_handle_snd_msg_event(server, req_packet, client);
            break;
        default:
            LOGD("OPCODE is wrong\n");
            return;
    }
}

static void server_handle_req_event(Server *server, int fd, unsigned int id) {
    char *buf;
    Client *client;
    short check_sum;
    Stream_Buf *stream_buf, *r_stream_buf, *c_stream_buf;
    int n_byte, read_len, packet_len, len, buf_size;
    BOOLEAN result;
    long int payload_len;
    Packet *packet;

    payload_len = 0;
    packet_len = 0;
    buf_size = 0;
    buf = NULL;
    r_stream_buf = NULL;

    LOGD("Start server_handle_req_event()\n");

    client = server_find_client(server, id);
    if (!client) {
        LOGD("There is nothing to point the Client\n");
        return;
    }

    if (client->fd != fd) {
        LOGD("The client fd was wrong\n");
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
        if (buf[packet_len -3] != EOP) {
            LOGD("Packet is wrong\n");
            server_destroy_client_stream_buf_list(client);
            destroy_stream_buf(r_stream_buf);
            free(buf);
            return;
        }

        check_sum = packet_create_checksum(NULL, buf, packet_len);
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

    client_fd = accept(server->fd, NULL, NULL);
    if (client_fd < 0) {
        LOGD("Failed to accept socket\n");
        return -1;
    }

    LOGD("connected:%d\n", client_fd);
    return client_fd;
}

static void server_handle_events(int fd, void *user_data, unsigned int watcher_id, int looper_event) {
    int client_fd;
    int result;
    unsigned int id;

    Server *server;
    server = (Server*) user_data;

    if (looper_event & LOOPER_HUP_EVENT) {
        server_handle_disconnect_event(server, fd, watcher_id);
    } else if (looper_event & LOOPER_IN_EVENT) {
        if (fd == server->fd && watcher_id == server->id) {
            client_fd = server_handle_accept_event(server);
            if (client_fd != -1) {
                id = looper_add_watcher(server->looper, client_fd, server_handle_events, server, LOOPER_IN_EVENT | LOOPER_HUP_EVENT);
                result = server_add_client(server, client_fd, id);
                if (result == -1) {
                    LOGD("Failed to add the Client\n");
                }
            }
        } else {
            server_handle_req_event(server, fd, watcher_id);
        }
    }
}

Server *new_server(Looper *looper) {
    Server *server;
    MessageDB *mesg_db;
    AccountDB *account_db;

    struct sockaddr_un addr;
    int server_fd;
    int id;

    if (!looper) {
        LOGD("Looper is empty\n");
        return NULL;
    }

    if (access(SOCKET_PATH, F_OK) == 0) {
        if (unlink(SOCKET_PATH) < 0) {
            LOGD("Failed to unlink\n");
            return NULL;
        }
    }

    if ((server_fd = socket(AF_UNIX, SOCK_STREAM,0)) == -1) {
        LOGD("socket error\n");
        return NULL;
    }

    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, SOCKET_PATH, sizeof(addr.sun_path)-1);

    if (bind(server_fd, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
        LOGD("bind error\n");
        close(server_fd);
        return NULL;
    }

    if (listen(server_fd, 0) == -1) {
        LOGD("listen error\n");
        close(server_fd);
        return NULL;
    }
    
    mesg_db = message_db_open(MESSAGE_DB_DATA_FORMAT);
    if (!mesg_db) {
        LOGD("Failed to make the MessageDB\n");
        return NULL;
    }
    account_db = account_db_open(ACCOUNT_DATA_FORMAT);
    if (!account_db) {
        LOGD("Failed to make the AccountDB\n");
        return NULL;
    }

    server = (Server*) malloc(sizeof(Server));

    if (!server) {
        LOGD("Failed to make Server\n");
        return NULL;
    }

    server->looper= looper;
    server->fd = server_fd;
    server->client_list = NULL;
    server->online_list = NULL;
    server->mesg_db = mesg_db;
    server->account_db = account_db;
    id = looper_add_watcher(server->looper, server_fd, server_handle_events, server, LOOPER_IN_EVENT | LOOPER_HUP_EVENT);

    if (id < 0) {
        LOGD("Failed to add watcher\n");
        destroy_server(server);
        return NULL;
    }

    server->id = id;
    return server;
}

void destroy_server(Server *server) {
    DList *list;

    if (!server) {
        LOGD("There is no pointer to Server\n");
        return;
    }

    looper_stop(server->looper);

    if (close(server->fd) < 0) {
        LOGD("Failed to close server\n");
        return;
    }

    list = server->client_list;
    d_list_free(list, server_destroy_client);

    looper_remove_all_watchers(server->looper);
    free(server);
}
