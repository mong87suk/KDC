#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "converter.h"
#include "packet.h"

static void check_buf(char *buf) {
    Header *header;
    Tail *tail;
    char sop, eop;
    short op_code, check_sum;
    long int payload_len;
    int i;

    if (!buf) {
        printf("%s %s Can't check the buf\n", __FILE__, __func__);
        return;
    }

    header = new_header(0, 0, 0);
    memcpy(header, buf, HEADER_SIZE);
    sop = get_sop(NULL, header);
    printf("%s %s Header: ", __FILE__, __func__);
    printf("sop: %02X, ", sop);
    op_code = get_op_code(NULL, header);
    printf("op_code: %d, ", op_code);
    payload_len = get_payload_len(NULL, header);
    printf("payload_len: %ld\n", payload_len);
    printf("%s %s Body: ", __FILE__, __func__);
    buf = buf + HEADER_SIZE;

    if (payload_len > 0) {
        for (i = 0; i< payload_len; i++) {
            printf("%c", buf[i]);
        }
        buf = buf + payload_len;
    }

    printf("\n");
    printf("%s %s Tail: ", __FILE__, __func__);
    tail = new_tail(0, 0);
    memcpy(tail, buf, TAIL_SIZE);
    eop = get_eop(NULL, tail);
    printf("eop: %02X, ", eop);
    check_sum = get_check_sum(NULL, tail);
    printf("check_sum: %02X\n", check_sum);
}

CONVERT_RESULT convert_packet_to_buf(Packet *packet, char *buf) {
    long int payload_len;
    Header *header;
    Tail *tail;
    char *payload;

    payload_len = 0;

    if (!packet || !buf) {
        printf("%s %s Can't convert packet to buf\n", __FILE__, __func__);
        return CONVERT_FAILURE;
    }

    header = get_header(packet);
    tail = get_tail(packet);
    payload_len = get_payload_len(packet, NULL);
    if (!header || !tail) {
        printf("%s %s Can't convert packet to binary\n", __FILE__, __func__);
        return CONVERT_FAILURE;
    }

    memcpy(buf, header, HEADER_SIZE);
    
    if (payload_len) {
        payload = get_payload(packet, NULL);
        memcpy(buf + HEADER_SIZE, payload, payload_len);
    }

    memcpy(buf + HEADER_SIZE + payload_len, tail, TAIL_SIZE);
    check_buf(buf);
    return CONVERT_SUCUESS;
}