#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "data_file.h"
#include "m_boolean.h"
#include "utils.h"
#include "stream_buf.h"

struct _DataFile {
    int fd;
    char *path;
};

DataFile *data_file_open(char *name) {
    DataFile *data_file;
    char *path;
    int fd;

    if (!name || strlen(name) < 0) {
        LOGD("Can't open\n");
        return NULL;
    }

    path = utils_create_path(name, DATAFILE);
    if (!path) {
        LOGD("Failed to make path\n");
        return NULL;
    }

    fd = utils_open(path);
    if (fd < 0) {
        LOGD("Failed to make open the index file\n");
        return NULL;
    }

    data_file = (DataFile*) malloc(sizeof(DataFile));
    if (!data_file) {
        LOGD("Faield to make the Index File\n");
        return NULL;
    }

    data_file->fd = fd;
    data_file->path = path;
    return data_file;
}

void data_file_close(DataFile *data_file) {
    if (!data_file) {
        LOGD("There is nothing to point the Data_File\n");
        return;
    }

    if (close(data_file->fd) < 0) {
        LOGD("Faield to close\n");
        return;
    }
    
    free(data_file->path);
    free(data_file);
    return;
}

void data_file_delete_all(DataFile *data_file) {
    int fd;

    if (!data_file) {
        LOGD("There is nothing to point the Data_File\n");
        return;
    }

    if (close(data_file->fd) < 0) {
        LOGD("Faield to close\n");
        return;
    }

    if (unlink(data_file->path) < 0) {
        LOGD("Failed to unlink\n");
        return;
    }

    fd = open(data_file->path, O_RDWR | O_APPEND | O_CREAT, S_IRUSR | S_IWUSR);
    if (fd < 0) {
        LOGD("Failed to open the file\n");
        return;
    }

    data_file->fd = fd;

    return;
}
 
int data_file_get_offset(DataFile *data_file) {
    int offset;
    if (!data_file) {
        LOGD("There is nothing to point the Data_File\n");
        return -1;
    }

    offset = lseek(data_file->fd, 0, SEEK_END);
    if (offset < 0) {
        LOGD("Failed to get data offset\n");
        return -1;
    }

    return offset;
}

int data_file_get_fd(DataFile *data_file) {
    if (!data_file) {
        LOGD("There is nothing to point the Data File\n");
        return -1;
    }

    return data_file->fd;
}

int data_file_write_entry(DataFile *data_file, int id, Stream_Buf *entry) {
    int n_byte;
    int len;
    char *buf;

    if (!data_file) {
        LOGD("There is nothing to point the DataFile\n");
        return FALSE;
    }

    n_byte = write_n_byte(data_file->fd, &id, sizeof(id));
    if (n_byte != sizeof(id)) {
        LOGD("Failed to write n byte\n");
        return FALSE;
    }

    len = stream_buf_get_position(entry);
    if (len < 0) {
        LOGD("Failed to get position\n");
        return FALSE;
    }

    buf = stream_buf_get_buf(entry);

    if (!buf) {
        LOGD("Failed to get the buf\n");
        return FALSE;
    }

    n_byte = write_n_byte(data_file->fd, stream_buf_get_buf(entry), len);
    if (n_byte != len) {
        LOGD("Failed to write n byte\n");
        return FALSE;
    }

    return TRUE;
}
