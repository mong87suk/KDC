#ifndef __INDEX_FILE_H__
#define __INDEX_FILE_H__

#define INDEXFILE "index_file"

#include "entry_point.h"

typedef struct _IndexFile IndexFile;

IndexFile* new_index_file(char *index_file_name, int field_mask);
int get_index_file_end_offset(IndexFile *index_file);
int set_index_info(IndexFile *index_file);
void destroy_index_file(IndexFile *index_file);
int create_entry_point_id(IndexFile *index_file);
int get_last_id(IndexFile *index_file);
int set_last_id(IndexFile *index_file, int last_id);
int add_entry_point(IndexFile *index_file, EntryPoint *entry_point);
int update_index_file(IndexFile *index_file, int field_mask);
int get_count(IndexFile *index_file);
EntryPoint* find_entry_point(IndexFile *index_file, int entry_point_id);
void delete_entry_point(IndexFile *index_file, EntryPoint *entry_point);
EntryPoint* get_list(IndexFile *index_file);
#endif
