#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "DBLinkedList.h"
#include "index_file.h"
#include "utils.h"
#include "entry_point.h"
#include "m_boolean.h"
#include "database.h"

struct _IndexFile {
    int field_mask;
    int fd;
    int last_id;
    int entry_count;
    char *path;
    DList *entry_list;
};

static int index_file_match_entry_point(void *data1, void *data2) {
    EntryPoint *entry_point;
    int entry_id;
    int id;

    if (!data1 || !data2) {
        LOGD("Can't to match entry point\n");
        return FALSE;
    }

    entry_point = (EntryPoint*) data1;
    entry_id = *((int*) data2);

    id = entry_point_get_id(entry_point);

    if (id == entry_id) {
        return TRUE;
    } else {
        return FALSE;
    }
}

static void index_file_write_entry_point(void *data, void *user_data) {
    EntryPoint *entry_point;
    int fd;
    int n_byte;
    int id;
    int offset;

    if (!data || !user_data) {
        LOGD("Can't write the EntryPoint\n");
        return;
    }

    entry_point = (EntryPoint*) data;
    fd = *((int*)user_data);

    id = entry_point_get_id(entry_point);
    if (id < 0) {
        LOGD("Failed to get the id\n");
        return;
    }

    n_byte = write_n_byte(fd, &id, sizeof(id));
    if (n_byte != sizeof(id)) {
        LOGD("Failed to write the id\n");
        return;
    }

    offset = entry_point_get_offset(entry_point);
    if (offset < 0) {
        LOGD("Failed to get the offset\n");
        return;
    }

    n_byte = write_n_byte(fd, &offset, sizeof(offset));
    if (n_byte != sizeof(offset)) {
        LOGD("Failed to write EntryPoint\n");
        return;
    }
}

static int index_file_load(IndexFile *index_file, DataBase *database) {
    int field_mask, last_id, entry_count;
    int n_byte, fd;
    int id, offset;
    int i;
    int read_size;
    EntryPoint *entry_point;

    if (!index_file) {
        LOGD("There is nothing to point the index file\n");
        return FALSE;
    }
    offset = 0;
    field_mask = 0;
    fd = index_file->fd;

    if (fd < 0) {
        LOGD("fd was wrong\n");
        return FALSE;
    }

    offset = lseek(fd, 0, SEEK_SET);
    if (offset < 0) {
        LOGD("Failed to set offset\n");
        return FALSE;
    }

    read_size = sizeof(int);
    n_byte = utils_read_n_byte(fd, &field_mask, read_size);
    if (n_byte != read_size) {
        LOGD("Failed to read the last id\n");
        return FALSE;
    }

    if (index_file->field_mask != field_mask) {
        LOGD("field_mask was updated\n");
        index_file->field_mask = field_mask;
    }

    n_byte = utils_read_n_byte(fd, &last_id, read_size);
    if (n_byte != read_size) {
        LOGD("Failed to read the last id\n");
        return FALSE;
    }
    index_file->last_id = last_id;

    n_byte = utils_read_n_byte(fd, &entry_count, read_size);
    if (n_byte != read_size) {
        LOGD("Failed to read the size\n");
        return FALSE;
    }
    index_file->entry_count = entry_count;
    
    offset = 0;
    for (i = 0; i < entry_count; i++) {

        n_byte = utils_read_n_byte(fd, &id , sizeof(id));
        if (n_byte != sizeof(id)  || id < 0) {
            LOGD("Failed to read the entry point\n");
            return FALSE;
        }

        n_byte = utils_read_n_byte(fd, &offset, sizeof(offset));
        if (n_byte != sizeof(offset) || offset < 0) {
            LOGD("Failed to read the entry point\n");
            return FALSE;
        }

        entry_point = new_entry_point(id, offset, database);
        if (!entry_point) {
            LOGD("Failed to get buf\n");
            return FALSE;
        }
        index_file->entry_list = d_list_append(index_file->entry_list, entry_point);
    }
    return TRUE;
}

static void index_file_free_entry_point(void *data) {
    if (!data) {
        LOGD("There is nothing to point the Entry_Point\n");
        return;
    }

    destroy_entry_point((EntryPoint*) data);
}

