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

struct _Server {
    int fd;
    DList *client_list;
    Looper *looper;
    Mesg_File *mesg_file;
    Mesg_File_DB *mesg_file_db;
};

struct _Client {
    int fd;
    int file_offset;
    CLIENT_READ_REQ_STATE read_state;
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
    client->read_state = CLIENT_READ_REQ_READY;
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
    client->read_state = CLIENT_READ_REQ_READY;
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

static APPEND_DATA_RESULT append_data_to_buf(DList *stream_buf_list, char *buf) {
    if (!stream_buf_list) {
        LOGD("There is no a pointer to Stream_Buf\n");
        return APPEND_DATA_FAILURE;
    }
    d_list_foreach(stream_buf_list, append_data, buf);
    return APPEND_DATA_SUCCESS;
}

static short copy_payload_len(char *buf, long int *payload_len) {
    int i = 0;
    if (!buf) {
        LOGD("There is nothing to point the buf\n");
        return FALSE;
    }
    buf = buf + sizeof(char) + sizeof(short);

    memcpy(payload_len, buf, sizeof(long int));
    LOGD("payload_len: %ld\n", *payload_len);
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
    short result;
    Stream_Buf *stream_buf;
    int n_byte, read_len, packet_len, position;
    long int payload_len;

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

    if (client->read_state == CLIENT_READ_REQ_READY) {
        client->read_state = CLIENT_READ_REQ_START;
        do {
            if (client->stream_buf_list == NULL) {
                stream_buf = new_stream_buf(HEADER_SIZE);
                printf("%d\n", HEADER_SIZE);
                client->stream_buf_list = d_list_append(client->stream_buf_list, stream_buf);
            }
            n_byte = read(fd, get_available_buf(stream_buf), get_available_size(stream_buf));
            if (n_byte < 0) {
                LOGD("Failed to read\n");
                destroy_client_stream_buf_list(client);
                return;
            }

            result = set_position(stream_buf, n_byte);
            if (result == FALSE) {
                LOGD("Failed to set the position\n");
                destroy_client_stream_buf_list(client);
                return;
            }
            position = get_position(stream_buf);
        } while (position != HEADER_SIZE);
        
        buf = get_buf(stream_buf);           
        if (buf[0] != SOP) {
            LOGD("Failed to get binary\n");
            d_list_free(client->stream_buf_list, destroy_stream_buf_list);
            client->stream_buf_list = NULL; 
            client->read_state = CLIENT_READ_REQ_READY;
            return;
        }
        result = copy_payload_len(buf, &payload_len);
        if (result == FALSE) {
            LOGD("Failed to copy payload\n");
            d_list_free(client->stream_buf_list, destroy_stream_buf_list);
            client->stream_buf_list = NULL;
            client->read_state = CLIENT_READ_REQ_READY;
            return;
        }
        client->packet_len = HEADER_SIZE + payload_len + TAIL_SIZE;
    }

    if (client->read_state == CLIENT_READ_REQ_START) {

        stream_buf = new_stream_buf(MAX_BUF_LEN);
        client->stream_buf_list = d_list_append(client->stream_buf_list, stream_buf);
        packet_len = client->packet_len;  
        read_len = get_buffer_size(client->stream_buf_list);

        do {
            position = get_position(stream_buf);
            if (position >= MAX_BUF_LEN) {
                stream_buf = new_stream_buf(MAX_BUF_LEN);
                client->stream_buf_list = d_list_append(client->stream_buf_list, stream_buf);
            }

            n_byte = read(fd, get_available_buf(stream_buf), get_available_size(stream_buf));
            if (n_byte < 0) {
                LOGD("Failed to read\n");
                d_list_free(client->stream_buf_list, destroy_stream_buf_list);
                client->read_state = CLIENT_READ_REQ_READY;
                client->stream_buf_list = NULL;
                return;
            }
            read_len += n_byte;
            result = set_position(stream_buf, n_byte);
            if (result == FALSE) {
                LOGD("Failed to set the position\n");
                d_list_free(client->stream_buf_list, destroy_stream_buf_list);
                client->read_state = CLIENT_READ_REQ_READY;
                client->stream_buf_list = NULL;
                return;
            }
        } while ((read_len - packet_len));
    }

    read_len = get_buffer_size(client->stream_buf_list);
    LOGD("buf_len:%d\n", read_len);
    buf = (char*) malloc(read_len);

    if (!buf) {
        LOGD("Failed to make buf\n");
        return;
    }
    memset(buf, 0, read_len);
    result = append_data_to_buf(client->stream_buf_list, buf);

    if (result == APPEND_DATA_FAILURE) {
        LOGD("Failed to append input data to buf\n");
        return;
    }

    d_list_free(client->stream_buf_list, destroy_stream_buf_list);
    client->stream_buf_list = NULL;
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
