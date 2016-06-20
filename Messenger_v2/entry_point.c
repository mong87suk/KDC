#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "entry_point.h"
#include "utils.h"
#include "m_boolean.h"

struct _EntryPoint {
    int id;
    int fd;
    int offset;
    int field_mask;
};

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
    int flag = 0xf;
    int max_move = 15;
    char colum;
    int len, n_byte;
    int field_mask, fd;

    field_mask = entry_point->field_mask;
    fd = entry_point->fd;
    len = 0;

    do {
        colum = (field_mask >> (4 * max_move)) & flag;
        LOGD("%d\n", colum);
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
