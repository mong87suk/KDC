#ifndef __CONVERTER_H__
#define __CONVERTER_H__

#include "packet.h"
#include "message.h"
#include "m_boolean.h"

BOOLEAN convert_packet_to_buf(Packet *packet, char *buf);
Packet *convert_buf_to_packet(char *buf);
Message *convert_payload_to_mesgs(char *payload, int *mesg_num);
Message *convert_payload_to_mesg(char *payload, int *mesg_len);
BOOLEAN convert_mesgs_to_payload(Message *mesgs, char *payload, int len);
#endif
