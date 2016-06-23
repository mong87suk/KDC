#ifndef __DATABASE_H__
#define __DATABASE_H__

#include "DBLinkedList.h"
#include "stream_buf.h"

#define INTEGER_FIELD   0x00000001
#define STRING_FIELD    0x00000002
#define FIELD_SIZE               4

typedef struct _DataBase DataBase;

DataBase* new_database(char *name, char *filed_mask);
void destroy_database(DataBase *database);
int database_add_entry(DataBase *database, Stream_Buf *entry);
int get_entry_point_count(DataBase *database);
Stream_Buf* get_entry(DataBase *database, int id);
int delete_entry(DataBase *database, int entry_point_id);
DList* get_entry_point_list(DataBase *database);
int database_update_entry(DataBase *database, int id, int colum, Stream_Buf *field);
Stream_Buf* get_entry(DataBase *database, int entry_point_id);
int database_get_field_mask(DataBase *database);

#endif
