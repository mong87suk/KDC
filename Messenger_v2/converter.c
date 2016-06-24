#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "converter.h"
#include "packet.h"
#include "m_boolean.h"
#include "utils.h"
#include "message.h"

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

        print_buf(payload, payload_len);
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

Packet* convert_buf_to_packet(char *buf) {
    Packet *packet;
    Header *header;
    Body *body;
    Tail *tail;
    char sop, eop;
    short op_code, check_sum;
    long int payload_len;
    char *payload;
    short result;
    int packet_size;
    if (!buf) {
        LOGD("Can't convert the buf to the packet\n");
        return NULL;
    }

    packet_size = get_packet_size();
    check_sum = 0;
    packet = (Packet*) malloc(packet_size);
    if (!packet) {
        LOGD("Failed to make the Packet\n");
        return NULL;
    }
    memset(packet, 0, packet_size);

    header = new_header(0, 0, 0);
    if (!header) {
        LOGD("Faield to make the Header\n");
        destroy_packet(packet);
        return NULL;
    }

    result = set_header(packet, header);
    if (result == FALSE) {
        LOGD("Failed to set the header\n");
        destroy_packet(packet);
        return NULL;
    }

    tail = new_tail(0, 0);
    if (!tail) {
        LOGD("Failed to make the Tail\n");
        destroy_packet(packet);
        return NULL;
    }

    result = set_tail(packet, tail);
    if (result == FALSE) {
        LOGD("Failed to set the tail\n");
        destroy_packet(packet);
        return NULL;
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

    LOGD("payload_len:%ld\n", payload_len);
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
    LOGD("chekc_sum:%d\n", check_sum);
    result = set_check_sum(packet, check_sum);
    if (result == FALSE) {
        LOGD("Failed to set the check_sum\n");
        return FALSE;
    }

    print_packet(packet);
    return packet;
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

        print_buf(buf + HEADER_SIZE, payload_len);
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

    mesg_size = get_message_size();
    mesgs_size = n * mesg_size;
    mesgs = (Message*) malloc(mesgs_size);
    tmp = mesgs;

    for (i = 0; i < n; i++) {
        mesgs = next_mesg(tmp, i);
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
    mesg_size = get_message_size();
    mesg = (Message*) malloc(get_message_size());
    if (!mesg) {
        LOGD("Failed to make message \n");
        return NULL;
    }
    memset(mesg, 0, mesg_size);

    memcpy(&time, payload, sizeof(time));
    payload += sizeof(time);
    set_time(mesg, time);

    memcpy(&len, payload, sizeof(len));
    payload += sizeof(len);
    set_str_len(mesg, len);

    str = (char*) malloc(len);
    if (!str) {
        LOGD("Failed to make the str\n");
        return NULL;
    }
    memcpy(str, payload, len);
    set_str(mesg, str);

    if (mesg_len) {
        *mesg_len = sizeof(time) + sizeof(len) + len;
    }

    return mesg;
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
    print_buf(tmp, sizeof(len));
    tmp += sizeof(len);

    for (i = 0; i < len; i++) {
        mesg = next_mesg(mesgs, i);
        if (!mesg) {
            LOGD("There is nothing to point the mesg\n");
            return FALSE;
        }

        if (str_len) {
            tmp += str_len;
        }

        time = get_time(mesg);
        if (time == -1) {
            LOGD("Failed to get the time\n");
            return FALSE;
        }
        memcpy(tmp, &time, sizeof(time));
        print_buf(tmp, sizeof(time));
        tmp += sizeof(time);

        str_len = get_str_len(mesg);
        if (str_len == -1) {
            LOGD("Failed to get the str_len\n");
            return FALSE;
        }
        memcpy(tmp, &str_len, sizeof(str_len));
        print_buf(tmp, sizeof(str_len));
        tmp += sizeof(str_len);

        str = get_str(mesg);
        if (!str) {
            LOGD("Faield to get the str\n");
            return FALSE;
        }
        memcpy(tmp, str, str_len);
        print_buf(tmp, str_len);
    }

    return TRUE;
}