IndexFile* index_file_open(char *name, int field_mask, DataBase *database) {
    IndexFile *index_file;
    char *path;
    int fd, size;
    int result;

    if (!name || strlen(name) < 0) {
        LOGD("Can't open\n");
        return NULL;
    }

    path = utils_create_path(name, INDEXFILE);
    if (!path) {
        LOGD("Failed to make path\n");
        return NULL;
    }
    LOGD("utils_create_path :%s\n", path);

    fd = utils_open(path);
    if (fd < 0) {
        LOGD("Failed to make open the index file\n");
        return NULL;
    }

    index_file = (IndexFile*) malloc(sizeof(IndexFile));
    if (!index_file) {
        LOGD("Faield to make the Index File\n");
        return NULL;
    }

    size = lseek(fd, 0, SEEK_END);
    if (size < 0) {
        LOGD("Failed to get the size\n");
        return NULL;
    }

    index_file->fd = fd;
    index_file->path = path;

    if (size > 0) {
        result = index_file_load(index_file, database);
        if (!result) {
            LOGD("Failed to load indexfile\n");
            return NULL;
        }
    } else {
        index_file->last_id = 0;
        index_file->entry_count = 0;
        index_file->entry_list = NULL;
        index_file->field_mask = field_mask;
    }

    return index_file;
}

void index_file_close(IndexFile *index_file) {
    if (!index_file) {
        LOGD("There is nothing to point the Index_File\n");
        return;
    }

    if (close(index_file->fd) < 0) {
        LOGD("Failed to close\n");
        return;
    }

    d_list_free(index_file->entry_list, index_file_free_entry_point);


    free(index_file->path);
    free(index_file);
    return;
}

void index_file_delete_all(IndexFile *index_file) {
    if (!index_file) {
        LOGD("There is nothing to point the Index_File\n");
        return;
    }

    d_list_free(index_file->entry_list, index_file_free_entry_point);
    index_file->entry_list = NULL;
    index_file->last_id = 0;
    index_file->entry_count = 0;
    return;
}

int index_file_get_last_id(IndexFile *index_file) {
    if (!index_file) {
        LOGD("There is nohting to point the Index_File\n");
        return -1;
    }

    return index_file->last_id;
}

int index_file_set_last_id(IndexFile *index_file, int last_id) {
    if (!index_file) {
        LOGD("There is nothing to point the Index_File\n");
        return FALSE;
    }

    index_file->last_id = last_id;
    return TRUE;
}

int index_file_add_entry_point(IndexFile *index_file, EntryPoint *entry_point) {
    if (!index_file || !entry_point) {
        LOGD("Can't add the EntryPoint\n");
        return FALSE;
    }

    index_file->entry_list = d_list_append(index_file->entry_list, entry_point);
    index_file->entry_count = d_list_length(index_file->entry_list);
    return TRUE;
}

int index_file_update(IndexFile *index_file) {
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

    fd = utils_open(index_file->path);

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

    d_list_foreach(index_file->entry_list, index_file_write_entry_point, &fd);

    return TRUE;
}

int index_file_get_count(IndexFile *index_file) {
    if (!index_file) {
        LOGD("There is nothing to point the IndexFile\n");
        return -1;
    }

    return index_file->entry_count;
}

EntryPoint* index_file_find_entry_point(IndexFile *index_file, int entry_point_id) {
    EntryPoint *entry_point;

    if (!index_file) {
        LOGD("There is nothing to point the IndexFile\n");
        return NULL;
    }

    entry_point = NULL;
    entry_point = d_list_find_data(index_file->entry_list, index_file_match_entry_point, &entry_point_id);

    return entry_point;
}

void index_file_delete_entry_point(IndexFile *index_file, EntryPoint *entry_point) {
    int id;
    DList *list;
    EntryPoint *last;

    last = NULL;

    if (!index_file || !entry_point) {
        LOGD("Cant't delete the EntryPoint\n");
        return;
    }

    id = entry_point_get_id(entry_point);
    if (id < 0) {
        LOGD("the entry point id was wrong\n");
        return;
    }

    index_file->entry_list = d_list_remove_with_data(index_file->entry_list, entry_point, index_file_free_entry_point);

    if (id == index_file->last_id) {
        list = d_list_last(index_file->entry_list);
        last = (EntryPoint*) d_list_get_data(list);

        if (!last) {
            LOGD("There is nothing to point the Entry Point\n");
            index_file->last_id = 0;
        } else {
            id = entry_point_get_id(last);
            if (id < 0) {
                LOGD("the entry point id was wrong\n");
                return;
            }
            index_file->last_id = id;
        }
    }
    index_file->entry_count = d_list_length(index_file->entry_list);
}

DList* index_file_get_list(IndexFile *index_file) {
    if (!index_file) {
        LOGD("There is nothing to point the IndexFile\n");
        return NULL;
    }

    return index_file->entry_list;
}
