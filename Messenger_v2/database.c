#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#include <kdc/DBLinkedList.h>

#include "database.h"
#include "index_file.h"
#include "data_file.h"
#include "entry_point.h"
#include "m_boolean.h"
#include "utils.h"
#include "stream_buf.h"

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
    DList *matched_entry;
    DList *where_list;
    EntryPoint *entry_point;
    BOOLEAN result;
};

struct _FieldList {
    DList *k_list;
    DList *other_list;
    int field_mask;
    int coulumn_count;
};

struct _CompData {
    DataBase *database;
    BOOLEAN boolean;
    int id;
};

static int database_new_field_mask(char *data_format) {
    int len;
    int field_mask;
    int i, j;
    char field;
    field_mask = 0;

    len = strlen(data_format);
    if (FIELD_NUM > 8) {
        LOGD("Data Format was wrong\n");
        return 0;
    }

    j = 0;
    for (i = len - 1; i >= 0; i--) {
        field = data_format[j];

        if (field == 'i') {
            field_mask = field_mask | ((INTEGER_FIELD) << (FIELD_SIZE * i));
        }

        if (field == 'k') {
            field_mask = field_mask | ((KEYWORD_FIELD) << (FIELD_SIZE * i));
        }

        if (field == 's') {
            field_mask = field_mask | ((STRING_FIELD) << (FIELD_SIZE * i));
        }
        j++;
    }
    LOGD("field_mask: 0x%02X\n", field_mask);
    return field_mask;
}

static void destroy_k_list(DList *k_list) {
    if (k_list) {
        d_list_remove(k_list, NULL);    
    }
}

static void destroy_other_list(DList *other_list) {
    if (other_list) {
        d_list_remove(other_list, NULL);
    }
}

static void database_comp_data(void *data, void *user_data) {
    int field_type;
    int i;
    Stream_Buf *comp_data;
    char *buf;
    
    struct _SearchData *search_data; 
    Where *where;
    
    if (!data) {
        LOGD("There is nothing to point the data\n");
        return;
    }

    where = (Where *) data;
    search_data = (struct _SearchData *) user_data;
    if (search_data->result == FALSE) {
        LOGD("comp result is false\n");
        return;
    }

    comp_data = utils_get_data(search_data->database, search_data->entry_point, where->column, &field_type);
    buf = stream_buf_get_buf(comp_data);
    i = 0;

    switch (field_type) {
        case INTEGER_FIELD:
        memcpy(&i, buf, FIELD_I_SIZE);
        if (i == *((int *) where->data)) {
            search_data->result = TRUE;
        } else {
            search_data->result = FALSE;
        }
        break;

        case STRING_FIELD:
        if (strcmp(buf, (char *) where->data) == 0) {
            search_data->result = TRUE;
        } else {
            search_data->result = FALSE;
        }
        break;

        default:
        LOGD("Field type was wrong\n");
        search_data->result = FALSE;
        return;
    }
    destroy_stream_buf(comp_data);
}

static void database_match_data(void *data, void *user_data) {
    struct _SearchData *search_data;
    
    if (!data) {
        LOGD("There is nothing to point the data\n");
        return;
    }

    search_data = (struct _SearchData *) user_data;
    search_data->entry_point = (EntryPoint *) data;
    search_data->result = TRUE;
    
    d_list_foreach(search_data->where_list, database_comp_data, search_data);

    if (search_data->result == TRUE) {
        search_data->matched_entry = d_list_append(search_data->matched_entry, (EntryPoint *) data);
    }
}

static void free_where(Where *where) {
    free(where);
}

static int database_get_field(int field_mask, int column_index, int count) {
    int index = 0;
    int field_type = 0;
    count--;
    for (int i = count; i >= 0; i--) {
        int column = (field_mask >> (FIELD_SIZE * i)) & FIELD_TYPE_FLAG;    
        if (column_index == index) {
            field_type = column;
            break;
        }
        index++;
    }
    return field_type;
} 

