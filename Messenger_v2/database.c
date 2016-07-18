#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "DBLinkedList.h"
#include "database.h"
#include "index_file.h"
#include "data_file.h"
#include "entry_point.h"
#include "m_boolean.h"
#include "utils.h"

struct _DataBase {
    IndexFile *index_file;
    DataFile *data_file;
    int field_mask;
};

struct _Where {
    int column;
    void *data;
};

// This struct is usded to search the Data
struct _SearchData {
    DataBase *database;
    DList *matched_list;
    DList *where_list;
};

static int database_new_field_mask(char *data_format) {
    int len;
    int field_mask;
    int i, j;
    char field;
    field_mask = 0;

    len = strlen(data_format);
    if (len > 8) {
        LOGD("Data Format was wrong\n");
        return 0;
    }

    j = 0;
    for (i = len - 1; i >= 0; i--) {
        field = data_format[j];

        if (field == 'i') {
            field_mask = field_mask | ((INTEGER_FIELD) << (FIELD_SIZE * i));
        }

        if (field == 's') {
            field_mask = field_mask | ((STRING_FIELD) << (FIELD_SIZE * i));
        }
        j++;
    }
    LOGD("field_mask: 0x%02X\n", field_mask);
    return field_mask;
}

void *database_get_data(DataBase *database, EntryPoint *entry_point, int column, int *field_type) {
    int field_mask;
    int fd, offset, n_byte;
    int id, entry_id;

    field_mask = database->field_mask;
    fd = data_file_get_fd(database->data_file);
    if (fd < 0) {
        LOGD("Failed to get the fd\n");
        return;
    }

    offset = entry_point_get_offset(entry_point);
    if (offset < 0) {
        LOGD("Failed to get the offset\n");
        return; 
    }

    offset = lseek(fd, offset, SEEK_SET);
    if (offset != entry_point->offset) {
        LOGD("Failed to set offset\n");
        return;
    }

    n_byte = utils_read_n_byte(fd, &id, sizeof(id));
    if (n_byte != sizeof(id)) {
        LOGD("Failed to read id\n");
        return;
    }

    entry_id = entry_point_get_id(entry);
    if (entry_id < 0) {
        LOGD("Failed to get the id\n");
        return;
    }

    if (id != entry_id) {
        LOGD("the entry id was wrong\n");
        return;
    }
}

static void database_match_data(void *data, void *user_data) {
    int field_type;
    int i, len;
    void *data;
    BOOLEAN result;
    
    struct _SearchData *search_data;
    EntryPoint *entry_point;
    DataBase *database;
    DList *where_list;
    DList *next;
    Where *where;

    if (!data) {
        LOGD("There is nothing to point the data\n");
        return;
    }

    entry_point = (EntryPoint *) data;
    search_data = (struct _SearchData *) user_data;
    database = search_data->database;

    where_list = search_data->where_list;
    
}

Where *new_where(int column,void *data) {
    Where *where;

    if (column < 0 || !data) {
        LOGD("Can't make the Where\n'");
        return NULL;
    }

    where = (Where *) malloc(sizeof(Where));
    if (!where) {
        LOGD("Failed to make the Where\n");
        return NULL;
    }

    where->column = column;
    where->data = data;
}

DataBase *database_open(char *name, char *data_format) {
    DataBase *database;
    IndexFile *index_file;
    DataFile *data_file;
    int field_mask;

    if (!name || !data_format) {
        LOGD("Can't make the DataBase\n");
        return NULL;
    }

    field_mask = database_new_field_mask(data_format);
    if (field_mask == 0) {
        LOGD("Failed to convert field_mask\n");
        return NULL;
    }

    database = (DataBase *) malloc(sizeof(DataBase));
    if (!database) {
        LOGD("Faield to make the new DataBase\n");
        return NULL;
    }

    index_file = index_file_open(name, field_mask, database);
    if (!index_file) {
        free(database);
        LOGD("Failed to open index file\n");
        return NULL;
    }

    data_file = data_file_open(name);
    if (!data_file) {
        index_file_close(index_file);
        free(database);
        LOGD("Failed to open data file\n");
        return NULL;
    }

    database->index_file = index_file;
    database->data_file = data_file;
    database->field_mask = field_mask;

    return database;
}

void database_close(DataBase *database) {
    if (!database) {
        LOGD("Failed to destroy the DataBase\n");
        return;
    }

    index_file_close(database->index_file);
    data_file_close(database->data_file);

    free(database);
}

void database_delete_all(DataBase *database) {
    if (!database) {
        LOGD("There is nothing to point the DataBase\n");
        return;
    }

    index_file_delete_all(database->index_file);
    data_file_delete_all(database->data_file);
    index_file_update(database->index_file);
}

