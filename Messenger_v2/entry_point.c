#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#include <kdc/DBLinkedList.h>

#include "entry_point.h"
#include "utils.h"
#include "m_boolean.h"
#include "stream_buf.h"
#include "database.h"

struct _EntryPoint {
    int id;
    int offset;
    DataBase *database;
};

EntryPoint *new_entry_point(int id, int offset, DataBase *database) {
    EntryPoint *entry_point;

    if (id < 0 || offset < 0) {
        LOGD("Can't make the entry_point\n");
        return NULL;
    }

    if (!database) {
        LOGD("There is nothing to point the DataBase\n");
        return NULL;
    }

    entry_point = (EntryPoint*) malloc(sizeof(EntryPoint));

    if (!entry_point) {
        LOGD("Failed to make the Entry_Point\n");
        return NULL;
    }

    entry_point->id = id;
    entry_point->offset = offset;
    entry_point->database = database;

    return entry_point;
}

void destroy_entry_point(EntryPoint *entry_point) {
    if (!entry_point) {
        LOGD("Failed to destroy the entry_point\n");
        return;
    }
    free(entry_point);
}

Stream_Buf *entry_point_get_value(EntryPoint *entry_point) {
    DList *stream_buf_list;
    Stream_Buf *stream_buf;
    int colum;
    int id, offset, field_mask;
    int buf_size, n_byte;
    char *buf;
    int len, fd;
    int count = 0;
    int i;
    BOOLEAN result;

    if (!entry_point) {
        LOGD("Thre is nothing to point the EntryPoint\n");
        return NULL;
    }

    fd = database_get_data_file_fd(entry_point->database);
    if (fd < 0) {
        LOGD("fd value is wrong\n");
        return NULL;
    }

    count = 0;
    id = 0;
    len = 0;
    buf_size = 0;
    stream_buf = NULL;
    stream_buf_list = NULL;

    field_mask = database_get_field_mask(entry_point->database);
    if (field_mask < 0) {
        LOGD("Failed to get the field_mask\n");
        return NULL;
    }

    offset = lseek(fd, entry_point->offset, SEEK_SET);
    if (offset != entry_point->offset) {
        LOGD("Failed to set offset\n");
        return NULL;
    }

    n_byte = utils_read_n_byte(fd, &id, sizeof(id));
    if (n_byte != sizeof(id)) {
        LOGD("Failed to read id\n");
        return NULL;
    }

    if (id != entry_point->id) {
        LOGD("the entry id was wrong\n");
        return NULL;
    }

    stream_buf = new_stream_buf(ID_SIZE);
    if (!stream_buf) {
        LOGD("Failed to new stream buf\n");
        return NULL;
    }
    buf_size += ID_SIZE;
    buf = stream_buf_get_buf(stream_buf);
    if (!buf) {
        LOGD("Failed to get buf\n");
        return NULL;
    }
    memcpy(buf, &id, ID_SIZE);
    stream_buf_increase_pos(stream_buf, n_byte);
    stream_buf_list = d_list_append(stream_buf_list, stream_buf);

    count = utils_get_colum_count(field_mask);
    if (count <= 0) {
        LOGD("Failed to get colum count\n");
        return NULL;
    }
    count -= 1;

    for (i = count; i >= 0; i--) {
        colum = (field_mask >> (FIELD_SIZE * i)) & FIELD_TYPE_FLAG;
        switch (colum) {
            case INTEGER_FIELD:
                stream_buf = new_stream_buf(sizeof(int));
                if (!stream_buf) {
                    LOGD("Failed to make the StreamBuf\n");
                    return NULL;
                }
                buf_size += sizeof(int);
                n_byte = utils_read_n_byte(fd, stream_buf_get_available(stream_buf), stream_buf_get_available_size(stream_buf));
                if (n_byte != sizeof(int)) {
                    LOGD("Failed to write n byte\n");
                    return NULL;
                }
                stream_buf_increase_pos(stream_buf, n_byte);
                stream_buf_list = d_list_append(stream_buf_list, stream_buf);
                break;

            case STRING_FIELD: case KEYWORD_FIELD:
                stream_buf = new_stream_buf(sizeof(int));
                if (!stream_buf) {
                    LOGD("Failed to make the StreamBuf\n");
                    return NULL;
                }

                n_byte = utils_read_n_byte(fd, stream_buf_get_available(stream_buf), stream_buf_get_available_size(stream_buf));
                if (n_byte != sizeof(int)) {
                    LOGD("Failed to write n byte\n");
                    return NULL;
                }
                stream_buf_increase_pos(stream_buf, n_byte);
                buf = stream_buf_get_buf(stream_buf);
                memcpy(&len, buf, n_byte);
                if (len < 0) {
                    LOGD("len value was wrong\n");
                    return NULL;
                }
                stream_buf_list = d_list_append(stream_buf_list, stream_buf);
                buf_size += sizeof(int);

                stream_buf = new_stream_buf(len);
                if (!stream_buf) {
                    LOGD("Failed to make the StreamBuf\n");
                    return NULL;
                }
                buf_size += len;
                n_byte = utils_read_n_byte(fd, stream_buf_get_available(stream_buf), stream_buf_get_available_size(stream_buf));
                if (n_byte != len) {
                    LOGD("Failed to write n byte\n");
                    return FALSE;
                }
                stream_buf_increase_pos(stream_buf, n_byte);
                stream_buf_list = d_list_append(stream_buf_list, stream_buf);
                break;

            case 0:
                break;

            default:
                LOGD("field mask was wrong\n");
                return NULL;
        }
    }

    stream_buf = new_stream_buf(buf_size);
    result = utils_append_data_to_buf(stream_buf_list, stream_buf);
    utils_destroy_stream_buf_list(stream_buf_list);

    if (result == FALSE) {
        LOGD("Failed to append data\n");
        return NULL;
    }

    return stream_buf;
}

int entry_point_get_id(EntryPoint *entry_point) {
    if (!entry_point) {
        LOGD("There is nothing to point the EntryPoint\n");
        return -1;
    }

    return entry_point->id;
}

BOOLEAN entry_point_set_offset(EntryPoint *entry_point, int offset) {
    if (!entry_point) {
        LOGD("There is nothing to point the EntryPoint\n");
        return FALSE;
    }

    if (offset < 0) {
        LOGD("Can't set offset\n");
        return FALSE;
    }

    entry_point->offset = offset;

    return TRUE;
}

int entry_point_get_offset(EntryPoint *entry_point) {
    if (!entry_point) {
        LOGD("There is nothing to point the EntryPoint\n");
        return -1;
    }
    return entry_point->offset;
}
