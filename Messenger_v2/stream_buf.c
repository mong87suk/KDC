struct _Stream_Buf {
    char *buf;
    int len;
    int used_size;
};

Str_Buf* new_stream_buf(int len) {
    Stream_Buf *stream_buf;

    if (len < 0) {
        printf("Can't make Stream_Buf\n");
        return NULL;
    }

    stream_buf = (Stream_Buf*) malloc(sizeof(Stream_Buf));
    stream_buf->buf = (char*) malloc(len);
    stream_buf->used_size = 0;

    if (!stream_buf->buf) {
        printf("Failed to make buf\n");
        return NULL;;
    }

    return stream_buf;
}
