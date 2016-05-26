#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "converter.h"
#include "packet.h"
#include "m_boolean.h"
#include "utils.h"

static void print_buf(char *buf, int len) {
    int i;

    if (!buf) {
        LOGD("Can't check the buf\n");
        return;
    }
    printf("\n");
    for (i = 0; i < len; i++) {
        printf("0x%02X ", (unsigned char) buf[i]);
    }
    printf("\n\n");
}

static void print_str(char *buf, int len) {
    int i;

    if (!buf) {
        LOGD("Can't check the buf\n");
        return;
    }
    printf("\n");
    for (i = 0; i < len; i++) {
        printf(" %2C", buf[i]);
    }
    printf("\n\n");
}

short convert_buf_to_packet(char *buf, Packet *packet) {
    Header *header;
    Body *body;
    Tail *tail;
    char sop, eop;
    short op_code, check_sum;
    long int payload_len;
    char *payload;
    short result;

    if (!buf || !packet) {
        printf("%s %s Can't convert the buf to the packet\n", __FILE__, __func__);
        return FALSE;
    }

    header = get_header(packet);
    if (!header) {
        header = new_header(0, 0, 0);
        result = set_header(packet, header);
        if (result == FALSE) {
            printf("Failed to set the header\n");
            return FALSE;
        }
    }

    tail = get_tail(packet);
    if (!tail) {
        tail = new_tail(0, 0);
        result = set_tail(packet, tail);
        if (result == FALSE) {
            printf("Failed to set the tail\n");
            return FALSE;
        }
    }

    memcpy(header, buf, HEADER_SIZE);
    sop = get_sop(NULL, header);
    printf("%s %s Header: ", __FILE__, __func__);
    printf("sop: %02X, ", sop);
    op_code = get_op_code(NULL, header);
    printf("op_code: %d, ", op_code);
    payload_len = get_payload_len(NULL, header);
    printf("payload_len: %ld\n", payload_len);
    buf = buf + HEADER_SIZE;

    if (payload_len > 0) {
        payload = (char*) malloc(payload_len);
        if (!payload) {
            printf("%s %s Failed to make the payload buf\n", __FILE__, __func__);
            return FALSE;
        }
        memcpy(payload, buf, payload_len);
        body = get_body(packet);
        if (!body) {
            body = new_body(payload);
        } else {
            result = set_payload(packet, payload);
            if (result == FALSE) {
                printf("Failed to set the payload\n");
                return FALSE;
            }
        }
    }
    return TRUE;
}

short convert_packet_to_buf(Packet *packet, char *buf) {
    long int payload_len;
    Header *header;
    Tail *tail;
    char *payload, *tmp_dest;
    char sop, eop;
    short op_code, check_sum;

    payload_len = 0;

    if (!packet || !buf) {
        printf("%s %s Can't convert packet to buf\n", __FILE__, __func__);
        return FALSE;
    }
    tmp_dest = buf;
    header = get_header(packet);
    tail = get_tail(packet);
    payload_len = get_payload_len(packet, NULL);
    if (!header || !tail) {
        printf("%s %s Can't convert packet to binary\n", __FILE__, __func__);
        return FALSE;
    }

    LOGD("Start to copy Header\n");
    sop = get_sop(packet, NULL);
    memcpy(tmp_dest, &sop, sizeof(sop));

    op_code = get_op_code(packet, NULL);
    tmp_dest += sizeof(sop);
    memcpy(tmp_dest, &op_code, sizeof(op_code));

    payload_len = get_payload_len(packet, NULL);
    tmp_dest += sizeof(op_code);
    memcpy(tmp_dest, &payload_len, sizeof(payload_len));
    print_buf(buf, HEADER_SIZE);
    LOGD("Finished to copy Header\n");

    tmp_dest += sizeof(payload_len);
    if (payload_len) {
        LOGD("Strart to copy Body\n");
        payload = get_payload(packet, NULL);
        memcpy(tmp_dest, payload, payload_len);

        print_str(buf + HEADER_SIZE, payload_len);
        tmp_dest += payload_len;
        LOGD("Finished to copy Body\n");
    }

    LOGD("Start to copy Tail\n");
    eop = get_eop(packet, NULL);
    memcpy(tmp_dest, &eop, sizeof(eop));

    tmp_dest += sizeof(eop);
    check_sum = get_check_sum(packet, NULL);
    memcpy(tmp_dest, &check_sum, sizeof(check_sum));
    print_buf(buf + HEADER_SIZE + payload_len, TAIL_SIZE);
    
    LOGD("Finished to copy Tail\n");
    return TRUE;
}
