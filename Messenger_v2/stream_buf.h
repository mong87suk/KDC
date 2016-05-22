#ifndef __STREAM_BUF_H__
#define __STREAM_BUF_H__

typedef struct _Stream_Buf Stream_Buf;

Stream_Buf* new_stream_buf(int len);
void destroy_stream_buf(Stream_Buf *stream_buf);
char* get_available_buf(Stream_Buf *stream_buf);
int get_available_size(Stream_Buf *stream_buf);
void set_available_size(Stream_Buf *stream_buf, int n_byte);
int get_len(Stream_Buf *stream_buf);
int get_position(Stream_Buf *stream_buf);
void set_position(Stream_Buf *stream_buf, int n_byte);
char* get_buf(Stream_Buf *stream_buf);

#endif
