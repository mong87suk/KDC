#include <stdio.h>

#include "message.h"
#include "utils.h"

void print_mesg(Message *mesg) {
    long int time;
    int str_len, i;
    char *str, *pos;

    if (!mesg) {
        LOGD("Can't print the Message\n");
        return;
    }   

    LOGD("\nPrint Message\n");
    time = get_time(mesg);
    pos = (char*) &time;
    for (i = 0; i < sizeof(time); i++) {
        printf("0x%02X ", (unsigned char)*(pos + i));
    }   

    str_len = get_str_len(mesg);
    pos = (char*) &str_len;
    for (i = 0; i < sizeof(str_len); i++) {
        printf("0x%02X ", (unsigned char)*(pos + i));
    }   

    str = get_str(mesg);
    pos = str;
    for (i = 0; i < str_len; i++) {
        printf("%c ", (unsigned char) *(pos + i));
    }   
    printf("\n\n");
}

int read_n_byte(int fd, char *buf, int size) {
    int n, tmp;

    tmp = size;

    if (!buf) {
        LOGD("Can't read n byte\n");
    }

    if (size < 0 || fd < 0) {
        LOGD("Can't read n byte\n");
    }

    while (tmp > 0) {
        n = read(fd, buf, size);
        tmp -= n;
    }

    return size;
}

int write_n_byte(int fd, char *buf, int size) {
    int n, tmp;

    tmp = size;

    if (!buf) {
        LOGD("Can't read n byte\n");
    }

    if (size < 0 || fd < 0) {
        LOGD("Can't read n byte\n");
    }

    while (tmp > 0) {
        n = write(fd, buf, size);
        tmp -= n;
    }

    return size;
}   
