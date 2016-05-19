#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <poll.h>
#include <sys/un.h>
#include <string.h>
#include <sys/types.h>

#include "server.h"
#include "looper.h"
#include "DBLinkedList.h"
#include "mesg_file_db.h"
#include "mesg_file.h"
#include "socket.h"

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
    int read_state;
    DList *stream_buf_list;
};

static void destroy_client(void *client) {
    Client *remove_client;

    if (!client) {
        printf("There is nothing to pointer the Client\n");
        return;
    }
    remove_client = (Client*) client;

    if (close(remove_client->fd) < 0) {
        printf("Falied to close client fd\n");
    }
    free(remove_client);
}

static int match_client(void *client, void *fd) {

    if (((Client*) client)->fd == *((int*) fd)) {
        return 1;
    } else {
        return 0;
    }
}

static Client* find_client(Server *server, int fd) {
    Client *client = NULL;
    client = (Client*) d_list_find_data(server->client_list, match_client, &fd);

    if (client == NULL) {
        printf("Can't find Client\n");
        return NULL;
    }
    return client;
}

static void add_client(Server *server, int fd) {
    Client *client;
    DList *list;

    list = server->client_list;
    client = (Client*) malloc(sizeof(Client));

    if (!client) {
        printf("Failed to make Client\n");
        return;
    }

    client->fd = fd;
    client->file_offset = 0;
    client->read_state = 0;

    server->client_list = d_list_append(list, client);
}

static void remove_client(Server *server, int fd) {
    if (!server && !server->client_list) {
        printf("Can't remove Client\n");
        return;
    }

    server->client_list = d_list_remove_with_user_data(server->client_list, &fd, match_client, destroy_client);
}

static void read_packet(int fd) {
    char buf[MAX_BUF_LEN];
    int n_byte;

    while ((n_byte = read(fd, buf, MAX_BUF_LEN))) {
       if (n_byte == 0) {
           printf("Finished read packet\n");
       }
    }
}

static void handle_disconnect_event(Server *server, int fd) {
    if (fd != server->fd) {
        remove_client(server, fd);
        remove_watcher(server->looper, fd);
    }
    printf("disconnected:%d\n", fd);
}

static void handle_req_event(Server *server, int fd) {
    read_packet(fd);
}

static int handle_accept_event(Server *server) {
    int client_fd;
    int state;

    client_fd = accept(server->fd, NULL, NULL);
    if (client_fd < 0) {
        printf("Failed to accept socket\n");
        return -1;
    }

    add_client(server, client_fd);
    printf("connected:%d\n", client_fd);
    return client_fd;
}

static void handle_events(int fd, void *user_data, int revents) {
    int client_fd;
    Server *server;
    server = (Server*) user_data;

    if (!server && (fd < 0)) {
        printf("Can't handle events\n");
        return;
    }

    if (revents & POLLHUP) {
         handle_disconnect_event(server, fd);
    } else if (revents & POLLIN) {
        if (fd == server->fd) {
            client_fd = handle_accept_event(server);
            if (client_fd != -1) {
                add_watcher(server->looper, client_fd, handle_events, server, POLLIN);
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
        printf("Looper is empty\n");
        return NULL;
    }

    server = (Server*) malloc(sizeof(Server));

    if (!server) {
        printf("Failed to make Server\n");
        return NULL;
    }

    if (unlink(SOCKET_PATH) < 0) {
        printf("Failed to unlink\n");
        return NULL;
    }

    if ((server_fd = socket(AF_UNIX, SOCK_STREAM,0)) == -1) {
        perror("socket error");
        return NULL;
    }

    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, SOCKET_PATH, sizeof(addr.sun_path)-1);

    if (bind(server_fd, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
        perror("bind error");
        close(server_fd);
        return NULL;
    }

    if (listen(server_fd, 0) == -1) {
        perror("listen error");
        close(server_fd);
        return NULL;
    }

    mesg_file = new_mesg_file();
    mesg_file_db = new_mesg_file_db();

    if (!mesg_file && !mesg_file_db) {
        printf("There is no a pointer to Mesg_File or Mesg_File_DB\n");
        return NULL;
    }

    server->looper = looper;
    server->fd = server_fd;
    server->client_list = NULL;
    server->mesg_file = mesg_file;
    server->mesg_file_db = mesg_file_db;

    add_watcher(server->looper, server_fd, handle_events, server, POLLIN);

    return server;
}

void destroy_server(Server *server) {
    DList *list;

    if (!server) {
        printf("There is no pointer to Server\n");
        return;
    }

    stop(server->looper);

    if (close(server->fd) < 0) {
        printf("Failed to close server\n");
        return;
    }

    list = server->client_list;
    d_list_free(list, destroy_client);

    remove_all_watchers(server->looper);
    destroy_mesg_file_db(server->mesg_file_db);
    destroy_mesg_file(server->mesg_file);

    free(server);
}