static void database_compare_field(void *data, void *user_data) {
    Where *where = (Where *) data;
    struct _FieldList *fieldList = (struct _FieldList *) user_data;
    int field = database_get_field(fieldList->field_mask, where->column, fieldList->coulumn_count);

    if (field == KEYWORD_FIELD) {
        fieldList->k_list = d_list_append(fieldList->k_list, where);
    } else {
        fieldList->other_list = d_list_append(fieldList->other_list, where);
    }
}

static void database_order_where_list(DList *where_list, struct _FieldList *fieldList) {
    d_list_foreach(where_list, database_compare_field, fieldList);
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

    return where;
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

    data_file = data_file_open(name);
    if (!data_file) {
        free(database);
        LOGD("Failed to open data file\n");
        return NULL;
    }
    database->data_file = data_file;
    database->field_mask = field_mask;
    
    index_file = index_file_open(name, field_mask, database);
    if (!index_file) {
        data_file_close(data_file);
        free(database);
        LOGD("Failed to open index file\n");
        return NULL;
    }
    database->index_file = index_file;   
    
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

int database_add_entry(DataBase *database, Stream_Buf *entry, int id) {
    int offset;

    BOOLEAN result;
    BOOLEAN update = FALSE;
    EntryPoint *entry_point;

    if (!database || !entry || id < 0) {
        LOGD("Failed to add entry\n");
        return -1;
    }

    offset = data_file_get_offset(database->data_file);
    if (offset < 0) {
        LOGD("Failed to get data file offset\n");
        return -1;
    }

    entry_point = index_file_find_entry(database->index_file, id);
    if (!entry_point) {
        entry_point = new_entry_point(id, offset, database);
        if (!entry_point) {
            LOGD("Failed to make the Entry_Point\n");
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
    } else {
        update = TRUE;
        entry_point_set_offset(entry_point, offset);
    }

    result = data_file_write_entry(database->data_file, id, entry);
    if (!result) {
        LOGD("Failed to set value\n");
        destroy_entry_point(entry_point);
        index_file_delete_entry(database->index_file, entry_point);
        return -1;
    }

    result = index_file_update(database->index_file);
    if (!result) {
        LOGD("Failed to update indexfile\n");
        destroy_entry_point(entry_point);
        index_file_delete_entry(database->index_file, entry_point);
        return -1;
    }

    int field_mask = database->field_mask;
    int count = utils_get_colum_count(field_mask);
    int column = 0;
    int field_type = 0;
    Stream_Buf *stream_buf = NULL;
    count--;
    for (int i = count; i >= 0; i--) {
        int column_type = (field_mask >> (FIELD_SIZE * i)) & FIELD_TYPE_FLAG;    
        if (column_type == KEYWORD_FIELD) {
            stream_buf = utils_get_data_with_buf(field_mask, entry, column, &field_type);
            char *key = stream_buf_get_buf(stream_buf);
            if (!stream_buf || field_type != KEYWORD_FIELD || !key) {
                LOGD("Failed to get data\n");
                destroy_entry_point(entry_point);
                index_file_delete_entry(database->index_file, entry_point);
                return -1;
            }
            if (update) {
                index_file_update_keyword(database->index_file, column, key, id); 
            } else {
                index_file_insert_keyword(database->index_file, column, key, id); 
            }
        }
        column++;
    }
    
    destroy_stream_buf(stream_buf);
    
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
    Stream_Buf *entry;
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

    entry = entry_point_get_value(entry_point);
    if (!entry) {
        LOGD("Can't delete entry\n");
        return FALSE;
    }

    int field_mask = database->field_mask;
    int count = utils_get_colum_count(field_mask);
    int column = 0;
    int field_type = 0;
    Stream_Buf *stream_buf = NULL;
    count--;
    for (int i = count; i >= 0; i--) {
        int column_type = (field_mask >> (FIELD_SIZE * i)) & FIELD_TYPE_FLAG;    
        if (column_type == KEYWORD_FIELD) {
            stream_buf = utils_get_data(database, entry_point, column, &field_type);
            char *key = stream_buf_get_buf(stream_buf);
            if (!stream_buf || field_type != KEYWORD_FIELD || !key) {
                LOGD("Failed to get data\n");
                destroy_entry_point(entry_point);
                index_file_delete_entry(database->index_file, entry_point);
                return -1;
            }
            index_file_delete_keyword(database->index_file, column, key);    
        }
        column++;
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

int database_update_entry(DataBase *database, EntryPoint *entry_point, Stream_Buf *entry) {
    if (!database || !entry) {
        LOGD("There is nothing to point the DataBase or entry\n");
        return -1;
    }

    int id = entry_point_get_id(entry_point);
    if (id < 0) {
        LOGD("Failed to get entry id\n");
        return -1;
    }
    
    id = database_add_entry(database, entry, id);

    return id;
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

    if (!database || id < 0) {
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

void database_comp_k_field(void *data, void *user_data) {
    Where *where = (Where *)data;
    struct _CompData *comp_data = (struct _CompData *) user_data;
    LOGD("data:%s\n", (char*) where->data);
    int id = index_file_get_entry_id(comp_data->database->index_file, (char*) where->data, where->column);
    LOGD("id:%d\n", id);
    if (comp_data->id == -1) {
        comp_data->id = id;
    } else {
        if (comp_data->id != id) {
            comp_data->boolean = FALSE;
        }
    }
}

static EntryPoint* database_get_entry_point(DataBase *database, DList *k_list) {
    EntryPoint *entry_point = NULL;
    struct _CompData comp_data;

    comp_data.database = database;
    comp_data.boolean = TRUE;
    comp_data.id = -1;

    d_list_foreach(k_list, database_comp_k_field, &comp_data);
    if (comp_data.boolean == TRUE) {
        entry_point = database_find_entry_point(database, comp_data.id); 
    }

    return entry_point;
}

DList *database_search(DataBase *database, DList *where_list) {
    DList *entry_list = NULL;
    EntryPoint *entry_point = NULL;
    struct _SearchData search_data;
    struct _FieldList fieldList;

    if (!database || !where_list) {
        LOGD("Can't search a data\n");
        return NULL;
    }

    int count = utils_get_colum_count(database->field_mask);
    if (count < 0) {
        LOGD("Can't search a data\n");
        return NULL;
    }

    fieldList.k_list = NULL;
    fieldList.other_list = NULL;
    fieldList.field_mask = database->field_mask;
    fieldList.coulumn_count = count;
    database_order_where_list(where_list, &fieldList);

    if (fieldList.k_list) {
        entry_point = database_get_entry_point(database, fieldList.k_list);
        LOGD("id:%d\n", entry_point_get_id(entry_point));
        if (!entry_point) {
            LOGD("Can't search a data\n");
            return NULL;    
        } else {
            entry_list = d_list_append(entry_list, entry_point);
        }
    } else {
        entry_list = database_get_entry_list(database);
    }

    if (!entry_list) {
        LOGD("Can't search a data\n");
        destroy_k_list(fieldList.k_list);
        destroy_other_list(fieldList.other_list);
        return NULL;
    }

    search_data.database = database;
    search_data.matched_entry = NULL;
    search_data.where_list = fieldList.other_list;
    
    d_list_foreach(entry_list, database_match_data, &search_data);
    destroy_k_list(fieldList.k_list);
    destroy_other_list(fieldList.other_list);
    
    return search_data.matched_entry;
}

void destroy_where(void *where) {
    if (!where) {
        LOGD("Failed to destory the where\n");
        return;
    }
    free_where((Where *) where);
}

void destroy_where_list(DList *where_list) {
    if (!where_list) {
        LOGD("Can't' destroy the where list\n");
        return;
    }
    d_list_free(where_list, destroy_where);   
}

void destroy_matched_list(DList *matched_entry) {
    if (!matched_entry) {
        LOGD("Can't destroy the matched list\n");
    }
    d_list_remove(matched_entry, NULL);
}

DataFile *database_get_datafile(DataBase *database) {
    if (!database) {
        LOGD("Can't get the datafile\n");
        return NULL;
    }

    return database->data_file;
}

int database_get_last_id(DataBase *database) {
    if (!database) {
        LOGD("Can't get last id\n");
        return -1;
    }

    int id = index_file_get_last_id(database->index_file);
    return id;
}