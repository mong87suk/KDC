#ifndef __INDEX_FILE_H__
#define __INDEX_FILE_H__

#include "entry_point.h"
#include "database.h"

#define INDEXFILE "index_file"

typedef struct _IndexFile IndexFile;

IndexFile* index_file_open(char *name, int field_mask, DataBase *database);
int get_index_file_end_offset(IndexFile *index_file);
void index_file_close(IndexFile *index_file);
int create_entry_point_id(IndexFile *index_file);
int get_last_id(IndexFile *index_file);
int set_last_id(IndexFile *index_file, int last_id);
int add_entry_point(IndexFile *index_file, EntryPoint *entry_point);
int update_index_file(IndexFile *index_file);
int get_count(IndexFile *index_file);
EntryPoint* find_entry_point(IndexFile *index_file, int entry_point_id);
void delete_entry_point(IndexFile *index_file, EntryPoint *entry_point);
DList* get_list(IndexFile *index_file);
#endif
