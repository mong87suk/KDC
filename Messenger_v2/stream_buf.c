#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "stream_buf.h"

struct _Stream_Buf {
    char *buf;
    int len;
    int position;
    int available_size;
};

Stream_Buf* new_stream_buf(int len) {
    Stream_Buf *stream_buf;

    if (len < 0) {
        printf("%s %s Can't make Stream_Buf\n", __FILE__, __func__);
        return NULL;
    }

    stream_buf = (Stream_Buf*) malloc(sizeof(Stream_Buf));
    stream_buf->buf = (char*) malloc(len);
    if (!stream_buf->buf) {
        printf("%s %s Failed to make buf\n", __FILE__, __func__);
        return NULL;
    }

    memset(stream_buf->buf, 0, len);
    stream_buf->position = 0;
    stream_buf->len = len;
    stream_buf->available_size = len;

    return stream_buf;
}

void destroy_stream_buf(Stream_Buf *stream_buf) {
    if (!stream_buf && !stream_buf->buf) {
        printf("%s %s There is nothing to destroy Stream_buf\n", __FILE__, __func__);
        return;
    }

    free(stream_buf->buf);
    free(stream_buf);
}

char* get_available_buf(Stream_Buf *stream_buf) {
    if (!stream_buf && !stream_buf->buf) {
        printf("%s %s There is nothing to point Stream_Buf\n", __FILE__, __func__);
        return NULL;
    }
    return stream_buf->buf + stream_buf->position;
}

int get_available_size(Stream_Buf *stream_buf) {
    if (!stream_buf && !stream_buf->buf) {
        printf("%s %s There is nothing to point Stream_Buf\n", __FILE__, __func__);
        return 0;
    }
    return stream_buf->available_size;
}

STREAM_BUF_SET_VAULE_RESULT set_available_size(Stream_Buf *stream_buf, int n_byte) {
    if (!stream_buf && !stream_buf->buf) {
        printf("%s %s There is nothing to point Stream_Buf\n", __FILE__, __func__);
        return STREAM_BUF_SET_VALUE_FAILURE;
    }
    stream_buf->available_size -= n_byte;

    return STREAM_BUF_SET_VAULE_SUCCESS;
}

int get_len(Stream_Buf *stream_buf) {
    if (!stream_buf) {
        printf("%s %s There is nothing to point the Stream_Buf\n", __FILE__, __func__);
        return 0;
    }
    return stream_buf->len;
}

int get_position(Stream_Buf *stream_buf) {
    if (!stream_buf) {
        printf("%s %s There is nothing to point the Stream_Buf\n", __FILE__, __func__);
        return 0;
    }
    return stream_buf->position;
}

STREAM_BUF_SET_VAULE_RESULT set_position(Stream_Buf *stream_buf, int n_byte) {
    if (!stream_buf) {
        printf("%s %s There is nothing to point the Stream_Buf\n", __FILE__, __func__);
        return STREAM_BUF_SET_VALUE_FAILURE;
    }
    stream_buf->position += n_byte;
    return STREAM_BUF_SET_VAULE_SUCCESS;
}

char* get_buf(Stream_Buf *stream_buf) {
    if (!stream_buf && !stream_buf->buf) {
        printf("%s %s There is nothing to point the Stream_Buf\n", __FILE__, __func__);
        return NULL;
    }
    return stream_buf->buf;
}
