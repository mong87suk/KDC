#ifndef __DATABASE_H__
#define __DATABASE_H__

#include "DBLinkedList.h"
#include "stream_buf.h"

#define INTEGER_FIELD (int) 0x10000000
#define STRING_FIELD  (int) 0x20000000

typedef struct _DataBase DataBase;

DataBase* new_database(char *file_name, char *filed_mask);
void destroy_database(DataBase *database);
int add_entry(DataBase *database, char *buf);
int get_entry_point_count(DataBase *database);
Stream_Buf* get_entry(DataBase *database, int id);
int delete_entry(DataBase *database, int entry_point_id);
DList* get_entry_point_list(DataBase *database);
int update_entry(DataBase *database, int id, int colum, char *field);
Stream_Buf* get_entry(DataBase *database, int entry_point_id);

#endif
