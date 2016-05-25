#ifndef __M_LOG_H__
#define __M_LOG_H__

#include <stdarg.h>

#define LOGD(fmt, ...) printf("[DEBUG]\t%s:%d %s " fmt, __FILE__, __LINE__, __func__, ## __VA_ARGS__)

#endif
