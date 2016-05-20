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
        printf("Failed to make Packet\n");
        return NULL;
    }

    packet->header = header;
    packet->body = body;
    packet->tail = tail;

    return packet;
}

void destroy_packet(Packet *packet) {
    if (!packet) {
        printf("There is nothing to remove packet\n");
        return;
    }

    free(packet->header);
    free(packet->body);
    free(packet->tail);
}

Header* new_header(char sop, short op_code, long int payload_len) {
    Header *header;

    header = (Header*) malloc(sizeof(Header));
    if (!header) {
        printf("Failed to make Header\n");
        return NULL;
    }

    header->sop = sop;
    header->op_code = op_code;
    header->payload_len = payload_len;

    return header;
}

void destroy_header(Header *header) {
    if (!header) {
        printf("There is nothing to remove header\n");
        return;
    }

    free(header);
}

Body* new_body(char *payload) {
    Body *body;

    body = (Body*) malloc(sizeof(Body));
    if (!body) {
        printf("Failed to make Body\n");
        return NULL;
    }

    body->payload = payload;
    return body;
}

void destroy_body(Body *body) {
    if (!body) {
        printf("There is nothing to remove Body\n");
        return;
    }

    free(body->payload);
    free(body);
}

Tail* new_tail(char eop, short check_sum) {
    Tail *tail;

    tail = (Tail*) malloc(sizeof(Tail));
    if (!tail) {
        printf("Failed to make Tail\n");
        return NULL;
    }

    tail->eop = eop;
    tail->check_sum = check_sum;

    return tail;
}

void destroy_tail(Tail *tail) {
    if (!tail) {
        printf("There is nothing to remove Tail\n");
        return;
    }

    free(tail);
}

short get_op_code(Packet *packet) {
    Header *header;

    if (!packet && !packet->header) {
        printf("Can't get op_code\n");
        return -1;
    }

    header = packet->header;
    return header->op_code;
}

long int get_payload_len(Packet *packet) {
    Header *header;

    if (!packet && !packet->header) {
        printf("Can't get payload_len\n");
        return -1;
    }

    header = packet->header;
    return header->payload_len;
}

char* get_payload(Packet *packet) {
    Body *body;

    if(!packet && !packet->body) {
        printf("Can't get payload\n");
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
        printf("There is nothing to point the Packet\n");
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

void set_check_sum(Packet *packet, short check_sum) {
    Tail *tail;

    if (!packet) {
        printf("There is nothing to point packet\n");
        return;
    }

    tail = packet->tail;
    tail->check_sum = check_sum;
}
