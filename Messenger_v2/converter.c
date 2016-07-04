#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "converter.h"
#include "packet.h"
#include "m_boolean.h"
#include "utils.h"
#include "message.h"

static void converter_print_buf(char *buf, int len) {
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

static void converter_print_packet(Packet *packet) {
    char sop, eop;
    short op_code, checksum;
    long int payload_len;
    char *payload, *pos;
    Header *header;
    Body *body;
    Tail *tail;
    int i;

    header = packet_get_header(packet);
    if (!header) {
        LOGD("Failed to get header\n");
        return;
    }

    tail = packet_get_tail(packet);
    if (!tail) {
        LOGD("Failed to get tail\n");
        return;
    }

    LOGD("Print Header\n\n");
    sop = packet_get_sop(NULL, header);
    printf("0x%02X ", (unsigned char) sop);
    op_code = packet_get_op_code(NULL, header);
    pos = (char*) &op_code;
    for (i = 0; i < sizeof(op_code); i++) {
        printf("0x%02X ", (unsigned char)*(pos + i));
    }
    payload_len = packet_get_payload_len(NULL, header);
    pos = (char*) &payload_len;
    for (i = 0; i < sizeof(payload_len); i++) {
        printf("0x%02X ", (unsigned char)*(pos + i));
    }
    printf("\n\n");

    if (payload_len > 0) {
        LOGD("Print Body\n");
        body = packet_get_body(packet);
        if (!body) {
            LOGD("Failed to get the body\n");
            return;
        }
        payload = packet_get_payload(NULL, body);

        converter_print_buf(payload, payload_len);
    }

    LOGD("Print Tail\n\n");
    eop = packet_get_eop(NULL, tail);
    pos = (char*) &eop;
    for (i = 0; i < sizeof(eop); i++) {
        printf("0x%02X ", (unsigned char)(*(pos + i)));
    }
    checksum = packet_get_checksum(NULL, tail);
    pos = (char*) &checksum;
    for (i = 0; i < sizeof(checksum); i++) {
        printf("0x%02X ", (unsigned char)(*(pos + i)));
    }
    printf("\n\n");
}

Packet* convert_buf_to_packet(char *buf) {
    Packet *packet;
    Header *header;
    Body *body;
    Tail *tail;
    char sop, eop;
    short op_code, checksum;
    long int payload_len;
    char *payload;
    BOOLEAN result;
    int packet_size;
    if (!buf) {
        LOGD("Can't convert the buf to the packet\n");
        return NULL;
    }

    packet_size = packet_get_size();
    checksum = 0;
    packet = (Packet*) malloc(packet_size);
    if (!packet) {
        LOGD("Failed to make the Packet\n");
        return NULL;
    }
    memset(packet, 0, packet_size);

    header = packet_new_header(0, 0, 0);
    if (!header) {
        LOGD("Faield to make the Header\n");
        destroy_packet(packet);
        return NULL;
    }

    result = packet_set_header(packet, header);
    if (result == FALSE) {
        LOGD("Failed to set the header\n");
        destroy_packet(packet);
        return NULL;
    }

    tail = packet_new_tail(0, 0);
    if (!tail) {
        LOGD("Failed to make the Tail\n");
        destroy_packet(packet);
        return NULL;
    }

    result = packet_set_tail(packet, tail);
    if (result == FALSE) {
        LOGD("Failed to set the tail\n");
        destroy_packet(packet);
        return NULL;
    }

    sop = buf[0];
    result = packet_set_sop(packet, sop);
    buf += sizeof(sop);

    memcpy(&op_code, buf, sizeof(op_code));
    result = packet_set_op_code(packet, op_code);
    buf += sizeof(op_code);

    memcpy(&payload_len, buf, sizeof(payload_len));
    result = packet_set_payload_len(packet, payload_len);
    buf += sizeof(payload_len);

    LOGD("payload_len:%ld\n", payload_len);
    if (payload_len > 0) {
        payload = (char*) malloc(payload_len);
        if (!payload) {
            LOGD("Failed to make the payload buf\n");
            return FALSE;
        }
        memcpy(payload, buf, payload_len);
        body = packet_new_body(payload);
        result = packet_set_body(packet, body);
        if (result == FALSE) {
            LOGD("Failed to set the Body\n");
            return FALSE;
        }

        result = packet_set_payload(packet, payload);
        if (result == FALSE) {
            LOGD("Failed to set the payload\n");
            return FALSE;
        }
        buf += payload_len;
    }

    memcpy(&eop, buf, sizeof(eop));
    result = packet_set_eop(packet, eop);
    if (result == FALSE) {
        LOGD("Failed to set the eop\n");
        return FALSE;
    }
    buf += sizeof(eop);

    memcpy(&checksum, buf, sizeof(checksum));
    LOGD("chekc_sum:%d\n", checksum);
    result = packet_set_checksum(packet, checksum);
    if (result == FALSE) {
        LOGD("Failed to set the check_sum\n");
        return FALSE;
    }

    converter_print_packet(packet);
    return packet;
}

BOOLEAN convert_packet_to_buf(Packet *packet, char *buf) {
    long int payload_len;
    Header *header;
    Tail *tail;
    char *payload, *tmp_dest;
    char sop, eop;
    short op_code, checksum;

    payload_len = 0;

    if (!packet || !buf) {
        LOGD("Can't convert packet to buf\n");
        return FALSE;
    }
    tmp_dest = buf;
    header = packet_get_header(packet);
    tail = packet_get_tail(packet);
    payload_len = packet_get_payload_len(packet, NULL);
    if (!header || !tail) {
        LOGD("Can't convert packet to binary\n");
        return FALSE;
    }

    LOGD("Start to copy Header\n");
    sop = packet_get_sop(packet, NULL);
    memcpy(tmp_dest, &sop, sizeof(sop));

    op_code = packet_get_op_code(packet, NULL);
    tmp_dest += sizeof(sop);
    memcpy(tmp_dest, &op_code, sizeof(op_code));

    payload_len = packet_get_payload_len(packet, NULL);
    tmp_dest += sizeof(op_code);
    memcpy(tmp_dest, &payload_len, sizeof(payload_len));
    converter_print_buf(buf, HEADER_SIZE);
    LOGD("Finished to copy Header\n");
    tmp_dest += sizeof(payload_len);

    if (payload_len) {
        LOGD("Strart to copy Body\n");
        payload = packet_get_payload(packet, NULL);
        memcpy(tmp_dest, payload, payload_len);

        converter_print_buf(buf + HEADER_SIZE, payload_len);
        tmp_dest += payload_len;
        LOGD("Finished to copy Body\n");
    }

    LOGD("Start to copy Tail\n");
    eop = packet_get_eop(packet, NULL);
    memcpy(tmp_dest, &eop, sizeof(eop));

    tmp_dest += sizeof(eop);
    checksum = packet_get_checksum(packet, NULL);
    memcpy(tmp_dest, &checksum, sizeof(checksum));
    converter_print_buf(buf + HEADER_SIZE + payload_len, TAIL_SIZE);
    LOGD("Finished to copy Tail\n\n");
    return TRUE;
}

Message* convert_payload_to_mesgs(char *payload, int *mesg_num) {
    int n, i;
    int mesgs_size;
    int mesg_size;
    int mesg_len;

    Message *mesgs;
    Message *mesg, *tmp;

    if (!payload) {
        LOGD("There is nothing to point the Payload\n");
        return NULL;
    }

    memcpy(&n, payload, sizeof(n));
    LOGD("Message number: %d\n", n);
    payload += sizeof(n);

    mesg_size = message_get_size();
    mesgs_size = n * mesg_size;
    mesgs = (Message*) malloc(mesgs_size);
    tmp = mesgs;

    for (i = 0; i < n; i++) {
        mesgs = message_next(tmp, i);
        mesg = convert_payload_to_mesg(payload, &mesg_len);
        if (!mesg) {
            continue;
        }
        memcpy(mesgs, mesg, mesg_size);
        payload += mesg_len;
    }

    *mesg_num = n;
    return tmp;
}

Message* convert_payload_to_mesg(char *payload, int *mesg_len) {
    long int time;
    int len, mesg_size;
    char *str;
    Message *mesg;

    if (!payload) {
        LOGD("Can't convert payload to Message\n");
        return NULL;
    }
    mesg_size = message_get_size();
    mesg = (Message*) malloc(message_get_size());
    if (!mesg) {
        LOGD("Failed to make message \n");
        return NULL;
    }
    memset(mesg, 0, mesg_size);

    memcpy(&time, payload, sizeof(time));
    payload += sizeof(time);
    message_set_time(mesg, time);

    memcpy(&len, payload, sizeof(len));
    payload += sizeof(len);
    message_set_str_len(mesg, len);

    str = (char*) malloc(len);
    if (!str) {
        LOGD("Failed to make the str\n");
        return NULL;
    }
    memcpy(str, payload, len);
    message_set_str(mesg, str);

    if (mesg_len) {
        *mesg_len = sizeof(time) + sizeof(len) + len;
    }

    return mesg;
}

BOOLEAN convert_mesgs_to_payload(Message *mesgs, char *payload, int len) {
    long int time;
    int str_len, i;
    char *str, *tmp;
    Message *mesg;

    if (!mesgs || !payload) {
        LOGD("Can't convert mesgs to buf\n");
        return FALSE;
    }
    str_len = 0;
    tmp = payload;
    memcpy(tmp, &len, sizeof(len));
    converter_print_buf(tmp, sizeof(len));
    tmp += sizeof(len);

    for (i = 0; i < len; i++) {
        mesg = message_next(mesgs, i);
        if (!mesg) {
            LOGD("There is nothing to point the mesg\n");
            return FALSE;
        }

        if (str_len) {
            tmp += str_len;
        }

        time = message_get_time(mesg);
        if (time == -1) {
            LOGD("Failed to get the time\n");
            return FALSE;
        }
        memcpy(tmp, &time, sizeof(time));
        converter_print_buf(tmp, sizeof(time));
        tmp += sizeof(time);

        str_len = message_get_str_len(mesg);
        if (str_len == -1) {
            LOGD("Failed to get the str_len\n");
            return FALSE;
        }
        memcpy(tmp, &str_len, sizeof(str_len));
        converter_print_buf(tmp, sizeof(str_len));
        tmp += sizeof(str_len);

        str = message_get_str(mesg);
        if (!str) {
            LOGD("Faield to get the str\n");
            return FALSE;
        }
        memcpy(tmp, str, str_len);
        converter_print_buf(tmp, str_len);
    }

    return TRUE;
}
