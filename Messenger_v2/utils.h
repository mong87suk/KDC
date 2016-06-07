#ifndef __M_LOG_H__
#define __M_LOG_H__

#include "message.h"

#define LOGD(fmt, ...) printf("[DEBUG]\t%s:%d %s " fmt, __FILE__, __LINE__, __func__, ## __VA_ARGS__)

void print_mesg(Message *mesg);

#endif
