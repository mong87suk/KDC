#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "packet.h"
#include "m_log.h"
#include "m_boolean.h"

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
        M_LOG("Failed to make Packet");
        return NULL;
    }

    packet->header = header;
    packet->body = body;
    packet->tail = tail;

    return packet;
}

void destroy_packet(Packet *packet) {
    if (!packet) {
        M_LOG("There is nothing to remove packet");
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
        M_LOG("Failed to make Header");
        return NULL;
    }

    memset(header, 0, sizeof(Header));
    header->sop = sop;
    header->op_code = op_code;
    header->payload_len = payload_len;

    return header;
}

void destroy_header(Header *header) {
    if (!header) {
        M_LOG("There is nothing to remove header");
        return;
    }

    free(header);
}

Body* new_body(char *payload) {
    Body *body;

    body = (Body*) malloc(sizeof(Body));
    if (!body) {
        M_LOG("Failed to make Body");
        return NULL;
    }
    memset(body, 0, sizeof(Body));
    body->payload = payload;
    return body;
}

void destroy_body(Body *body) {
    if (!body) {
        M_LOG("There is nothing to remove Body");
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
        M_LOG("Failed to make Tail");
        return NULL;
    }
    memset(tail, 0, sizeof(Tail));
    tail->eop = eop;
    tail->check_sum = check_sum;

    return tail;
}

void destroy_tail(Tail *tail) {
    if (!tail) {
        M_LOG("There is nothing to remove Tail");
        return;
    }

    free(tail);
}


short set_op_code(Packet *packet, short op_code) {
    Header *header;
    if (!packet || !packet->header) {
        M_LOG("Can't set op_code");
        return FALSE;
    }

    header = packet->header;
    header->op_code = op_code;

    return TRUE;
}

char get_sop(Packet *packet, Header *header) {
    if (packet) {
        header = packet->header;
    }

    if (!header) {
        M_LOG("Can't get sop");
        return -1;
    }

    return header->sop;
}

char get_eop(Packet *packet, Tail *tail) {
    if (packet) {
        tail = packet->tail;
    }

    if (!tail) {
        M_LOG("Can't get eop");
        return -1;
    }
    return tail->eop;
}

short get_op_code(Packet *packet, Header *header) {
    if (packet) {
        header = packet->header;
    }
    if (!header) {
        M_LOG("Can't get op_code");
        return -1;
    }
    return header->op_code;
}

short get_check_sum(Packet *packet, Tail *tail) {
    if (packet) {
        tail = packet->tail;
    }

    if (!tail) {
        M_LOG("Can't get check_sum");
        return -1;
    }
    return tail->check_sum;
}

short set_payload_len(Packet *packet, long int payload_len) {
    Header *header;

    if (!packet || !packet->header) {
        M_LOG("Can't set payload_len");
        return FALSE;
    }

    header = packet->header;
    header->payload_len = payload_len;

    return TRUE;
}

long int get_payload_len(Packet *packet, Header *header) {
    if (packet) {
        header = packet->header;
    }

    if (!header) {
        M_LOG("Can't get payload_len");
        return -1;
    }

    return header->payload_len;
}

short set_payload(Packet *packet, char *payload) {
    Body *body;

    if(!packet || !packet->body) {
        M_LOG("Can't set payload");
        return FALSE;
    }

    body = packet->body;
    body->payload = payload;
    return TRUE;
}

char* get_payload(Packet *packet, Body *body) {
    if (packet) {
        body = packet->body;
    }

    if (!body) {
        M_LOG("Can't get payload");
        return NULL;
    }
    return body->payload;
}

short do_check_sum(Packet *packet) {
    short op_code;
    long int payload_len;
    char *payload;
    int i;
    short check_sum;

    if (!packet) {
        M_LOG("There is nothing to point the Packet");
        return -1;
    }

    check_sum = 0;

    op_code = get_op_code(packet, NULL);
    payload_len = get_payload_len(packet, NULL);
    payload = get_payload(packet, NULL);

    check_sum += SOP;
    check_sum += op_code;
    check_sum += payload_len;

    for(i = 0; i < payload_len; i++) {
        check_sum += payload[i];
    }

    check_sum += EOP;
    return check_sum;
}

short set_check_sum(Packet *packet, short check_sum) {
    Tail *tail;

    if (!packet) {
        M_LOG("There is nothing to point packet");
        return FALSE;
    }

    tail = packet->tail;
    tail->check_sum = check_sum;

    return TRUE;
}

short set_body(Packet *packet, Body *body) {
    if (!packet || !body) {
        M_LOG("Can't set the Body");
        return FALSE;
    }

    packet->body = body;
    return TRUE;
}

short set_header(Packet *packet, Header *header) {
    if (!packet || !header) {
        M_LOG("Can't set the Body");
        return FALSE;
    }

    packet->header = header;
    return TRUE;
}

short set_tail(Packet *packet, Tail *tail) {
    if (!packet || !tail) {
        M_LOG("Can't set the Tail");
        return FALSE;
    }

    packet->tail = tail;
    return TRUE;
}

Header* get_header(Packet *packet) {
    if (!packet) {
        M_LOG("Can't get the Header");
        return NULL;
    }
    return packet->header;
}

Tail* get_tail(Packet *packet) {
    if (!packet) {
        M_LOG("Can't get the Tail");
        return NULL;
    }
    return packet->tail;
}

Body* get_body(Packet *packet) {
    if (!packet) {
        M_LOG("Can't get the Body");
        return NULL;
    }
    return packet->body;
}

int get_packet_len(Packet *packet) {
    int len;
    if (!packet) {
        M_LOG("There is nothing to point the Packet");
        return -1;
    }

    len = get_payload_len(packet, NULL);
    if (len == -1) {
        M_LOG("Failed to get packet_len");
        return -1;
    }

    len += HEADER_SIZE + TAIL_SIZE;
    return len;
}
