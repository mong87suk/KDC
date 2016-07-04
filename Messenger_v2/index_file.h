#ifndef __INDEX_FILE_H__
#define __INDEX_FILE_H__

#include "mesg_type.h"
#include "database.h"

#define INDEXFILE "index_file"

IndexFile* index_file_open(char *name, int field_mask, DataBase *database);
void index_file_close(IndexFile *index_file);
void index_file_delete_all(IndexFile *index_file);
int index_file_get_last_id(IndexFile *index_file);
BOOLEAN index_file_set_last_id(IndexFile *index_file, int last_id);
BOOLEAN index_file_add_entry(IndexFile *index_file, EntryPoint *entry_point);
BOOLEAN index_file_update(IndexFile *index_file);
int index_file_get_count(IndexFile *index_file);
EntryPoint* index_file_find_entry(IndexFile *index_file, int id);
void index_file_delete_entry(IndexFile *index_file, EntryPoint *entry_point);
DList* index_file_get_list(IndexFile *index_file);
#endif
