#include <stdlib.h>
#include <stdio.h>

#include "packet.h"

struct _Header
{
    char sop;
    short op_code;
    long int payload_len;
};

struct _Body
{
    char *payload;
};

struct _Tail
{
    char eop;
    char check_sum;
};

struct _Packet
{
    Header *header;
    Body *body;
    Tail *tail;
};

Packet* new_packet(Header *header, Body *body, Tail *tail) {
    Packet *packet;

    packet = (Packet*) malloc(sizeof(Packet));

    if (!packet) {
        printf("%s %s Failed to make Packet\n", __FILE__, __func__);
        return NULL;
    }

    packet->header = header;
    packet->body = body;
    packet->tail = tail;

    return packet;
}

void destroy_packet(Packet *packet) {
    if (!packet) {
        printf("%s %s There is nothing to remove packet\n", __FILE__, __func__);
        return;
    }

    if (packet->header) {
        free(packet->header);
    }

    if (packet->body) {
        free(packet->body);
    }

    if (packet->tail) {
        free(packet->tail);
    }
}

Header* new_header(char sop, short op_code, long int payload_len) {
    Header *header;

    header = (Header*) malloc(sizeof(Header));
    if (!header) {
        printf("%s %s Failed to make Header\n", __FILE__, __func__);
        return NULL;
    }

    header->sop = sop;
    header->op_code = op_code;
    header->payload_len = payload_len;

    return header;
}

void destroy_header(Header *header) {
    if (!header) {
        printf("%s %s There is nothing to remove header\n", __FILE__, __func__);
        return;
    }

    free(header);
}

Body* new_body(char *payload) {
    Body *body;

    body = (Body*) malloc(sizeof(Body));
    if (!body) {
        printf("%s %s Failed to make Body\n", __FILE__, __func__);
        return NULL;
    }

    body->payload = payload;
    return body;
}

void destroy_body(Body *body) {
    if (!body) {
        printf("%s %s There is nothing to remove Body\n", __FILE__, __func__);
        return;
    }

    if (body->payload) {
        free(body->payload);
    }

    free(body);
}

Tail* new_tail(char eop, short check_sum) {
    Tail *tail;

    tail = (Tail*) malloc(sizeof(Tail));
    if (!tail) {
        printf("%s %s Failed to make Tail\n", __FILE__, __func__);
        return NULL;
    }

    tail->eop = eop;
    tail->check_sum = check_sum;

    return tail;
}

void destroy_tail(Tail *tail) {
    if (!tail) {
        printf("%s %s There is nothing to remove Tail\n", __FILE__, __func__);
        return;
    }

    free(tail);
}


PACKET_SET_VALURE_RESULT set_op_code(Packet *packet, short op_code) {
    Header *header;
    if (!packet || !packet->header) {
        printf("%s %s Can't set op_code\n", __FILE__, __func__);
        return PACKET_SET_VALUE_FAILURE;
    }

    header = packet->header;
    header->op_code = op_code;

    return PACKET_SET_VALUE_SUCCESS;
}

short get_op_code(Packet *packet) {
    Header *header;

    if (!packet || !packet->header) {
        printf("%s %s Can't get op_code\n", __FILE__, __func__);
        return -1;
    }

    header = packet->header;
    return header->op_code;
}

PACKET_SET_VALURE_RESULT set_payload_len(Packet *packet, long int payload_len) {
    Header *header;

    if (!packet || !packet->header) {
        printf("%s %s Can't set payload_len\n", __FILE__, __func__);
        return PACKET_SET_VALUE_FAILURE;
    }

    header = packet->header;
    header->payload_len = payload_len;

    return PACKET_SET_VALUE_SUCCESS;
}

long int get_payload_len(Packet *packet) {
    Header *header;

    if (!packet || !packet->header) {
        printf("%s %s Can't get payload_len\n", __FILE__, __func__);
        return -1;
    }

    header = packet->header;
    return header->payload_len;
}

PACKET_SET_VALURE_RESULT set_payload(Packet *packet, char *payload) {
    Body *body;

    if(!packet || !packet->body) {
        printf("%s %s Can't set payload\n", __FILE__, __func__);
        return PACKET_SET_VALUE_FAILURE;
    }

    body = packet->body;
    body->payload = payload;
    return PACKET_SET_VALUE_SUCCESS;
}

char* get_payload(Packet *packet) {
    Body *body;

    if(!packet || !(packet->body)) {
        printf("%s %s Can't get payload\n", __FILE__, __func__);
        return NULL;
    }

    body = packet->body;
    return body->payload;
}

short do_check_sum(Packet *packet) {
    short op_code;
    long int payload_len;
    char *payload;
    int i;
    short check_sum;

    if (!packet) {
        printf("%s %s There is nothing to point the Packet\n", __FILE__, __func__);
        return -1;
    }

    check_sum = 0;

    op_code = get_op_code(packet);
    payload_len = get_payload_len(packet);
    payload = get_payload(packet);

    check_sum += SOP;
    check_sum += op_code;
    check_sum += payload_len;

    for(i = 0; i < payload_len; i++) {
        check_sum += payload[i];
    }

    check_sum += EOP;
    return check_sum;
}

PACKET_SET_VALURE_RESULT set_check_sum(Packet *packet, short check_sum) {
    Tail *tail;

    if (!packet) {
        printf("%s %s There is nothing to point packet\n", __FILE__, __func__);
        return PACKET_SET_VALUE_FAILURE;
    }

    tail = packet->tail;
    tail->check_sum = check_sum;

    return PACKET_SET_VALUE_SUCCESS;
}

PACKET_SET_VALURE_RESULT set_body(Packet *packet, Body *body) {
    if (!packet || !body) {
        printf("%s %s Can't set the Body\n", __FILE__, __func__);
        return PACKET_SET_VALUE_FAILURE;
    }

    packet->body = body;
    return PACKET_SET_VALUE_SUCCESS;
}

Header* get_header(Packet *packet) {
    if (!packet) {
        printf("%s %s Can't get the Header\n", __FILE__, __func__);
        return NULL;
    }
    return packet->header;
}

Tail* get_tail(Packet *packet) {
    if (!packet) {
        printf("%s %s Can't get the Tail\n", __FILE__, __func__);
        return NULL;
    }
    return packet->tail;
}

int get_packet_len(Packet *packet) {
    int len;
    if (!packet) {
        printf("%s %s There is nothing to point the Packet\n", __FILE__, __func__);
        return -1;
    }

    len = get_payload_len(packet);
    if (len == -1) {
        printf("%s %s Failed to get packet_len\n", __FILE__, __func__);
        return -1;
    }

    len += HEADER_SIZE + TAIL_SIZE;
    return len;
}
