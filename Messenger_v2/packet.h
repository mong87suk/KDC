#ifndef __PACKET_H__
#define __PACKET_H__

#define REQ_ALL_MSG            (char) 0x01
#define RES_ALL_MSG            (char) 0x02
#define REQ_INTERVAL_MSG       (char) 0x03
#define RCV_MSG                (char) 0x04
#define REQ_FIRST_OR_LAST_MSG  (char) 0x05
#define RCV_FIRST_OR_LAST_MSG  (char) 0x06
#define REQ_MAKE_ACCOUNT       (char) 0x07
#define REQ_LOG_IN             (char) 0x08

#define SOP                    (char) 0xAA
#define EOP                    (char) 0xFF

#define INTERVAL_SIZE           4
#define TIME_SIZE               8
#define STR_LEN_SIZE            4

#define HEADER_SIZE            11
#define TAIL_SIZE              3

#define LOG_IN_INFO_NUM        2

#include "stream_buf.h"
#include "m_boolean.h"

typedef struct _Header Header;
typedef struct _Body Body;
typedef struct _Tail Tail;
typedef struct _Packet Packet;

Packet *new_packet(char *buf);
void destroy_packet(Packet *packet);
Header *packet_new_header(char sop, short op_code, long int payload_len);
void packet_destroy_header(Header *header);
Body *packet_new_body(char *payload);
void packet_destroy_body(Body *body);
Tail *packet_new_tail(char eop, short check_sum);
void packet_destroy_tail(Tail *tail);
short packet_create_checksum(Packet *packet, char *buf, int len);
Header *packet_get_header(Packet *packet);
Tail *packet_get_tail(Packet *packet);
Body *packet_get_body(Packet *packet);
int packet_get_len(Packet *packet);
char packet_get_sop(Packet *packet, Header *header);
char packet_get_eop(Packet *packet, Tail *tail);
short packet_get_checksum(Packet *packet, Tail *tail);
short packet_get_op_code(Packet *packet, Header *header);
long int packet_get_payload_len(Packet *packet, Header *header);
char* packet_get_payload(Packet *packet, Body *body);
int packet_get_size();

BOOLEAN packet_set_sop(Packet *packet, char sop);
BOOLEAN packet_set_eop(Packet *packet, char eop);
BOOLEAN packet_set_op_code(Packet *packet, short op_code);
BOOLEAN packet_set_payload_len(Packet *packet, long int payload_len);
BOOLEAN packet_set_checksum(Packet *packet, short checksum);
BOOLEAN packet_set_body(Packet *packet, Body *body);
BOOLEAN packet_set_payload(Packet *packet, char *payload);
BOOLEAN packet_set_header(Packet *packet, Header *header);
BOOLEAN packet_set_tail(Packet *packet, Tail* tail);

#endif
