#ifndef __CONVERTER_H__
#define __CONVERTER_H__

#include "packet.h"
#include "message.h"

short convert_packet_to_buf(Packet *packet, char *buf);
int convert_buf_to_packet(char *buf, Packet *packet);
int convert_payload_to_mesg(char *payload, Message *mesg);
int convert_mesgs_to_payload(Message *mesgs, char *payload, int len);
#endif
