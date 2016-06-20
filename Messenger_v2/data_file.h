#ifndef __DATA_FILE_H__
#define __DATA_FILE_H__

#define DATAFILE "data_file"

typedef struct _Data_File Data_File;

Data_File* new_data_file(char *data_file_name);
void destroy_data_file(Data_File *data_file);
int get_data_file_offset(Data_File *data_file);
int get_data_file_fd(Data_File *data_file);

#endif
