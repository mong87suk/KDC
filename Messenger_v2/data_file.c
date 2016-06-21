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

DataFile *new_data_file(char *file_name) {
    DataFile *data_file;
    char *homedir, *data_file_path;
    int data_file_namelen, home_pathlen, data_file_pathlen;
    int file_fd;

    homedir = getenv("HOME");

    home_pathlen = strlen(homedir);
    data_file_namelen = strlen(file_name) + strlen(DATAFILE) + 2;

    data_file_pathlen = home_pathlen + data_file_namelen;

    data_file_path = (char*) malloc(data_file_pathlen + 1);
    if (!data_file_path) {
        LOGD("Failed to make the data_file_path\n");
        return NULL;
    }

    memset(data_file_path, 0, data_file_pathlen + 1);

    strncpy(data_file_path, homedir, home_pathlen);
    data_file_path[home_pathlen] = '/';

    strncpy(data_file_path + home_pathlen + 1, file_name, strlen(file_name));
    data_file_path[home_pathlen + 1 + strlen(file_name)] = '_';

    strncpy(data_file_path + home_pathlen + 1 + strlen(file_name) + 1, DATAFILE, strlen(DATAFILE));
    LOGD("index_file_path:%s\n", data_file_path);

    file_fd = open(data_file_path, O_RDWR | O_APPEND | O_CREAT, S_IRUSR | S_IWUSR);

    if (file_fd == -1) {
        LOGD("Failed to make the index file\n");
        return NULL;
    }

    data_file = (DataFile*) malloc(sizeof(DataFile));
    if (!data_file) {
        LOGD("Faield to make the Index File\n");
        return NULL;
    }

    data_file->fd = file_fd;
    data_file->path = data_file_path;
    return data_file;
}

void destroy_data_file(DataFile *data_file) {
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

int get_data_file_fd(DataFile *data_file) {
    if (!data_file) {
        LOGD("There is nothing to point the Data File\n");
        return -1;
    }

    return data_file->fd;
}
