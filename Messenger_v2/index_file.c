#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

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


static void index_file_free_entry(void *data) {
    if (!data) {
        LOGD("There is nothing to point the Entry_Point\n");
        return;
    }

    destroy_entry_point((EntryPoint*) data);
}

static int index_file_match_entry(void *data1, void *data2) {
    EntryPoint *entry_point;
    int entry_id;
    int id;

    if (!data1 || !data2) {
        LOGD("Can't to match entry point\n");
        return 0;
    }

    entry_point = (EntryPoint*) data1;
    entry_id = *((int*) data2);

    id = entry_point_get_id(entry_point);

    if (id == entry_id) {
        return 1;
    } else {
        return 0;
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

static BOOLEAN index_file_load(IndexFile *index_file, DataBase *database) {
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
    entry_point = NULL;

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
            d_list_free(index_file->entry_list, index_file_free_entry);
            return FALSE;
        }

        n_byte = utils_read_n_byte(fd, &offset, sizeof(offset));
        if (n_byte != sizeof(offset) || offset < 0) {
            LOGD("Failed to read the entry point\n");
            d_list_free(index_file->entry_list, index_file_free_entry);
            return FALSE;
        }

        entry_point = new_entry_point(id, offset, database);
        if (!entry_point) {
            LOGD("Failed to get buf\n");
            d_list_free(index_file->entry_list, index_file_free_entry);
            return FALSE;
        }
        index_file->entry_list = d_list_append(index_file->entry_list, entry_point);
    }
    return TRUE;
}

IndexFile *index_file_open(char *name, int field_mask, DataBase *database) {
    IndexFile *index_file;
    char *path;
    int fd, size;
    BOOLEAN result;

    if (!name || strlen(name) < 0) {
        LOGD("Can't open\n");
        return NULL;
    }

    if (!field_mask) {
        LOGD("field_maks was wrong\n");
        return NULL;
    }

    path = utils_create_path(name, INDEXFILE);
    if (!path) {
        LOGD("Failed to make path\n");
        return NULL;
    }
    LOGD("utils_create_path :%s\n", path);

    fd = open(path, O_RDWR | O_APPEND | O_CREAT, S_IRUSR | S_IWUSR);
    if (fd < 0) {
        LOGD("Failed to make open the index file\n");
        free(path);
        return NULL;
    }

    index_file = (IndexFile*) malloc(sizeof(IndexFile));
    if (!index_file) {
        LOGD("Faield to make the Index File\n");
        if (close(fd) < 0) {
            LOGD("Failed to close\n");
            return NULL;
        }

        if (unlink(path) < 0) {
            LOGD("Failed to unlink\n");
            return NULL;
        }
        return NULL;
    }

    size = lseek(fd, 0, SEEK_END);
    if (size < 0) {
        LOGD("Failed to get the size\n");
        if (close(fd) < 0) {
            LOGD("Failed to close\n");
            return NULL;
        }

        if (unlink(path) < 0) {
            LOGD("Failed to unlink\n");
            return NULL;
        }
        free(index_file);
        return NULL;
    }

    index_file->fd = fd;
    index_file->path = path;
    index_file->field_mask = field_mask;

    if (size > 0) {
        result = index_file_load(index_file, database);
        if (!result) {
            LOGD("Failed to load indexfile\n");
            if (close(fd) < 0) {
                LOGD("Failed to close\n");
                return NULL;
            }

            if (unlink(path) < 0) {
                LOGD("Failed to unlink\n");
                return NULL;
            }
            free(index_file);
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

    d_list_free(index_file->entry_list, index_file_free_entry);

    free(index_file->path);
    free(index_file);
    return;
}

BOOLEAN index_file_update(IndexFile *index_file) {
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

    d_list_foreach(index_file->entry_list, index_file_write_entry_point, &fd);

    return TRUE;
}

void index_file_delete_all(IndexFile *index_file) {
    int result;
    if (!index_file) {
        LOGD("There is nothing to point the Index_File\n");
        return;
    }

    d_list_free(index_file->entry_list, index_file_free_entry);
    index_file->entry_list = NULL;
    index_file->last_id = 0;
    index_file->entry_count = 0;

    result = index_file_update(index_file);

    if (!result) {
        LOGD("Failed to update\n");
        return;
    }

    return;
}

int index_file_get_last_id(IndexFile *index_file) {
    if (!index_file) {
        LOGD("There is nohting to point the Index_File\n");
        return -1;
    }

    return index_file->last_id;
}

BOOLEAN index_file_set_last_id(IndexFile *index_file, int last_id) {
    if (!index_file) {
        LOGD("There is nothing to point the Index_File\n");
        return FALSE;
    }

    index_file->last_id = last_id;
    return TRUE;
}

BOOLEAN index_file_add_entry(IndexFile *index_file, EntryPoint *entry_point) {
    if (!index_file || !entry_point) {
        LOGD("Can't add the EntryPoint\n");
        return FALSE;
    }

    index_file->entry_list = d_list_append(index_file->entry_list, entry_point);
    index_file->entry_count = d_list_length(index_file->entry_list);
    return TRUE;
}

int index_file_get_count(IndexFile *index_file) {
    if (!index_file) {
        LOGD("There is nothing to point the IndexFile\n");
        return -1;
    }

    return index_file->entry_count;
}

EntryPoint *index_file_find_entry(IndexFile *index_file, int id) {
    EntryPoint *entry_point;

    if (!index_file) {
        LOGD("There is nothing to point the IndexFile\n");
        return NULL;
    }

    entry_point = NULL;
    entry_point = d_list_find_data(index_file->entry_list, index_file_match_entry, &id);

    return entry_point;
}

void index_file_delete_entry(IndexFile *index_file, EntryPoint *entry_point) {
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

    index_file->entry_list = d_list_remove_with_data(index_file->entry_list, entry_point, index_file_free_entry);

    if (id == index_file->last_id) {
        list = d_list_last(index_file->entry_list);

        if (!list) {
            LOGD("Last entry has been removed\n");
            index_file->last_id = 0;
        } else {
            last = (EntryPoint*) d_list_get_data(list);
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

DList *index_file_get_list(IndexFile *index_file) {
    if (!index_file) {
        LOGD("There is nothing to point the IndexFile\n");
        return NULL;
    }

    return index_file->entry_list;
}

EntryPoint *index_file_nth_entry(IndexFile *index_file, int nth) {
    DList *list;
    EntryPoint* entry;

    if (!index_file) {
        LOGD("There is noting to point the IndexFile\n");
        return NULL;
    }

    list = d_list_nth_for(index_file->entry_list, nth);
    entry = (EntryPoint*) d_list_get_data(list);

    if (!entry) {
        LOGD("Can't get nth entry\n");
        return NULL;
    }

    return entry;
}
