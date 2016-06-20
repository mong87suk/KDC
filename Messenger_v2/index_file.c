#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "DBLinkedList.h"
#include "index_file.h"
#include "utils.h"
#include "entry_point.h"
#include "m_boolean.h"

struct _Index_File {
    int fd;
    int last_id;
    int entry_count;
    DList *entry_list;
};

Index_File* new_index_file(char *index_file_name) {
    Index_File *index_file;
    char *homedir, *index_file_path;
    int index_file_namelen, home_pathlen, index_file_pathlen;
    int file_fd;

    homedir = getenv("HOME");
    
    home_pathlen = strlen(homedir);
    index_file_namelen = strlen(index_file_name);

    index_file_pathlen = home_pathlen + index_file_namelen;

    index_file_path = (char*) malloc(index_file_pathlen + 1);
    if (!index_file_path) {
        LOGD("Failed to make the index_file_path\n");   
        return NULL;
    }

    memset(index_file_path, 0, index_file_pathlen + 1);

    strncpy(index_file_path, homedir, home_pathlen);
    strncpy(index_file_path + home_pathlen, index_file_name, index_file_namelen);

    LOGD("index_file_path:%s\n", index_file_path);

    file_fd = open(index_file_path, O_RDWR | O_APPEND | O_CREAT, S_IRUSR | S_IWUSR);

    if (file_fd == -1) {
        LOGD("Failed to make the index file\n");
        return NULL;
    }

    index_file = (Index_File*) malloc(sizeof(Index_File));
    if (!index_file) {
        LOGD("Faield to make the Index File\n");
        return NULL;
    }

    index_file->fd = file_fd;
    index_file->last_id = 0;
    index_file->entry_count = 0;
    index_file->entry_list = NULL;

    return index_file;
}

int get_index_file_end_offset(Index_File *index_file) {
    int end_offset;

    if (!index_file) {
        LOGD("There is nothing to point the index_file\n");
        return FALSE;
    }

    end_offset = lseek(index_file->fd, 0, SEEK_END);

    return end_offset;
}

int set_index_info(Index_File *index_file) {
    int last_id, size, count;
    int n_byte, fd;
    int i;
    Entry_Point *entry_point;

    if (!index_file) {
        LOGD("There is nothing to point the index file\n");
        return FALSE;
    }

    fd = index_file->fd;
    lseek(fd, 0, SEEK_SET);

    size = sizeof(last_id);
    n_byte = read_n_byte(fd, &last_id, size);

    if (n_byte != size) {
        LOGD("Failed to read the last id\n");
        return FALSE;
    }

    index_file->last_id = last_id;

    size = sizeof(count);
    n_byte = read_n_byte(fd, &count, size);

    if (n_byte != size) {
        LOGD("Failed to read the size\n");
        return FALSE;
    }

    size = get_entry_point_size();
    for (i = 0; i < count; i++) {
        entry_point = (char*) malloc(size);
        n_byte = read_n_byte(fd, entry_point, size);

        if (n_byte != size) {
            LOGD("Failed to read the entry point\n");
            return FALSE;
        }
        index_file->entry_list = d_list_append(index_file->entry_list, (Entry*) entry_point);
    }
    return TRUE;
}

void destroy_index_file(Index_File *index_file) {
    if (!index_file) {
        LOGD("There is nothing to point the Index_File\n");
        return;
    }

    d_list_free(index_file->entry_list, destroy_entry_point);
    if (unlink(index_file->fd) < 0) { 
        LOGD("Failed to unlink\n");
        return;
    }

    free(index_file);
    return;
}

int create_entry_point_id(Index_File *index_file) {
    if (!index_file) {
        LOGD("There is nothing to point the Index_File\n");
        return -1;
    }

    return (index_file->last_id + 1);
}

int get_last_id(Index_File *index_file) {
    if (!index_file) {
        LOGD("There is nohting to point the Index_File\n");
        return -1;
    }

    return index_file->last_id;
}

int set_last_id(Index_File *index_file, int last_id) {
    if (!index_file) {
        LOGD("There is nothing to point the Index_File\n");
        return FALSE;
    }

    index_file->last_id = last_id;
    return TRUE;
}
