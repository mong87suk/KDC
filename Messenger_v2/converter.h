#ifndef __CONVERTER_H__
#define __CONVERTER_H__

#include "packet.h"

typedef enum {
    CONVERT_SUCUESS = 1,
    CONVERT_FAILURE = -1
} CONVERT_RESULT;

CONVERT_RESULT convert_packet_to_buf(Packet *packet, char *buf);

#endif
