#ifndef __DATA_FILE_H__
#define __DATA_FILE_H__

#define DATAFILE "data_file"

typedef struct _DataFile DataFile;

DataFile* data_file_open(char *name);
void data_file_close(DataFile *data_file);
void data_file_delete(DataFile *data_file);
int data_file_get_offset(DataFile *data_file);
int data_file_get_fd(DataFile *data_file);

#endif
