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

struct _IndexFile {
    int field_mask;
    int fd;
    int last_id;
    int entry_count;
    char *path;
    DList *entry_list;
};

int match_entry_point(void *data1, void *data2) {
    EntryPoint *entry_point;
    int entry_id;
    int id;

    if (!data1 || !data2) {
        LOGD("Can't to match entry point\n");
        return FALSE;
    }

    entry_point = (EntryPoint*) data1;
    entry_id = *((int*) data2);

    id = get_entry_point_id(entry_point);

    if (id == entry_id) {
        return TRUE;
    } else {
        return FALSE;
    }
}

static void write_entry_point(void *data, void *user_data) {
    EntryPoint *entry_point;
    int fd;
    int size, n_byte;

    if (!data || !user_data) {
        LOGD("Can't write the EntryPoint\n");
        return;
    }

    entry_point = (EntryPoint*) data;
    fd = *((int*)user_data);
    size = get_entry_point_size();

    n_byte = write_n_byte(fd, entry_point, size);
    if (n_byte != size) {
        LOGD("Failed to write EntryPoint\n");
        return;
    }
}


void free_entry_point(void *data) {
    if (!data) {
        LOGD("There is nothing to point the Entry_Point\n");
        return;
    }

    destroy_entry_point((EntryPoint*) data);
}

IndexFile* new_index_file(char *file_name, int field_mask) {
    IndexFile *index_file;
    char *homedir, *index_file_path;
    int index_file_namelen, home_pathlen, index_file_pathlen;
    int file_fd;

    homedir = getenv("HOME");

    home_pathlen = strlen(homedir);
    index_file_namelen = strlen(file_name) + strlen(INDEXFILE) + 2;

    index_file_pathlen = home_pathlen + index_file_namelen;

    index_file_path = (char*) malloc(index_file_pathlen + 1);
    if (!index_file_path) {
        LOGD("Failed to make the index_file_path\n");
        return NULL;
    }

    memset(index_file_path, 0, index_file_pathlen + 1);

    strncpy(index_file_path, homedir, home_pathlen);
    index_file_path[home_pathlen] = '/';

    strncpy(index_file_path + home_pathlen + 1, file_name, strlen(file_name));
    index_file_path[home_pathlen + 1 + strlen(file_name)] = '_';

    strncpy(index_file_path + home_pathlen + 1 + strlen(file_name) + 1, INDEXFILE, strlen(INDEXFILE));
    LOGD("index_file_path:%s\n", index_file_path);

    file_fd = open(index_file_path, O_RDWR | O_APPEND | O_CREAT, S_IRUSR | S_IWUSR);

    if (file_fd < 0) {
        LOGD("Failed to make the index file\n");
        return NULL;
    }

    index_file = (IndexFile*) malloc(sizeof(IndexFile));
    if (!index_file) {
        LOGD("Faield to make the Index File\n");
        return NULL;
    }

    index_file->fd = file_fd;
    index_file->last_id = 0;
    index_file->entry_count = 0;
    index_file->entry_list = NULL;
    index_file->path = index_file_path;
    index_file->field_mask = field_mask;

    return index_file;
}

int get_index_file_end_offset(IndexFile *index_file) {
    int end_offset;

    if (!index_file) {
        LOGD("There is nothing to point the index_file\n");
        return FALSE;
    }

    end_offset = lseek(index_file->fd, 0, SEEK_END);

    return end_offset;
}

int set_index_info(IndexFile *index_file) {
    int field_mask, last_id, size, entry_count;
    int n_byte, fd;
    int offset;
    int i;
    EntryPoint *entry_point;

    if (!index_file) {
        LOGD("There is nothing to point the index file\n");
        return FALSE;
    }
    offset = 0;
    field_mask = 0;
    fd = index_file->fd;

    offset = lseek(fd, 0, SEEK_SET);
    if (offset) {
        LOGD("Failed to set offset\n");
        return FALSE;
    }

    size = sizeof(int);
    n_byte = read_n_byte(fd, &field_mask, size);
    if (n_byte != size) {
        LOGD("Failed to read the last id\n");
        return FALSE;
    }

    if (index_file->field_mask != field_mask) {
        LOGD("field_mask was updated\n");
        index_file->field_mask = field_mask;
    }

    n_byte = read_n_byte(fd, &last_id, size);
    if (n_byte != size) {
        LOGD("Failed to read the last id\n");
        return FALSE;
    }
    index_file->last_id = last_id;

    n_byte = read_n_byte(fd, &entry_count, size);
    if (n_byte != size) {
        LOGD("Failed to read the size\n");
        return FALSE;
    }
    index_file->entry_count = entry_count;

    size = get_entry_point_size();
    for (i = 0; i < entry_count; i++) {
        entry_point = (EntryPoint*) malloc(size);
        n_byte = read_n_byte(fd, entry_point, size);

        if (n_byte != size) {
            LOGD("Failed to read the entry point\n");
            return FALSE;
        }
        index_file->entry_list = d_list_append(index_file->entry_list, entry_point);
    }
    return TRUE;
}

