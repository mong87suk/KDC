#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "entry_point.h"
#include "message.h"
#include "utils.h"
#include "stream_buf.h"
#include "DBLinkedList.h"
#include "m_boolean.h"

static void utils_append_data(void *data, void *user_data) {
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

static char* utils_create_full_name(char *name, char *file_name) {
    int namelen, file_namelen, full_namelen;
    char *full_name;

    if (!name || !file_name) {
        LOGD("There is nothing to point name or file_name\n");
        return NULL;
    }

    namelen = strlen(name);
    if (namelen == 0) {
        LOGD("Failed to get the name length\n");
        return NULL;
    }

    file_namelen = strlen(file_name);
    if (namelen == 0) {
        LOGD("Failed to get the name length\n");
        return NULL;
    }

    full_namelen = namelen + file_namelen + 1;

    full_name = (char*) malloc(full_namelen);
    if (!full_name) {
        LOGD("Failed to get buf\n");
        return NULL;
    }

    memset(full_name, 0, full_namelen);
    memcpy(full_name, name, namelen);
    full_name[namelen] = '_';
    memcpy(full_name + namelen + 1, file_name, file_namelen);

    return full_name;
}

static void utils_free_stream_buf(void *data) {
    Stream_Buf *stream_buf;

    stream_buf = (Stream_Buf*) data;

    if (!stream_buf) {
        LOGD("There is nothing to remove the Stream_Buf\n");
        return;
    }
    destroy_stream_buf(stream_buf);
}

void print_mesg(Message *mesg) {
    long int time;
    int str_len, i;
    char *str, *pos;

    if (!mesg) {
        LOGD("Can't print the Message\n");
        return;
    }   

    LOGD("\nPrint Message\n");
    time = get_time(mesg);
    pos = (char*) &time;
    for (i = 0; i < sizeof(time); i++) {
        printf("0x%02X ", (unsigned char)*(pos + i));
    }   

    str_len = get_str_len(mesg);
    pos = (char*) &str_len;
    for (i = 0; i < sizeof(str_len); i++) {
        printf("0x%02X ", (unsigned char)*(pos + i));
    }   

    str = get_str(mesg);
    pos = str;
    for (i = 0; i < str_len; i++) {
        printf("%c ", (unsigned char) *(pos + i));
    }   
    printf("\n\n");
}

int read_n_byte(int fd, void *buf, int size) {
    int n, tmp;

    tmp = size;

    if (!buf) {
        LOGD("Can't read n byte\n");
    }

    if (size < 0 || fd < 0) {
        LOGD("Can't read n byte\n");
    }

    while (tmp > 0) {
        n = read(fd, buf, size);
        if (n < 0) {
            LOGD("Faield to read buf\n");
            return -1;
        }
        tmp -= n;
    }

    return size;
}

int write_n_byte(int fd, void *buf, int size) {
    int n, tmp;

    tmp = size;

    if (!buf) {
        LOGD("Can't read n byte\n");
    }

    if (size < 0 || fd < 0) {
        LOGD("Can't read n byte\n");
    }

    while (tmp > 0) {
        n = write(fd, buf, size);
        if (n < 0) {
            LOGD("Failed to write buf\n");
            return -1;
        }
        tmp -= n;
    }

    return size;
}

int utils_get_count_to_move_flag(int field_mask) {
    int state;
    int i = 0;

    while (1) {
        state = field_mask & (FIELD_TYPE_FLAG << (i * FIELD_SIZE));
        if (!state) {
            break;
        }
        i++;
    }
    LOGD("count to move flag:%d\n", i);
    return (i - 1);
}

int utils_append_data_to_buf(DList *stream_buf_list, Stream_Buf *stream_buf) {
    if (!stream_buf_list || !stream_buf) {
        LOGD("Can't append data to Stream_Buf\n");
        return FALSE;
    }   
    d_list_foreach(stream_buf_list, utils_append_data, stream_buf);
    return TRUE;
}

void utils_destroy_stream_buf_list(DList *stream_buf_list) {
    if (!stream_buf_list) {
        LOGD("There is nothing to point the Stream Buf\n");
        return;
    }
    d_list_free(stream_buf_list, utils_free_stream_buf);
}

char* utils_create_path(char *name, char *file_name) {
    char *homedir, *full_name, *path;
    int home_pathlen, pathlen, full_namelen;

    if (!name || !file_name) {
        LOGD("There is nothing to point name or file_name\n");
        return NULL;
    }

    LOGD("utils_create_full_name\n");
    full_name = utils_create_full_name(name, file_name);
    if (!full_name) {
        LOGD("Failed to create full name\n");
        return NULL;
    }

    LOGD("full_name:%s\n", full_name);
    
    homedir = getenv("HOME");
    if (!homedir) {
        LOGD("There is no 'HOME' enviorment\n");
    }

    home_pathlen = strlen(homedir);
    full_namelen = strlen(full_name);

    pathlen = home_pathlen + full_namelen + 1;

    path = (char*) malloc(pathlen + 1);
    if (!path) {
        LOGD("Failed to make the index_file_path\n");
        return NULL;
    }

    memset(path, 0, pathlen + 1);

    strncpy(path, homedir, home_pathlen);
    path[home_pathlen] = '/';

    strncpy(path + home_pathlen + 1, full_name, full_namelen);
    LOGD("path:%s\n", path);
    return path;
}

int utils_open(char *path) {
    int fd;

    if (!path) {
        LOGD("There is nothing to point the path\n");
        return -1;
    }

    fd = open(path, O_RDWR | O_APPEND | O_CREAT, S_IRUSR | S_IWUSR);
    if (fd < 0) {
        LOGD("Failed to open the file\n");
        return -1;
    }

    return fd;
}
