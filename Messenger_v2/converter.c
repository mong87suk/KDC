#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "converter.h"
#include "packet.h"

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

    if (!header || !tail) {
        printf("%s %s Can't convert packet to binary\n", __FILE__, __func__);
        return CONVERT_FAILURE;
    }

    memcpy(buf, header, HEADER_SIZE);
    
    if (payload_len) {
        payload = get_payload(packet);
        memcpy(buf + HEADER_SIZE, payload, payload_len);
    }

    memcpy(buf + HEADER_SIZE + payload_len, tail, TAIL_SIZE);
    return CONVERT_SUCUESS;
}
