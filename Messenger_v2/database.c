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

static int data_base_convert_data_format_to_field_mask(char *data_format) {
    int len;
    int field_mask;
    int i, j;
    char field;
    field_mask = 0;

    len = strlen(data_format);
    j = 0;
    for (i = len - 1; i >= 0; i--) {
        field = data_format[j];
        LOGD("field:%c\n", field);

        if (field == 'i') {
            field_mask = field_mask | ((INTEGER_FIELD) << (FIELD_SIZE * i));
        }

        if (field == 's') {
            field_mask = field_mask | ((STRING_FIELD) << (FIELD_SIZE * i));
            LOGD("s field_mask: 0x%02X\n", field_mask);
        }
        j++;
    }
    LOGD("field_mask: 0x%02X\n", field_mask);
    return field_mask;
}

static int database_set_value(EntryPoint *entry_point, Stream_Buf *entry, int field_mask, int fd) {
    char colum;
    int len, n_byte;
    int id;
    int count = 0;

    if (!entry_point) {
        LOGD("There is nothing to point the EntryPoint\n");
        return FALSE;
    }

    if (fd < 0 || filed_mask < 0) {
        LOGD("Can't set value\n");
        return FALSE;
    }

    len = 0;
    id = entry_point->id;

    n_byte = write_n_byte(fd, &id, sizeof(id));
    if (n_byte != sizeof(id)) {
        LOGD("Failed to write n byte\n");
        return FALSE;
    }
    count = entry_point_get_count_to_move_flag(field_mask);
    do {
        colum = (field_mask >> (FIELD_SIZE * count)) & FIELD_TYPE_FLAG;
        LOGD("colum:%d\n", colum);
        switch (colum) {
            case 1:
                n_byte = write_n_byte(fd, buf, sizeof(int));
                if (n_byte != sizeof(int)) {
                    LOGD("Failed to write n byte\n");
                    return FALSE;
                }
                buf += sizeof(int);
                break;

            case 2:
                memcpy(&len, buf, sizeof(int));
                LOGD("len:%d\n", len);
                if (len < 0) {
                    LOGD("len value was wrong\n");
                    return FALSE;
                }
                n_byte = write_n_byte(fd, buf, sizeof(int));
                if (n_byte != sizeof(int)) {
                    LOGD("Failed to write n byte\n");
                    return FALSE;
                }
                buf += sizeof(int);

                n_byte = write_n_byte(fd, buf, len);
                if (n_byte != len) {
                    LOGD("Failed to write n byte\n");
                    return FALSE;
                }
                buf += len;
                break;

            case 0:
                break;

            default:
                LOGD("field mask was wrong\n");
                return FALSE;
        }
        count--;
    } while (colum && count >= 0);

    return TRUE;
}

