#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "packet.h"
#include "utils.h"
#include "m_boolean.h"
#include "stream_buf.h"
#include "converter.h"

struct _Header {
    char sop;
    short op_code;
    long int payload_len;
};

struct _Body {
    char *payload;
};

struct _Tail {
    char eop;
    short checksum;
};

struct _Packet {
    Header *header;
    Body *body;
    Tail *tail;
};

Packet *new_packet(char *buf) {
    Packet *packet;

    packet = convert_buf_to_packet(buf);
    if (!packet) {
        LOGD("Failed to convert buf to the Packet\n");
        return NULL;
    }
    return packet;
}

void destroy_packet(Packet *packet) {
    if (!packet) {
        LOGD("There is nothing to remove packet\n");
        return;
    }

    packet_destroy_header(packet->header);
    packet_destroy_body(packet->body);
    packet_destroy_tail(packet->tail);

    free(packet);
}

Header *packet_new_header(char sop, short op_code, long int payload_len) {
    Header *header;

    header = (Header*) malloc(sizeof(Header));
    if (!header) {
        LOGD("Failed to make Header\n");
        return NULL;
    }

    memset(header, 0, sizeof(Header));
    header->sop = sop;
    header->op_code = op_code;
    header->payload_len = payload_len;

    return header;
}

void packet_destroy_header(Header *header) {
    if (!header) {
        LOGD("There is nothing to remove header\n");
        return;
    }

    free(header);
}

Body *packet_new_body(char *payload) {
    Body *body;

    body = (Body*) malloc(sizeof(Body));
    if (!body) {
        LOGD("Failed to make Body");
        return NULL;
    }
    memset(body, 0, sizeof(Body));
    body->payload = payload;
    return body;
}

void packet_destroy_body(Body *body) {
    if (!body) {
        LOGD("There is nothing to remove Body\n");
        return;
    }

    if (body->payload) {
        free(body->payload);
    }

    free(body);
}

Tail *packet_new_tail(char eop, short checksum) {
    Tail *tail;

    tail = (Tail*) malloc(sizeof(Tail));
    if (!tail) {
        LOGD("Failed to make Tail");
        return NULL;
    }
    memset(tail, 0, sizeof(Tail));
    tail->eop = eop;
    tail->checksum = checksum;

    return tail;
}

void packet_destroy_tail(Tail *tail) {
    if (!tail) {
        LOGD("There is nothing to remove Tail");
        return;
    }

    free(tail);
}

BOOLEAN packet_set_sop(Packet *packet, char sop) {
    Header *header;
    if (!packet || !packet->header) {
        LOGD("Can't set op_code");
        return FALSE;
    }

    header = packet->header;
    header->sop = sop;

    return TRUE;
}

BOOLEAN packet_set_eop(Packet *packet, char eop) {
    Tail *tail;
    if (!packet || !packet->tail) {
        LOGD("Can't set op_code");
        return FALSE;
    }

    tail = packet->tail;
    tail->eop = eop;

    return TRUE;
}

BOOLEAN packet_set_op_code(Packet *packet, short op_code) {
    Header *header;
    if (!packet || !packet->header) {
        LOGD("Can't set op_code");
        return FALSE;
    }

    header = packet->header;
    header->op_code = op_code;

    return TRUE;
}

char packet_get_sop(Packet *packet, Header *header) {
    if (packet) {
        header = packet->header;
    }

    if (!header) {
        LOGD("Can't get sop");
        return -1;
    }

    return header->sop;
}

char packet_get_eop(Packet *packet, Tail *tail) {
    if (packet) {
        tail = packet->tail;
    }

    if (!tail) {
        LOGD("Can't get eop");
        return -1;
    }
    return tail->eop;
}

short packet_get_op_code(Packet *packet, Header *header) {
    if (packet) {
        header = packet->header;
    }
    if (!header) {
        LOGD("Can't get op_code");
        return -1;
    }
    return header->op_code;
}

