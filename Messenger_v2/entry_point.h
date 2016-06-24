#ifndef __ENTRY_POINT_H__
#define __ENTRY_POINT_H__

#include "stream_buf.h"
#include "database.h"

#define MAX_MOVE         15
#define FIELD_TYPE_FLAG  0xf
#define FIELD_SIZE       4
#define ENTRY_POINT_SIZE  16 

EntryPoint* new_entry_point(int id, int offset, DataBase *database);
void destroy_entry_point(EntryPoint *entry_point);
int entry_point_get_size();
int entry_point_get_id(EntryPoint *entry_point);
Stream_Buf* entry_point_get_value(EntryPoint *entry_point);
int entry_point_set_offset(EntryPoint *entry_point, int offset);
int entry_point_get_offset(EntryPoint *entry_point);

#endif