int database_add_entry(DataBase *database, Stream_Buf *entry) {
    EntryPoint *entry_point;
    int offset, id;
    BOOLEAN result;

    if (!database || !entry) {
        LOGD("Failed to add entry\n");
        return -1;
    }

    offset = data_file_get_offset(database->data_file);
    if (offset < 0) {
        LOGD("Failed to get data file offset\n");
        return -1;
    }

    id = index_file_get_last_id(database->index_file);
    if (id < 0) {
        LOGD("Failed to create the entry point id\n");
        return -1;
    }

    id += 1;

    entry_point = new_entry_point(id, offset, database);
    if (!entry_point) {
        LOGD("Failed to make the Entry_Point\n");
        return -1;
    }

    result = data_file_write_entry(database->data_file, id, entry);
    if (!result) {
        LOGD("Failed to set value\n");
        destroy_entry_point(entry_point);
        return -1;
    }

    result = index_file_set_last_id(database->index_file, id);
    if (!result) {
        LOGD("Failed to set last id\n");
        destroy_entry_point(entry_point);
        return -1;
    }

    result = index_file_add_entry(database->index_file, entry_point);
    if (!result) {
        LOGD("Failed to add EntryPoint\n");
        destroy_entry_point(entry_point);
        return -1;
    }

    result = index_file_update(database->index_file);
    if (!result) {
        LOGD("Failed to update indexfile\n");
        destroy_entry_point(entry_point);
        return -1;
    }

    return id;
}

int database_get_entry_count(DataBase *database) {
    int count;
    if(!database) {
        LOGD("There is nothing to point the DataBase\n");
        return -1;
    }

    count = index_file_get_count(database->index_file);
    if (count < 0) {
        LOGD("Failed to get count\n");
        return -1;
    }
    return count;
}

BOOLEAN database_delete_entry(DataBase *database, int id) {
    EntryPoint *entry_point;
    int result;

    if (!database) {
        LOGD("There is nothing to point the DataBase\n");
        return FALSE;
    }

    entry_point = index_file_find_entry(database->index_file, id);
    if (!entry_point) {
        LOGD("There is nothing to point the EntryPoint\n");
        return FALSE;
    }

    index_file_delete_entry(database->index_file, entry_point);
    result = index_file_update(database->index_file);

    if (!result) {
        LOGD("Falied to update index file\n");
        return FALSE;
    }

    return TRUE;
}

DList *database_get_entry_list(DataBase *database) {
    DList *list;

    if (!database) {
        LOGD("There is nothing to point the DataBase\n");
        return NULL;
    }
    list = index_file_get_list(database->index_file);
    return list;
}

int database_update_entry(DataBase *database, EntryPoint *entry_point, Stream_Buf *entry, int id) {
    int offset;
    int result;

    if (!database || !entry) {
        LOGD("There is nothing to point the DataBase or entry\n");
        return FALSE;
    }

    offset = data_file_get_offset(database->data_file);
    if (offset < 0) {
        LOGD("Failed to get data file offset\n");
        return FALSE;
    }

    result = entry_point_set_offset(entry_point, offset);
    if (!result) {
        LOGD("Failed to set offset\n");
        return FALSE;
    }

    result = data_file_write_entry(database->data_file, id, entry);
    if (!result) {
        LOGD("Failed to set value\n");
        return FALSE;
    }

    result = index_file_update(database->index_file);
    if (!result) {
        LOGD("Failed to update indexfile\n");
        return FALSE;
    }

    return FALSE;
}

Stream_Buf *database_get_entry(DataBase *database, int id) {
    EntryPoint *entry_point;
    Stream_Buf *entry;
    int fd;

    if (!database) {
        LOGD("There is nothing to point the DataBase\n");
        return NULL;
    }

    entry_point = index_file_find_entry(database->index_file, id);
    if (!entry_point) {
        LOGD("There is nothing to point the EntryPoint\n");
        return NULL;
    }

    fd = data_file_get_fd(database->data_file);
    if (fd < 0) {
        LOGD("Failed to get fd\n");
        return NULL;
    }

    entry = entry_point_get_value(entry_point);
    if (!entry) {
        LOGD("Failed to get value\n");
        return NULL;
    }

    return entry;
}

int database_get_field_mask(DataBase *database) {
    if (!database) {
        LOGD("There is nothing to point the DataBase\n");
        return 0;
    }

    return database->field_mask;
}

EntryPoint *database_find_entry_point(DataBase *database, int id) {
    EntryPoint *entry_point;

    if (!database || id <= 0) {
        LOGD("Can't get entry point\n");
        return NULL;
    }

    entry_point = index_file_find_entry(database->index_file, id);
    if (!entry_point) {
        LOGD("Failed to fined entry point\n");
        return NULL;
    }

    return entry_point;
}

EntryPoint *database_nth_entry_point(DataBase *database, int nth) {
    EntryPoint* entry;

    if (!database) {
        LOGD("Can't get entry point\n");
        return NULL;
    }

    entry = index_file_nth_entry(database->index_file, nth);
    if (!entry) {
        LOGD("Failed to get nth entry\n");
        return NULL;
    }
    return entry;
}

int database_get_data_file_fd(DataBase *database) {
    int fd;

    if (!database) {
        LOGD("There is nothing to point the DataBase");
        return -1;
    }

    fd = data_file_get_fd(database->data_file);
    if (fd < 0) {
        LOGD("Failed to get the fd\n");
        return -1;
    }

    return fd;
}

DList *database_search(DataBase *database, DList *where_list) {
    DList *entry_list;
    struct _SearchData search_data;

    if (!database || !where_list) {
        LOGD("Can't search a data\n");
        return NULL;
    }

    entry_list = database_get_entry_list(database);
    if (!entry_list) {
        LOGD("Can't search a data\n");
        return NULL;
    }

    search_data.database = database;
    search_data.matched_list = NULL;
    search_data.where_list = where_list;
    
    d_list_foreach(entry_list, database_match_data, &serach_data); 
}
