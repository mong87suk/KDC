#ifndef __STREAM_BUF_H__
#define __STREAM_BUF_H__

typedef enum {
    STREAM_BUF_SET_VAULE_SUCCESS = 1,
    STREAM_BUF_SET_VALUE_FAILURE = -1
} STREAM_BUF_SET_VAULE_RESULT;

typedef struct _Stream_Buf Stream_Buf;

Stream_Buf* new_stream_buf(int len);
void destroy_stream_buf(Stream_Buf *stream_buf);
char* get_available_buf(Stream_Buf *stream_buf);
int get_available_size(Stream_Buf *stream_buf);
int get_len(Stream_Buf *stream_buf);
int get_position(Stream_Buf *stream_buf);
char* get_buf(Stream_Buf *stream_buf);
STREAM_BUF_SET_VAULE_RESULT set_available_size(Stream_Buf *stream_buf, int n_byte);
STREAM_BUF_SET_VAULE_RESULT set_position(Stream_Buf *stream_buf, int n_byte);

#endif
