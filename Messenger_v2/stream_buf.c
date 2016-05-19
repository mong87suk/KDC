#include "stream_buf.h"

struct _Stream_Buf {
    char *buf;
    int len;
    int used_size;
};

Stream_Buf* new_stream_buf(int len) {
    Stream_Buf *stream_buf;

    if (len < 0) {
        printf("Can't make Stream_Buf\n");
        return NULL;
    }

    stream_buf = (Stream_Buf*) malloc(sizeof(Stream_Buf));
    stream_buf->buf = (char*) malloc(len);    
    if (!stream_buf->buf) {
        printf("Failed to make buf\n");
        return NULL;;
    }

    memset(stream_buf->buf, 0, len);
    stream_buf->used_size = 0;
    stream_buf->len = len;

    return stream_buf;
}

char* get_available_buf(Stream_Buf *stream_buf) {
    if (!stream_buf) {
        printf("There is nothing to point the Stream_Buf\n");
        return NULL;
    }
    stream_buf->buf = stream_buf->buf + stream_buf->used_size;
    
    return stream_buf->buf;
}

int get_available_size(Stream_Buf *stream_buf) {
    if (!stream_buf) {
        printf("There is nothing to point the Stream_Buf\n");
        return 0;
    }
    return stream_buf->len;
}

void sum_used_n_byte(Stream_Buf *stream_buf, int n_byte) {
    if (!stream_buf) {
        printf("There is nothing to point the Stream_Buf\n");
        return 0;
    }

    stream_buf->used_size += n_byte;
    return;
}
 
