#ifndef __ENTRY_POINT_H__
#define __ENTRY_POINT_H__

#include "stream_buf.h"
#include "DBLinkedList.h"

#define MAX_MOVE    15
#define COLUM_FLAG  (int) 0xf

typedef struct _EntryPoint EntryPoint;

EntryPoint* new_entry_point(int id, int fd, int offset, int field_mask);
void destroy_entry_point(EntryPoint *entry_point);
int get_entry_point_size();
int set_value(EntryPoint *entry_point, char *buf);
int get_entry_point_id(EntryPoint *entry_point);
Stream_Buf* get_value(EntryPoint *entry_point);

#endif
