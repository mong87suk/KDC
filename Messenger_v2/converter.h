#ifndef __CONVERTER_H__
#define __CONVERTER_H__

#include "packet.h"

short convert_packet_to_buf(Packet *packet, char *buf);
short convert_buf_to_packet(char *buf, Packet *packet);

#endif
