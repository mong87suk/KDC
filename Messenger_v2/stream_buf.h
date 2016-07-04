#ifndef __STREAM_BUF_H__
#define __STREAM_BUF_H__

#include "m_boolean.h"

#define MAX_BUF_LEN 10

typedef struct _Stream_Buf Stream_Buf;

Stream_Buf* new_stream_buf(int len);
void destroy_stream_buf(Stream_Buf *stream_buf);
BOOLEAN stream_buf_increase_pos(Stream_Buf *stream_buf, int n_byte);
char* stream_buf_get_available(Stream_Buf *stream_buf);
int stream_buf_get_available_size(Stream_Buf *stream_buf);
int stream_buf_get_position(Stream_Buf *stream_buf);
char* stream_buf_get_buf(Stream_Buf *stream_buf);

#endif
