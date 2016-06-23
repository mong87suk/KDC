#ifndef __ENTRY_POINT_H__
#define __ENTRY_POINT_H__

#include "stream_buf.h"
#include "DBLinkedList.h"

#define MAX_MOVE         15
#define FIELD_TYPE_FLAG  0xf
#define FIELD_SIZE       4

typedef struct _EntryPoint EntryPoint;

EntryPoint* new_entry_point(int id, int offset, int field_mask);
void destroy_entry_point(EntryPoint *entry_point);
int get_entry_point_size();
int get_entry_point_id(EntryPoint *entry_point);
Stream_Buf* entry_point_get_value(EntryPoint *entry_point, int fd);
int entry_point_set_offset(EntryPoint *entry_point, int offset);

#endif
