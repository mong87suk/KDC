#ifndef __PACKET_H__
#define __PACKET_H__

#define REQ_ALL_MSG            0x01
#define RES_ALL_MSG            0x02
#define SND_MSG                0x03
#define RCV_MSG                0x04
#define REQ_FIRST_OR_LAST_MSG  0x05
#define RCV_FIRST_OR_LAST_MSG  0x06

#define SOP                    0xAA
#define EOP                    0xFF

#define HEADER_SIZE            11
#define TAIL_SIZE              3

typedef enum {
    PACKET_SET_VALUE_SUCCESS = 1,
    PACKET_SET_VALUE_FAILURE = -1
} PACKET_SET_VALURE_RESULT;

typedef struct _Header Header;
typedef struct _Body Body;
typedef struct _Tail Tail;
typedef struct _Packet Packet;

Packet* new_packet(Header *header, Body *body, Tail *tail);
void destroy_packet(Packet *packet);
Header* new_header(char sop, short op_code, long int payload_len);
void destroy_header(Header *header);
Body* new_body(char *payload);
void destroy_body(Body *body);
Tail* new_tail(char eop, short check_sum);
void destroy_tail(Tail *tail);
short do_check_sum(Packet *packet);
Header* get_header(Packet *packet);
Tail* get_tail(Packet *packet);
Body* get_body(Packet *packet);
int get_packet_len(Packet *packet);
char get_sop(Packet *packet, Header *header);
char get_eop(Packet *packet, Tail *tail);
short get_check_sum(Packet *packet, Tail *tail);
short get_op_code(Packet *packet, Header *header);
long int get_payload_len(Packet *packet, Header *header);
char* get_payload(Packet *packet, Body *body);

PACKET_SET_VALURE_RESULT set_op_code(Packet *packet, short op_code);
PACKET_SET_VALURE_RESULT set_payload_len(Packet *packet, long int payload_len);
PACKET_SET_VALURE_RESULT set_check_sum(Packet *packet, short check_sum);
PACKET_SET_VALURE_RESULT set_body(Packet *packet, Body *body);
PACKET_SET_VALURE_RESULT set_payload(Packet *packet, char *payload);
PACKET_SET_VALURE_RESULT set_header(Packet *packet, Header *header);
PACKET_SET_VALURE_RESULT set_tail(Packet *packet, Tail* tail);

#endif
