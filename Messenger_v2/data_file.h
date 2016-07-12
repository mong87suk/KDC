#ifndef __DATA_FILE_H__
#define __DATA_FILE_H__

#include "stream_buf.h"
#include "m_boolean.h"

#define DATAFILE "data_file"

typedef struct _DataFile DataFile;

DataFile *data_file_open(char *name);
void data_file_close(DataFile *data_file);
void data_file_delete_all(DataFile *data_file);
int data_file_get_offset(DataFile *data_file);
int data_file_get_fd(DataFile *data_file);
BOOLEAN data_file_write_entry(DataFile *data_file, int id, Stream_Buf *entry);
#endif
