#include <stdio.h>
#include <stdlib.h>

#include "entry_point.h"
#include "utils.h"
#include "m_boolean.h"

struct _Entry_Point {
    int fd;
    int offset;
}

Entry_Point* new_entry_point() {
    Entry_Point *entry_point;

    entry_point = (Entry_Point*) malloc(sizeof(Entry_Point));
    
    if (!entry_point) {
        LOGD("Failed to make the Entry_Point\n");
        return NULL;
    }

    return entry_point;
}

void destroy_entry_point(Entry_Point *entry_point) {
    if (!entry_point) {
        LOGD("Failed to destroy the entry_point\n");
        return;
    }
    free(entry_point);
}
    
int get_entry_point_size() {
    return sizeof(Entry_Point);
}

int set_offset(Entry_Point *entry_point, int offset) {
    if (!entry_point) {
        LOGD("There is nothing to point the entry_point\n");
        return FALSE;
    }

    entry_point->offset = offset;

    return TRUE;
}

int set_id(Entry_Point *entry_point, int id) {
    if (!entry_point) {
        LOGD("There is nothing to point the entry_point\n");
        return FALSE;
    }

    entry_point->id = id;

    return TRUE;
}

int set_value(Entry_Point *entry_point, int fd, int field_mask, char *buf) {
    int flag = 0xf;
    int max_move = 15;
    char colum;
    int len, n_byte;

    do {
        colum = (field_mask >> (4 * max_colum)) & flag;
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

            default:
                LOGD("field mask was wrong\n");
                return FALSE;
        }

        max_move--;
    } while (colum);

    return TRUE;
}
