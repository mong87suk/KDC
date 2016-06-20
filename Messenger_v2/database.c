#include <stdio.h>
#include <stdlib.h>

#include "database.h"
#include "index_file.h"
#include "data_file.h"
#include "entry_point.h"
#include "m_boolean.h"
#include "utils.h"

struct _DataBase {
    Index_File *index_file;
    Data_File *data_file;
};

DataBase* new_database(char *index_file_name, char *data_file_name) {
    DataBase *database;
    Index_File *index_file;
    Data_File *data_file;
    int end_offset;
    int result;

    index_file = new_index_file(index_file_name);
    data_file = new_data_file(data_file_name);

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

int add_entry(DataBase *database, char *buf, int field_mask) {
    Entry_Point *entry_point;
    int offset, id, fd;
    int result;

    if (!database || !buf) {
        LOGD("Failed to add entry\n");
        return -1;
    }

    entry_point = new_entry_point();
    if (!entry_point) {
        LOGD("Failed to make the Entry_Point\n");
        return -1;
    }

    offset = get_data_file_offset(database->data_file);
    if (offset < 0) {
        LOGD("Failed to get data file offset\n");
        return -1;
    }

    result = set_offset(entry_point, offset);
    if (!result) {
        LOGD("Failed to set offset\n");
        return -1;
    }

    id = create_entry_point_id(database->index_file);
    if (id < 0) {
        LOGD("Failed to create the entry point id\n");
        return -1;
    }

    result = set_id(entry_point, id);
    if (!result ) {
        LOGD("Failed to set id\n");
        return -1;
    }

    fd = get_data_file_fd(database->data_file);
    if (fd < 0) {
        LOGD("Failed to get the data file fd\n");
        return -1;
    }

    result = set_value(entry_point, fd, field_mask, buf);
    if (!result) {
        LOGD("Failed to set value\n");
        return -1;
    }
}
