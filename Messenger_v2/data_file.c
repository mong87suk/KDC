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
        LOGD("Faield to unlink\n");
        return;
    }
    
    free(data_file->path);
    free(data_file);
    return;
}

int get_data_file_offset(DataFile *data_file) {
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