void destroy_index_file(IndexFile *index_file) {
    if (!index_file) {
        LOGD("There is nothing to point the Index_File\n");
        return;
    }

    d_list_free(index_file->entry_list, free_entry_point);
    if (close(index_file->fd) < 0) {
        LOGD("Failed to unlink\n");
        return;
    }

    free(index_file->path);
    free(index_file);
    return;
}

int create_entry_point_id(IndexFile *index_file) {
    if (!index_file) {
        LOGD("There is nothing to point the Index_File\n");
        return -1;
    }

    return (index_file->last_id + 1);
}

int get_last_id(IndexFile *index_file) {
    if (!index_file) {
        LOGD("There is nohting to point the Index_File\n");
        return -1;
    }

    return index_file->last_id;
}

int set_last_id(IndexFile *index_file, int last_id) {
    if (!index_file) {
        LOGD("There is nothing to point the Index_File\n");
        return FALSE;
    }

    index_file->last_id = last_id;
    return TRUE;
}

int add_entry_point(IndexFile *index_file, EntryPoint *entry_point) {
    if (!index_file || !entry_point) {
        LOGD("Can't add the EntryPoint\n");
        return FALSE;
    }

    index_file->entry_list = d_list_append(index_file->entry_list, entry_point);
    index_file->entry_count = d_list_length(index_file->entry_list);
    return TRUE;
}

int update_index_file(IndexFile *index_file) {
    int fd, last_id, entry_count, field_mask;
    int n_byte;

    if (!index_file) {
        LOGD("There is nothing to point the IndexFile\n");
        return FALSE;
    }

    if (unlink(index_file->path) < 0) {
        LOGD("Failed to unlink\n");
        return FALSE;
    }

    fd = open(index_file->path, O_RDWR | O_APPEND | O_CREAT, S_IRUSR | S_IWUSR);

    if (fd < 0) {
        LOGD("Failed to open index file\n");
        return FALSE;
    }
    index_file->fd = fd;

    field_mask = index_file->field_mask;
    fd = index_file->fd;
    last_id = index_file->last_id;
    entry_count = index_file->entry_count;

    n_byte = write_n_byte(fd, &field_mask, sizeof(field_mask));
    if (n_byte != sizeof(field_mask)) {
        LOGD("Failed to write the field_mask\n");
        return FALSE;
    }

    n_byte = write_n_byte(fd, &last_id, sizeof(last_id));
    if (n_byte != sizeof(last_id)) {
        LOGD("Failed to write the last_id\n");
        return FALSE;
    }

    n_byte = write_n_byte(fd, &entry_count, sizeof(entry_count));
    if (n_byte != sizeof(entry_count)) {
        LOGD("Failed to write the entry_count\n");
        return FALSE;
    }

    d_list_foreach(index_file->entry_list, write_entry_point, &fd);

    return TRUE;
}

int get_count(IndexFile *index_file) {
    if (!index_file) {
        LOGD("There is nothing to point the IndexFile\n");
        return -1;
    }

    return index_file->entry_count;
}

EntryPoint* find_entry_point(IndexFile *index_file, int entry_point_id) {
    EntryPoint *entry_point;

    if (!index_file) {
        LOGD("There is nothing to point the IndexFile\n");
        return NULL;
    }

    entry_point = NULL;
    entry_point = d_list_find_data(index_file->entry_list, match_entry_point, &entry_point_id);

    return entry_point;
}

void delete_entry_point(IndexFile *index_file, EntryPoint *entry_point) {
    int id;
    DList *list;
    EntryPoint *last;

    last = NULL;

    if (!index_file || !entry_point) {
        LOGD("Cant't delete the EntryPoint\n");
        return;
    }

    id = get_entry_point_id(entry_point);
    if (id < 0) {
        LOGD("the entry point id was wrong\n");
        return;
    }

    index_file->entry_list = d_list_remove_with_data(index_file->entry_list, entry_point, free_entry_point);
    
    if (id == index_file->last_id) {
        list = d_list_last(index_file->entry_list);
        last = (EntryPoint*) d_list_get_data(list);
        if (!last) {
            LOGD("There is nothing to point the Entry Point\n");
            return;
        }

        id = get_entry_point_id(last);
        if (id < 0) {
            LOGD("the entry point id was wrong\n");
            return;
        }
        index_file->last_id = id;
    }
    index_file->entry_count = d_list_length(index_file->entry_list);
}

DList* get_list(IndexFile *index_file) {
    if (!index_file) {
        LOGD("There is nothing to point the IndexFile\n");
        return NULL;
    }

    return index_file->entry_list;
}
