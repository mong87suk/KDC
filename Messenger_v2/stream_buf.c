#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "stream_buf.h"
#include "utils.h"
#include "m_boolean.h"

struct _Stream_Buf {
    char *buf;
    int len;
    int position;
};

Stream_Buf* new_stream_buf(int len) {
    Stream_Buf *stream_buf;

    if (len < 0) {
        LOGD("Can't make Stream_Buf\n");
        return NULL;
    }

    stream_buf = (Stream_Buf*) malloc(sizeof(Stream_Buf));
    stream_buf->buf = (char*) malloc(len);
    if (!stream_buf->buf) {
        LOGD("Failed to make buf\n");
        return NULL;
    }

    memset(stream_buf->buf, 0, len);
    stream_buf->position = 0;
    stream_buf->len = len;

    return stream_buf;
}

void destroy_stream_buf(Stream_Buf *stream_buf) {
    if (!stream_buf && !stream_buf->buf) {
        LOGD("There is nothing to destroy Stream_buf\n");
        return;
    }

    free(stream_buf->buf);
    free(stream_buf);
}

char* stream_buf_get_available(Stream_Buf *stream_buf) {
    if (!stream_buf && !stream_buf->buf) {
        LOGD("There is nothing to point Stream_Buf\n");
        return NULL;
    }
    return stream_buf->buf + stream_buf->position;
}

int stream_buf_get_position(Stream_Buf *stream_buf) {
    if (!stream_buf) {
        LOGD("There is nothing to point the Stream_Buf\n");
        return 0;
    }
    return stream_buf->position;
}

int stream_buf_get_available_size(Stream_Buf *stream_buf) {
    if (!stream_buf) {
        LOGD("There is nothing to point the Stream_Buf\n");
        return 0;
    }

    return stream_buf->len - stream_buf->position;
}

short stream_buf_increase_position(Stream_Buf *stream_buf, int n_byte) {
    if (!stream_buf) {
        LOGD("There is nothing to point the Stream_Buf\n");
        return FALSE;
    }
    stream_buf->position += n_byte;
    return TRUE;
}

char* stream_buf_get_buf(Stream_Buf *stream_buf) {
    if (!stream_buf && !stream_buf->buf) {
        LOGD("There is nothing to point the Stream_Buf\n");
        return NULL;
    }
    return stream_buf->buf;
}
