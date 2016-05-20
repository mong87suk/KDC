#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "stream_buf.h"

struct _Stream_Buf {
    char *buf;
    char *start_position;
    int len;
    int position;
    int available_size;
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
        return NULL;
    }

    stream_buf->start_position = stream_buf->buf;
    memset(stream_buf->buf, 0, len);
    stream_buf->position = 0;
    stream_buf->len = len;
    stream_buf->available_size = len;

    return stream_buf;
}

void destroy_stream_buf(Stream_Buf *stream_buf) {
    if (!stream_buf && !stream_buf->buf) {
        printf("There is nothing to destroy Stream_buf\n");
        return;
    }

    free(stream_buf->buf);
    free(stream_buf);
}

char* get_buf(Stream_Buf *stream_buf) {
    if (!stream_buf) {
        printf("There is nothing to point Stream_Buf\n");
        return NULL;
    }
    return stream_buf->buf;
}

int get_available_size(Stream_Buf *stream_buf) {
    if (!stream_buf && !stream_buf->buf) {
        printf("There is nothing to point Stream_Buf\n");
        return 0;
    }
    return stream_buf->available_size;
}

void set_available_size(Stream_Buf *stream_buf, int n_byte) {
    if (!stream_buf && !stream_buf->buf) {
        printf("There is nothing to point Stream_Buf\n");
        return;
    }
    stream_buf->available_size -= n_byte;
}

int get_len(Stream_Buf *stream_buf) {
    if (!stream_buf) {
        printf("There is nothing to point the Stream_Buf\n");
        return 0;
    }
    return stream_buf->len;
}

int get_position(Stream_Buf *stream_buf) {
    if (!stream_buf) {
        printf("There is nothing to point the Stream_Buf\n");
        return 0;
    }
    return stream_buf->position;
}

void set_position(Stream_Buf *stream_buf, int n_byte) {
    if (!stream_buf && !stream_buf->buf) {
        printf("There is nothing to point the Stream_Buf\n");
        return;
    }
    stream_buf->position += n_byte;
    stream_buf->buf = stream_buf->buf + stream_buf->position;
}

void get_start_position(Stream_Buf *stream_buf) {
    char *buf;
    if (!stream_buf && !stream_buf->buf && get_position > 0) {
        printf("Can't set start position\n");
        return;
    }
    return buf[0];
}
