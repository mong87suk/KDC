#ifndef __INDEX_FILE_H__
#define __INDEX_FILE_H__

#define INDEXFILE "index_file"

#include "entry_point.h"

typedef struct _Index_File Index_File;

Index_File* new_index_file(char *index_file_name);
int get_index_file_end_offset(Index_File *index_file);
int set_index_info(Index_File *index_file);
void destroy_index_file(Index_File *index_file);
int create_entry_point_id(Index_File *index_file);
int get_last_id(Index_File *index_file);
int set_last_id(Index_File *index_file, int last_id);
int add_entry_point(Index_File *index_file, EntryPoint *entry_point);
int update_index_file(Index_File *index_file, int field_mask);

#endif
