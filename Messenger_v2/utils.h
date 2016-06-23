#ifndef __M_LOG_H__
#define __M_LOG_H__

#include "message.h"
#include "DBLinkedList.h"
#include "stream_buf.h"

#define LOGD(fmt, ...) printf("[DEBUG]\t%s:%d %s " fmt, __FILE__, __LINE__, __func__, ## __VA_ARGS__)

void print_mesg(Message *mesg);
int read_n_byte(int fd, void *buf, int size);
int write_n_byte(int fd, void *buf, int size);
int utils_get_count_to_move_flag(int field_mask);
int utils_append_data_to_buf(DList *stream_buf_list, Stream_Buf *stream_buf);
void utils_destroy_stream_buf_list(DList *stream_buf_list);
char* utils_create_path(char *name, char *file_name);
int utils_open(char *path);

#endif
