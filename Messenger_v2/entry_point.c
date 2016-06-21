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

struct _EntryPoint {
    int id;
    int fd;
    int offset;
    int field_mask;
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

EntryPoint* new_entry_point(int id, int fd, int offset, int field_mask) {
    EntryPoint *entry_point;

    if (id < 0 || fd < 0 || offset < 0 || field_mask < 0) {
        LOGD("Can't make the entry_point\n");
        return NULL;
    }

    entry_point = (EntryPoint*) malloc(sizeof(EntryPoint));

    if (!entry_point) {
        LOGD("Failed to make the Entry_Point\n");
        return NULL;
    }

    entry_point->id = id;
    entry_point->fd = fd;
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

int set_offset(EntryPoint *entry_point, int offset) {
    if (!entry_point) {
        LOGD("There is nothing to point the entry_point\n");
        return FALSE;
    }

    entry_point->offset = offset;

    return TRUE;
}

int set_id(EntryPoint *entry_point, int id) {
    if (!entry_point) {
        LOGD("There is nothing to point the entry_point\n");
        return FALSE;
    }

    entry_point->id = id;

    return TRUE;
}

int set_value(EntryPoint *entry_point, char *buf) {
    char colum;
    int len, n_byte;
    int field_mask, fd, id;
    int max_move;

    max_move = MAX_MOVE;
    field_mask = entry_point->field_mask;
    len = 0;
    fd = entry_point->fd;
    id = entry_point->id;

    n_byte = write_n_byte(fd, &id, sizeof(id));
    if (n_byte != sizeof(id)) {
        LOGD("Failed to write n byte\n");
        return FALSE;
    }

    do {
        colum = (field_mask >> (4 * max_move)) & COLUM_FLAG;
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
        max_move--;
    } while (colum);

    return TRUE;
}

Stream_Buf* get_value(EntryPoint *entry_point) {
    DList *stream_buf_list;
    Stream_Buf *stream_buf;
    int colum;
    int id, fd, offset, field_mask;
    int buf_size, n_byte;
    char *buf;
    int max_move;
    int len;

    if (!entry_point) {
        LOGD("Thre is nothing to point the EntryPoint\n");
        return NULL;
    }

    id = 0;
    len = 0;
    buf_size = 0;
    max_move = MAX_MOVE;
    stream_buf = NULL;
    stream_buf_list = NULL;

    fd = entry_point->fd;
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

    LOGD("id:%d entry_point id:%d\n",  id, entry_point->id);
    if (id != entry_point->id) {
        LOGD("the entry id was wrong\n");
        return NULL;
    }

    do {
        colum = (field_mask >> (4 * max_move)) & COLUM_FLAG;
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
                buf_size += sizeof(int);
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
        max_move--;
    } while (colum);

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
