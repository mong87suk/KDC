#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#include "DBLinkedList.h"
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

static void append_data(void *data, void *user_data) {
    char *dest, *src;
    int copy_n;
    Stream_Buf *data_stream_buf;
    Stream_Buf *user_data_stream_buf;

    data_stream_buf = (Stream_Buf*) data;
    user_data_stream_buf = (Stream_Buf*) user_data;

    if (!data_stream_buf || !user_data_stream_buf) {
        LOGD("There is nothing to point Stream_Buf\n");
        return;
    }

    dest = get_available_buf(user_data_stream_buf);
    src = get_buf(data_stream_buf);

    if (!src || !dest) {
        LOGD("There is nothing to point buf\n");
        return;
    }

    copy_n = get_position(data_stream_buf);
    memcpy(dest, src, copy_n);
    increase_position(user_data_stream_buf, copy_n);
}

static int append_data_to_buf(DList *stream_buf_list, Stream_Buf *stream_buf) {
    if (!stream_buf_list || !stream_buf) {
        LOGD("Can't append data to Stream_Buf\n");
        return FALSE;
    }
    d_list_foreach(stream_buf_list, append_data, stream_buf);
    return TRUE;
}

EntryPoint* new_entry_point(int id, int offset, int field_mask) {
    EntryPoint *entry_point;

    if (id < 0 || offset < 0 || field_mask < 0) {
        LOGD("Can't make the entry_point\n");
        return NULL;
    }

    entry_point = (EntryPoint*) malloc(sizeof(EntryPoint));

    if (!entry_point) {
        LOGD("Failed to make the Entry_Point\n");
        return NULL;
    }

    entry_point->id = id;
    entry_point->offset = offset;
    entry_point->field_mask = field_mask;

    return entry_point;
}

static void free_stream_buf(void *data) {
    Stream_Buf *stream_buf;

    stream_buf = (Stream_Buf*) data;

    if (!stream_buf) {
        LOGD("There is nothing to remove the Stream_Buf\n");
        return;
    }
    destroy_stream_buf(stream_buf);
}

static void destroy_stream_buf_list(DList *stream_buf_list) {
    if (!stream_buf_list) {
        LOGD("There is nothing to point the Stream Buf\n");
        return;
    }
    d_list_free(stream_buf_list, free_stream_buf);
}

void destroy_entry_point(EntryPoint *entry_point) {
    if (!entry_point) {
        LOGD("Failed to destroy the entry_point\n");
        return;
    }
    free(entry_point);
}

int get_entry_point_size() {
    return sizeof(EntryPoint);
}

Stream_Buf* entry_point_get_value(EntryPoint *entry_point, int fd) {
    DList *stream_buf_list;
    Stream_Buf *stream_buf;
    int colum;
    int id, offset, field_mask;
    int buf_size, n_byte;
    char *buf;
    int len;
    int count = 0;

    
    if (!entry_point) {
        LOGD("Thre is nothing to point the EntryPoint\n");
        return NULL;
    }

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

    database_get_field_mask(entry_point->database);
    field_mask = entry_point->field_mask;

    offset = lseek(fd, entry_point->offset, SEEK_SET);
    if (offset != entry_point->offset) {
        LOGD("Failed to set offset\n");
        return NULL;
    }

    n_byte = read_n_byte(fd, &id, sizeof(id));
    if (n_byte != sizeof(id)) {
        LOGD("Failed to read id\n");
        return NULL;
    }

    if (id != entry_point->id) {
        LOGD("the entry id was wrong\n");
        return NULL;
    }
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
                n_byte = read_n_byte(fd, get_available_buf(stream_buf), get_available_size(stream_buf));
                if (n_byte != sizeof(int)) {
                    LOGD("Failed to write n byte\n");
                    return NULL;
                }
                increase_position(stream_buf, n_byte);

                stream_buf_list = d_list_append(stream_buf_list, stream_buf);
                break;

            case 2:
                stream_buf = new_stream_buf(sizeof(int));
                if (!stream_buf) {
                    LOGD("Failed to make the StreamBuf\n");
                    return NULL;
                }
                
                n_byte = read_n_byte(fd, get_available_buf(stream_buf), get_available_size(stream_buf));
                if (n_byte != sizeof(int)) {
                    LOGD("Failed to write n byte\n");
                    return NULL;
                }
                increase_position(stream_buf, n_byte);
                buf = get_buf(stream_buf);
                memcpy(&len, buf, n_byte);
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
                n_byte = read_n_byte(fd, get_available_buf(stream_buf), get_available_size(stream_buf));
                if (n_byte != len) {
                    LOGD("Failed to write n byte\n");
                    return FALSE;
                }
                increase_position(stream_buf, n_byte);

                stream_buf_list = d_list_append(stream_buf_list, stream_buf);
                break;

            case 0:
                break;

            default:
                LOGD("field mask was wrong\n");
                return NULL;
        }
        count--;
    } while (colum && count >= 0);

    stream_buf = new_stream_buf(buf_size);
    append_data_to_buf(stream_buf_list, stream_buf);
    destroy_stream_buf_list(stream_buf_list);

    return stream_buf;
}

int get_entry_point_id(EntryPoint *entry_point) {
    if (!entry_point) {
        LOGD("There is nothing to point the EntryPoint\n");
        return -1;
    }

    return entry_point->id;
}

int entry_point_set_offset(EntryPoint *entry_point, int offset) {
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
