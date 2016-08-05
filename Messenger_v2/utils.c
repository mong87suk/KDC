#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <kdc/DBLinkedList.h>

#include "message.h"
#include "utils.h"
#include "stream_buf.h"
#include "m_boolean.h"
#include "database.h"
#include "data_file.h"
#include "entry_point.h"

static void utils_append_data(void *data, void *user_data) {
    char *dest, *src;
    int copy_n;
    Stream_Buf *data_stream_buf;
    Stream_Buf *user_data_stream_buf;

    data_stream_buf = (Stream_Buf*) data;
    user_data_stream_buf = (Stream_Buf*) user_data;

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

static char *utils_create_full_name(char *name, char *file_name) {
    int namelen, file_namelen, full_namelen;
    char *full_name;

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

void utils_print_mesg(Message *mesg) {
    long int time;
    int str_len, i;
    char *str, *pos;

    if (!mesg) {
        LOGD("Can't print the Message\n");
        return;
    }   

    LOGD("\nPrint Message\n");
    time = message_get_time(mesg);
    pos = (char*) &time;
    for (i = 0; i < sizeof(time); i++) {
        printf("0x%02X ", (unsigned char)*(pos + i));
    }   

    str_len = message_get_str_len(mesg);
    pos = (char*) &str_len;
    for (i = 0; i < sizeof(str_len); i++) {
        printf("0x%02X ", (unsigned char)*(pos + i));
    }   

    str = message_get_str(mesg);
    pos = str;
    for (i = 0; i < str_len; i++) {
        printf("%c ", (unsigned char) *(pos + i));
    }   
    printf("\n\n");
}

int utils_read_n_byte(int fd, void *buf, int size) {
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

    if (fd < 0) {
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

int utils_get_colum_count(int field_mask) {
    int state;
    int i = 0;

    if (field_mask < 0) {
        LOGD("filed_mask was wrong\n");
        return -1;
    }

    while (1) {
        state = field_mask & (FIELD_TYPE_FLAG << (i * FIELD_SIZE));
        if (!state) {
            break;
        }
        i++;
    }
    return i;
}

BOOLEAN utils_append_data_to_buf(DList *stream_buf_list, Stream_Buf *stream_buf) {
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

char *utils_create_path(char *name, char *file_name) {
    char *homedir, *full_name, *path;
    int home_pathlen, pathlen, full_namelen;

    if (!name || !file_name) {
        LOGD("There is nothing to point name or file_name\n");
        return NULL;
    }

    full_name = utils_create_full_name(name, file_name);
    if (!full_name) {
        LOGD("Failed to create full name\n");
        return NULL;
    }

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
    return path;
}

Stream_Buf *utils_get_data(DataBase *database, EntryPoint *entry_point, int column_index, int *field_type) {
    int field_mask, count, column, index;
    int fd, offset, n_byte;
    int id, entry_id;
    int len, size;
    int i;
    char *buf;
    
    BOOLEAN result;
    Stream_Buf *stream_buf;

    if (!database || !entry_point || column_index < 0) {
        LOGD("Can't get the data\n");
        return NULL;
    }

    field_mask = database_get_field_mask(database);
    
    DataFile *data_file = database_get_datafile(database);
    if (!data_file) {
        return NULL;
    }

    fd = data_file_get_fd(data_file);
    if (fd < 0) {
        LOGD("Failed to get the fd\n");
        return NULL;
    }

    offset = entry_point_get_offset(entry_point);
    if (offset < 0) {
        LOGD("Failed to get the offset\n");
        return NULL; 
    }

    if (offset != lseek(fd, offset, SEEK_SET)) {
        LOGD("Failed to set offset\n");
        return NULL;
    }

    n_byte = utils_read_n_byte(fd, &id, sizeof(id));
    if (n_byte != sizeof(id)) {
        LOGD("Failed to read id\n");
        return NULL;
    }

    entry_id = entry_point_get_id(entry_point);
    if (entry_id < 0) {
        LOGD("Failed to get the id\n");
        return NULL;
    }

    if (id != entry_id) {
        LOGD("the entry id was wrong\n");
        return NULL;
    }

    count = utils_get_colum_count(field_mask);
    if (count <= 0) {
        LOGD("Failed to get colum count\n");
        return NULL;
    }
    count -= 1;

    stream_buf = NULL;
    index = 0;
    for (i = count; i >= 0; i--) {
        column = (field_mask >> (FIELD_SIZE * i)) & FIELD_TYPE_FLAG;
        switch (column) {
            case INTEGER_FIELD:
                if (column_index == index) {
                    stream_buf = new_stream_buf(FIELD_I_SIZE);
                    if (!stream_buf) {
                        LOGD("Failed to make the StreamBuf\n");
                        return NULL;
                    }
                    n_byte = utils_read_n_byte(fd, stream_buf_get_available(stream_buf), stream_buf_get_available_size(stream_buf));
                    if (n_byte != FIELD_I_SIZE) {
                        LOGD("Failed to write n byte\n");
                        return NULL;
                    }
                    result = stream_buf_increase_pos(stream_buf, n_byte);
                    if (result == FALSE) {
                        destroy_stream_buf(stream_buf);
                        stream_buf = NULL;
                    }
                } else {
                    if (lseek(fd, FIELD_I_SIZE, SEEK_CUR) < 0) {
                        LOGD("Failed to locate the offset\n");
                        return NULL;
                    }
                }
                break;

            case STRING_FIELD:
                n_byte = utils_read_n_byte(fd, &len, sizeof(len));
                if (n_byte != sizeof(len)) {
                    LOGD("Failed to write n byte\n");
                    return NULL;
                }

                if (len < 0) {
                    LOGD("len value was wrong\n");
                    return NULL;
                }
                
                if (column_index == index) {
                    size = len + 1 + sizeof(len);
                    stream_buf = new_stream_buf(size);
                    if (!stream_buf) {
                        LOGD("Failed to make the StreamBuf\n");
                        return NULL;
                    }

                    buf = stream_buf_get_buf(stream_buf);
                    if (!buf) {
                        LOGD("Failed to get the buf\n");
                        return NULL;
                    }
                    memset(buf, 0, size);
                    memcpy(buf, &len, sizeof(len));
                    result = stream_buf_increase_pos(stream_buf, LEN_SIZE);
                    if (result == FALSE) {
                        destroy_stream_buf(stream_buf);
                        stream_buf = NULL;
                    }

                    n_byte = utils_read_n_byte(fd, stream_buf_get_available(stream_buf), len);
                    if (n_byte != len) {
                        LOGD("Failed to write n byte\n");
                        return NULL;
                    }
                    result = stream_buf_increase_pos(stream_buf, len);
                    if (result == FALSE) {
                        destroy_stream_buf(stream_buf);
                        stream_buf = NULL;
                    }

                } else {
                    if (lseek(fd, len, SEEK_CUR) < 0) {
                        LOGD("Failed to locate the offset\n");
                        return NULL;
                    }
                }
                break;

            case 0:
                break;

            default:
                LOGD("field mask was wrong\n");
                return NULL;
        }

        if (column_index == index) {
            *field_type = column;
            break;
        }

        index++;
    }

    return stream_buf;
}