static Stream_Buf* database_create_update_entry(EntryPoint *entry_point, int where, Stream_Buf *field, Stream_Buf *entry, int offset, int field_mask) {
    Stream_Buf *stream_buf;
    DList *stream_buf_list;
    char *buf;
    int field_mask;
    int len;
    int colum, index;
    int buf_size;
    int count = 0;

    if (!entry_point || !field || !entry) {
        LOGD("Can't create update entry\n");
        return FALSE;
    }

    if (where < 0 || offset < 0 || field_mask < 0) {
        LOGD("Can't create update entry\n");
        return FALSE;
    }

    stream_buf_list = NULL;
    len = 0;
    index = 0;
    buf_size = 0;
    count = utils_entry_point_get_count_to_move_flag(field_mask);
    do {
        colum = (field_mask >> (FIELD_SIZE * count)) & FIELD_TYPE_FLAG;
        switch (colum) {
            case 1:
                stream_buf = new_stream_buf(sizeof(int));
                if (!stream_buf) {
                    LOGD("Failed to make the StreamBuf\n");
                    return NULL;
                }
                buf_size += sizeof(int);
                if (index == where - 1) {
                    memcpy(get_available_buf(stream_buf), field, get_available_size(stream_buf));
                } else {
                    memcpy(get_available_buf(stream_buf), entry, get_available_size(stream_buf));
                    entry += sizeof(int);
                }

                increase_position(stream_buf, sizeof(int));
                stream_buf_list = d_list_append(stream_buf_list, stream_buf);
                break;

            case 2:
                stream_buf = new_stream_buf(sizeof(int));
                if (!stream_buf) {
                    LOGD("Failed to make the StreamBuf\n");
                    return NULL;
                }
                buf_size += sizeof(int);
                
                if (index == where - 1) {
                    memcpy(get_available_buf(stream_buf), field, get_available_size(stream_buf));
                    field += sizeof(int);
                } else {
                    memcpy(get_available_buf(stream_buf), entry, get_available_size(stream_buf));
                    entry += sizeof(int);
                }
                
                increase_position(stream_buf, sizeof(int));
                buf = get_buf(stream_buf);
                if (!buf) {
                    LOGD("Failed to get buf\n");
                    return NULL;
                }

                memcpy(&len, buf, sizeof(int));
                if (len < 0) {
                    LOGD("len value was wrong\n");
                    return NULL;
                }

                stream_buf_list = d_list_append(stream_buf_list, stream_buf);

                stream_buf = new_stream_buf(len);
                if (!stream_buf) {
                    LOGD("Failed to make the StreamBuf\n");
                    return NULL;
                }
                buf_size += len;

                if (index == where -1) {
                    memcpy(get_available_buf(stream_buf), field, get_available_size(stream_buf));
                } else {
                    memcpy(get_available_buf(stream_buf), entry, get_available_size(stream_buf));
                    entry += len;
                }
                increase_position(stream_buf, len);
                stream_buf_list = d_list_append(stream_buf_list, stream_buf);
                break;

            case 0:
                break;

            default:
                LOGD("field mask was wrong\n");
                return NULL;
        }
        index++;
        count--;
    } while (colum && count >= 0);

    stream_buf = new_stream_buf(buf_size);
    append_data_to_buf(stream_buf_list, stream_buf);
    destroy_stream_buf_list(stream_buf_list);
    entry_point_set_offset(offset);

    return stream_buf;
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

int database_add_entry(DataBase *database, Stream_Buf *entry) {
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

    entry_point = new_entry_point(id, offset, database);
    if (!entry_point) {
        LOGD("Failed to make the Entry_Point\n");
        return -1;
    }

    result = data_base_set_value(entry, fd);
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

    result = update_index_file(database->index_file);
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

int delete_entry(DataBase *database, int entry_point_id) {
    EntryPoint *entry_point;
    if (!database) {
        LOGD("There is nothing to point the DataBase\n");
        return FALSE;
    }

    entry_point = find_entry_point(database->index_file, entry_point_id);
    if (!entry_point) {
        LOGD("There is nothing to point the EntryPoint\n");
        return FALSE;
    }

    delete_entry_point(database->index_file, entry_point);
    update_index_file(database->index_file);

    return TRUE;
}

DList* get_entry_point_list(DataBase *database) {
    DList *list;

    if (!database) {
        LOGD("There is nothing to point the DataBase\n");
        return NULL;
    }
    list = get_list(database->index_file);
    return list;
}

int database_update_entry(DataBase *database, int id, int colum, Stream_Buf *field) {
    Stream_Buf *entry;
    Stream_Buf *updated_entry;
    EntryPoint *entry_point;
    char *buf;
    int offset;
    int result;
    int fd;

    if (!database) {
        LOGD("There is nothing to point the DataBase\n");
        return FALSE;
    }

    entry_point = find_entry_point(database->index_file, id);
    if (!entry_point) {
        LOGD("There is nothing to point the EntryPoint\n");
        return FALSE;
    }

    fd = get_data_file_fd(database->data_file);
    if (fd < 0) {
        LOGD("Failed to get fd\n");
        return FALSE;
    }

    entry = get_value(entry_point, fd);
    if (!entry) {
        LOGD("Failed to get value\n");
        return FALSE;
    }

    offset = get_data_file_offset(database->data_file);
    if (offset < 0) {
        LOGD("Failed to get data file offset\n");
        return FALSE;
    }

    updated_entry = database_create_update_entry(entry_point, colum, field, entry, offset);
    destroy_stream_buf(entry);

    if (!updated_entry) {
        LOGD("Failed to update field\n");
        return FALSE;
    }
    result = database_set_value(entry_point, updated_entry, database->field_mask, fd);
    destroy_stream_buf(updated_entry);
    if (!result) {
        LOGD("Failed to set value\n");
        return FALSE;
    }

    return TRUE;
}

Stream_Buf* get_entry(DataBase *database, int entry_point_id) {
    EntryPoint *entry_point;
    Stream_Buf *entry;
    int fd;

    if (!database) {
        LOGD("There is nothing to point the DataBase\n");
        return NULL;
    }

    entry_point = find_entry_point(database->index_file, entry_point_id);
    if (!entry_point) {
        LOGD("There is nothing to point the EntryPoint\n");
        return NULL;
    }

    fd = get_data_file_fd(database->data_file);
    if (fd < 0) {
        LOGD("Failed to get fd\n");
        return NULL;
    }

    entry = get_value(entry_point, fd);
    if (!entry) {
        LOGD("Failed to get value\n");
        return NULL;
    }

    return entry;
}
