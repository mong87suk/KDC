#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "converter.h"
#include "packet.h"
#include "m_boolean.h"
#include "utils.h"
#include "message.h"

static void print_mesg(Message *mesg) {
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

static void print_payload(char *buf, int len) {
    int i;

    if (!buf) {
        LOGD("Can't check the buf\n");
        return;
    }
    printf("\n");

    for (i = 0; i < sizeof(long int); i++) {
        printf("0x%02X ", (unsigned char) buf[i]);
    }

    len -= sizeof(long int);
    buf += sizeof(long int);

    for (i = 0; i < sizeof(int); i++) {
        printf("0x%02X ", (unsigned char) buf[i]);
    }

    len -= sizeof(int);
    buf += sizeof(int);

    for (i = 0; i < len; i++) {
        printf(" %2C", (unsigned char) buf[i]);
    }
    printf("\n\n");
}

static void print_packet(Packet *packet) {
    char sop, eop;
    short op_code, check_sum;
    long int payload_len;
    char *payload, *pos;
    Header *header;
    Body *body;
    Tail *tail;
    int i;

    header = get_header(packet);
    if (!header) {
        LOGD("Failed to get header\n");
        return;
    }

    tail = get_tail(packet);
    if (!tail) {
        LOGD("Failed to get tail\n");
        return;
    }

    LOGD("Print Header\n\n");
    sop = get_sop(NULL, header);
    printf("0x%02X ", (unsigned char) sop);
    op_code = get_op_code(NULL, header);
    pos = (char*) &op_code;
    for (i = 0; i < sizeof(op_code); i++) {
        printf("0x%02X ", (unsigned char)*(pos + i));
    }
    payload_len = get_payload_len(NULL, header);
    pos = (char*) &payload_len;
    for (i = 0; i < sizeof(payload_len); i++) {
        printf("0x%02X ", (unsigned char)*(pos + i));
    }
    printf("\n\n");

    if (payload_len > 0) {
        LOGD("Print Body\n");
        body = get_body(packet);
        if (!body) {
            LOGD("Failed to get the body\n");
            return;
        }
        payload = get_payload(NULL, body);

        print_payload(payload, payload_len);
    }

    LOGD("Print Tail\n\n");
    eop = get_eop(NULL, tail);
    pos = (char*) &eop;
    for (i = 0; i < sizeof(eop); i++) {
        printf("0x%02X ", (unsigned char)(*(pos + i)));
    }
    check_sum = get_check_sum(NULL, tail);
    pos = (char*) &check_sum;
    for (i = 0; i < sizeof(check_sum); i++) {
        printf("0x%02X ", (unsigned char)(*(pos + i)));
    }
    printf("\n\n");
}

int convert_buf_to_packet(char *buf, Packet *packet) {
    Header *header;
    Body *body;
    Tail *tail;
    char sop, eop;
    short op_code, check_sum;
    long int payload_len;
    char *payload;
    short result;

    if (!buf || !packet) {
        LOGD("Can't convert the buf to the packet\n");
        return FALSE;
    }

    header = new_header(0, 0, 0);
    if (!header) {
        LOGD("Faield to make the Header\n");
        return FALSE;
    }

    result = set_header(packet, header);
    if (result == FALSE) {
        LOGD("Failed to set the header\n");
        return FALSE;
    }

    tail = new_tail(0, 0);
    if (!tail) {
        LOGD("Failed to make the Tail\n");
        return FALSE;
    }

    result = set_tail(packet, tail);
    if (result == FALSE) {
        LOGD("Failed to set the tail\n");
        return FALSE;
    }

    sop = buf[0];
    result = set_sop(packet, sop);
    buf += sizeof(sop);

    memcpy(&op_code, buf, sizeof(op_code));
    result = set_op_code(packet, op_code);
    buf += sizeof(op_code);

    memcpy(&payload_len, buf, sizeof(payload_len));
    result = set_payload_len(packet, payload_len);
    buf += sizeof(payload_len);

    if (payload_len > 0) {
        payload = (char*) malloc(payload_len);
        if (!payload) {
            LOGD("Failed to make the payload buf\n");
            return FALSE;
        }
        memcpy(payload, buf, payload_len);
        body = new_body(payload);
        result = set_body(packet, body);
        if (result == FALSE) {
            LOGD("Failed to set the Body\n");
            return FALSE;
        }

        result = set_payload(packet, payload);
        if (result == FALSE) {
            LOGD("Failed to set the payload\n");
            return FALSE;
        }
        buf += payload_len;
    }

    memcpy(&eop, buf, sizeof(eop));
    result = set_eop(packet, eop);
    if (result == FALSE) {
        LOGD("Failed to set the eop\n");
        return FALSE;
    }
    buf += sizeof(eop);

    memcpy(&check_sum, buf, sizeof(check_sum));
    result = set_check_sum(packet, check_sum);
    if (result == FALSE) {
        LOGD("Failed to set the check_sum\n");
        return FALSE;
    }

    print_packet(packet);
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

        print_payload(buf + HEADER_SIZE, payload_len);
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
    LOGD("Finished to copy Tail\n\n");
    return TRUE;
}

int convert_payload_to_mesg(char *payload, Message *mesg) {
    long int time;
    int len;
    char *str;

    if (!payload || !mesg) {
        LOGD("Can't convert payload to Message\n");
        return FALSE;
    }

    memcpy(&time, payload, sizeof(time));
    payload += sizeof(time);
    set_time(mesg, time);

    memcpy(&len, payload, sizeof(len));
    payload += sizeof(len);
    set_str_len(mesg, len);

    str = (char*) malloc(len);
    if (!str) {
        LOGD("Failed to make the str\n");
        return FALSE;
    }
    memcpy(str, payload, len);
    set_str(mesg, str);

    print_mesg(mesg);
    return TRUE;
}

int convert_mesgs_to_payload(Message *mesgs, char *payload, int len) {
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
    tmp += sizeof(len);

    for (i = 0; i < len; i++) {
        mesg = next_mesg(mesgs, i);
        if (!mesg) {
            LOGD("There is nothing to point the mesg\n");
            return FALSE;
        }

        if (!str_len) {
            tmp += str_len;
        }

        time = get_time(mesg);
        if (time == -1) {
            LOGD("Failed to get the time\n");
            return FALSE;
        }
        memcpy(tmp, &time, sizeof(time));
        tmp += sizeof(time);

        str_len = get_str_len(mesg);
        if (str_len == -1) {
            LOGD("Failed to get the str_len\n");
            return FALSE;
        }
        memcpy(tmp, &str_len, sizeof(str_len));
        tmp += sizeof(str_len);

        str = get_str(mesg);
        if (!str) {
            LOGD("Faield to get the str\n");
            return FALSE;
        }
        memcpy(tmp, str, str_len);
    }

    return TRUE;
}
