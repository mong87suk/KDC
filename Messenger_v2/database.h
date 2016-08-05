#ifndef __DATABASE_H__
#define __DATABASE_H__

#include <kdc/DBLinkedList.h>

#include "mesg_type.h"
#include "stream_buf.h"
#include "m_boolean.h"
#include "stream_buf.h"
#include "data_file.h"

#define ID_SIZE                  4
#define LEN_SIZE                 4
#define NUM_SIZE                 4
#define FIELD_SIZE               4
#define FIELD_I_SIZE             4
#define FIELD_NUM                8
#define NO_KEY_COLUMN           -1

#define INTEGER_FIELD   0x00000001
#define KEYWORD_FIELD   0x00000002
#define STRING_FIELD    0x00000004

typedef struct _Where Where;
DataBase *database_open(char *name, char *filed_mask);
void database_close(DataBase *database);
void database_delete_all(DataBase *database);
int database_add_entry(DataBase *database, Stream_Buf *entry, int id);
int database_get_entry_count(DataBase *database);
Stream_Buf *database_get_entry(DataBase *database, int id);
BOOLEAN database_delete_entry(DataBase *database, int id);
DList *database_get_entry_list(DataBase *database);
int database_update_entry(DataBase *database, EntryPoint *entry_point, Stream_Buf *entry);
int database_get_field_mask(DataBase *database);
EntryPoint *database_find_entry_point(DataBase *database, int id);
int database_get_data_file_fd(DataBase *database);
EntryPoint *database_nth_entry_point(DataBase *database, int nth);
DList *database_search(DataBase *database, DList *where_list);
Where *new_where(int column,void *data);
void destroy_where(void *where);
void destroy_where_list(DList *where_list);
void destroy_matched_list(DList *matched_entry);
DataFile *database_get_datafile(DataBase *database);
int database_get_last_id(DataBase *database);
#endif
