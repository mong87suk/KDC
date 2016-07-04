#ifndef __M_LOG_H__
#define __M_LOG_H__

#include "message.h"
#include "DBLinkedList.h"
#include "stream_buf.h"

#define FIELD_TYPE_FLAG  0xf
#define FIELD_SIZE       4

#define LOGD(fmt, ...) printf("[DEBUG]\t%s:%d %s " fmt, __FILE__, __LINE__, __func__, ## __VA_ARGS__)

void utils_print_mesg(Message *mesg);
int utils_read_n_byte(int fd, void *buf, int size);
int write_n_byte(int fd, void *buf, int size);
int utils_get_colum_count(int field_mask);
BOOLEAN utils_append_data_to_buf(DList *stream_buf_list, Stream_Buf *stream_buf);
void utils_destroy_stream_buf_list(DList *stream_buf_list);
char* utils_create_path(char *name, char *file_name);

#endif
