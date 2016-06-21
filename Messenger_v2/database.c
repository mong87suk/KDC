#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

static int convert_data_format_to_field_mask(char *data_format) {
    int len;
    int field_mask;
    int i;
    char field;
    field_mask = 0;

    len = strlen(data_format);

    for (i = 0; i < len; i++) {
        field = data_format[i];
        LOGD("field:%c\n", field);
        switch (field) {
        case 'i':
            field_mask = field_mask | ((INTEGER_FIELD) >> i);
            break;

        case 's':
            field_mask = field_mask | ((STRING_FIELD) >> i);
            break;

        default:
            LOGD("filed_mask was wrong\n");
            return -1;
        }
    }
    LOGD("field_mask: 0x%02X\n", field_mask);
    return field_mask;
}

DataBase* new_database(char *file_name, char *data_format) {
    DataBase *database;
    IndexFile *index_file;
    DataFile *data_file;
    int end_offset;
    int result;
    int field_mask;

    if (!file_name || !data_format) {
        LOGD("Can't make the DataBase\n");
        return NULL;
    }

    field_mask = convert_data_format_to_field_mask(data_format);
    if (field_mask < 0) {
        LOGD("Failed to convert field_mask\n");
        return NULL;
    }

    index_file = new_index_file(file_name, field_mask);
    data_file = new_data_file(file_name);

    if (!index_file || !data_file) {
        LOGD("Failed to make the new DataBase\n");
        return NULL;
    }

    end_offset = get_index_file_end_offset(index_file);

    if (end_offset > 0) {
        result = set_index_info(index_file);

        if (!result) {
            LOGD("Falied to make the new DataBase\n");
            return NULL;
        }
    }

    database = (DataBase*) malloc(sizeof(DataBase));
    if (!database) {
        LOGD("Faield to make the new DataBase\n");
        return NULL;
    }

    database->index_file = index_file;
    database->data_file = data_file;
    database->field_mask = field_mask;

    return database;
}

void destroy_database(DataBase *database) {
    if (!database) {
        LOGD("Failed to destroy the DataBase\n");
        return;
    }

    destroy_index_file(database->index_file);
    destroy_data_file(database->data_file);

    free(database);
}

int add_entry(DataBase *database, char *buf) {
    EntryPoint *entry_point;
    int offset, id, fd;
    int result;

    if (!database || !buf) {
        LOGD("Failed to add entry\n");
        return -1;
    }
    
    offset = get_data_file_offset(database->data_file);
    if (offset < 0) {
        LOGD("Failed to get data file offset\n");
        return -1;
    }

    id = create_entry_point_id(database->index_file);
    if (id < 0) {
        LOGD("Failed to create the entry point id\n");
        return -1;
    }

    fd = get_data_file_fd(database->data_file);
    if (fd < 0) {
        LOGD("Failed to get the data file fd\n");
        return -1;
    }

    entry_point = new_entry_point(id, fd, offset, database->field_mask);
    if (!entry_point) {
        LOGD("Failed to make the Entry_Point\n");
        return -1;
    }

    result = set_value(entry_point, buf);
    if (!result) {
        LOGD("Failed to set value\n");
        return -1;
    }

    result = set_last_id(database->index_file, id);
    if (!result) {
        LOGD("Failed to set last id\n");
        return -1;
    }

    result = add_entry_point(database->index_file, entry_point);
    if (!result) {
        LOGD("Failed to add EntryPoint\n");
        return -1;
    }

    result = update_index_file(database->index_file, database->field_mask);
    if (!result) {
        LOGD("Failed to update indexfile\n");
        return -1;
    }

    return id;
}

int get_entry_point_count(DataBase *database) {
    int count;
    if(!database) {
        LOGD("There is nothing to point the DataBase\n");
        return -1;
    }

    count = get_count(database->index_file);
    if (count < 0) {
        LOGD("Failed to get count\n");
        return -1;
    }
    return count;
}

void delete_entry(DataBase *database, int entry_point_id) {
    EntryPoint *entry_point;
    if (!database) {
        LOGD("There is nothing to point the DataBase\n");
        return;
    }

    entry_point = find_entry_point(database->index_file, entry_point_id);

    if (!entry_point) {
        LOGD("There is nothing to point the EntryPoint\n");
        return;
    }

    delete_entry_point(entry_point);
    update_index_file(database->index_file);
}