short packet_get_checksum(Packet *packet, Tail *tail) {
    if (packet) {
        tail = packet->tail;
    }

    if (!tail) {
        LOGD("Can't get check_sum");
        return -1;
    }
    return tail->checksum;
}

int packet_get_size() {
    return sizeof(Packet);
}

BOOLEAN packet_set_payload_len(Packet *packet, long int payload_len) {
    Header *header;

    if (!packet || !packet->header) {
        LOGD("Can't set payload_len");
        return FALSE;
    }

    header = packet->header;
    header->payload_len = payload_len;

    return TRUE;
}

long int packet_get_payload_len(Packet *packet, Header *header) {
    if (packet) {
        header = packet->header;
    }

    if (!header) {
        LOGD("Can't get payload_len");
        return -1;
    }

    return header->payload_len;
}

BOOLEAN packet_set_payload(Packet *packet, char *payload) {
    Body *body;

    if(!packet || !packet->body) {
        LOGD("Can't set payload");
        return FALSE;
    }

    body = packet->body;
    body->payload = payload;
    return TRUE;
}

char *packet_get_payload(Packet *packet, Body *body) {
    if (packet) {
        body = packet->body;
    }

    if (!body) {
        LOGD("Can't get payload\n");
        return NULL;
    }
    return body->payload;
}

short packet_create_checksum(Packet *packet, char *buf, int len) {
    short op_code;
    long int payload_len;
    char *payload;
    int i;
    short checksum;

    if (!packet && !buf) {
        LOGD("Can't craet the check_sum\n");
        return -1;
    }

    checksum = 0;

    if (packet) {
        op_code = packet_get_op_code(packet, NULL);
        payload_len = packet_get_payload_len(packet, NULL);
        payload = packet_get_payload(packet, NULL);

        checksum += packet_get_sop(packet, NULL);
        checksum += op_code;
        checksum += payload_len;

        for(i = 0; i < payload_len; i++) {
            checksum += payload[i];
        }

        checksum += packet_get_eop(packet, NULL);
    }

    if (buf) {
        for(i = 0; i < len -2; i++) {
            checksum += (unsigned char) buf[i];
        }
    }
    return checksum;
}

BOOLEAN packet_set_checksum(Packet *packet, short checksum) {
    Tail *tail;

    if (!packet) {
        LOGD("There is nothing to point packet");
        return FALSE;
    }

    tail = packet->tail;
    tail->checksum = checksum;
    LOGD("checksum:%d\n", tail->checksum);

    return TRUE;
}

BOOLEAN packet_set_body(Packet *packet, Body *body) {
    if (!packet || !body) {
        LOGD("Can't set the Body");
        return FALSE;
    }

    packet->body = body;
    return TRUE;
}

BOOLEAN packet_set_header(Packet *packet, Header *header) {
    if (!packet || !header) {
        LOGD("Can't set the Body");
        return FALSE;
    }

    packet->header = header;
    return TRUE;
}

BOOLEAN packet_set_tail(Packet *packet, Tail *tail) {
    if (!packet || !tail) {
        LOGD("Can't set the Tail");
        return FALSE;
    }

    packet->tail = tail;
    return TRUE;
}

Header *packet_get_header(Packet *packet) {
    if (!packet) {
        LOGD("Can't get the Header");
        return NULL;
    }
    return packet->header;
}

Tail *packet_get_tail(Packet *packet) {
    if (!packet) {
        LOGD("Can't get the Tail");
        return NULL;
    }
    return packet->tail;
}

Body *packet_get_body(Packet *packet) {
    if (!packet) {
        LOGD("Can't get the Body");
        return NULL;
    }
    return packet->body;
}

int packet_get_len(Packet *packet) {
    int len;
    if (!packet) {
        LOGD("There is nothing to point the Packet");
        return -1;
    }

    len = packet_get_payload_len(packet, NULL);
    if (len == -1) {
        LOGD("Failed to get packet_len");
        return -1;
    }

    len += HEADER_SIZE + TAIL_SIZE;
    return len;
}