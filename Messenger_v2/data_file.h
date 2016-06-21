#ifndef __DATA_FILE_H__
#define __DATA_FILE_H__

#define DATAFILE "data_file"

typedef struct _DataFile DataFile;

DataFile* new_data_file(char *data_file_name);
void destroy_data_file(DataFile *data_file);
int get_data_file_offset(DataFile *data_file);
int get_data_file_fd(DataFile *data_file);

#endif
