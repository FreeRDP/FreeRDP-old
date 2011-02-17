#ifndef __DEBUG_H
#define __DEBUG_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef WITH_DEBUG_ASSERT
#include <assert.h>
#define ASSERT(a)	assert(a)
#else
#define ASSERT(a)	do { } while (0)
#endif

#ifdef WITH_DEBUG
#define DEBUG(fmt, ...)	printf("DBG %s (%d): " fmt, __FUNCTION__, __LINE__, ## __VA_ARGS__)
#else
#define DEBUG(fmt, ...) do { } while (0)
#endif

#ifdef WITH_DEBUG_KBD
#define DEBUG_KBD(fmt, ...) printf("DBG (KBD) %s (%d): " fmt, __FUNCTION__, __LINE__, ## __VA_ARGS__)
#else
#define DEBUG_KBD(fmt, ...) do { } while (0)
#endif

#ifdef WITH_DEBUG_RDP5
#define DEBUG_RDP5(fmt, ...) printf("DBG (RDP5) %s (%d): " fmt, __FUNCTION__, __LINE__, ## __VA_ARGS__)
#else
#define DEBUG_RDP5(fmt, ...) do { } while (0)
#endif

#ifdef WITH_DEBUG_SOUND
#define DEBUG_SOUND(fmt, ...) printf("DBG (SOUND) %s (%d): " fmt, __FUNCTION__, __LINE__, ## __VA_ARGS__)
#else
#define DEBUG_SOUND(fmt, ...) do { } while (0)
#endif

#ifdef WITH_DEBUG_SERIAL
#define DEBUG_SERIAL(fmt, ...) printf("DBG (SERIAL) %s (%d): " fmt, __FUNCTION__, __LINE__, ## __VA_ARGS__)
#else
#define DEBUG_SERIAL(fmt, ...) do { } while (0)
#endif

#ifdef WITH_DEBUG_CHANNEL
#define DEBUG_CHANNEL(fmt, ...) printf("DBG (CHANNEL) %s (%d): " fmt, __FUNCTION__, __LINE__, ## __VA_ARGS__)
#else
#define DEBUG_CHANNEL(fmt, ...) do { } while (0)
#endif

#ifdef WITH_DEBUG_SCARD
#define DEBUG_SCARD(fmt, ...) printf("DBG (SCARD) %s (%d): " fmt, __FUNCTION__, __LINE__, ## __VA_ARGS__)
#else
#define DEBUG_SCARD(fmt, ...) do { } while (0)
#endif

#ifdef WITH_DEBUG_NLA
#define DEBUG_NLA(fmt, ...) fprintf(stderr, "DBG (NLA) %s (%d): " fmt, __FUNCTION__, __LINE__, ## __VA_ARGS__)
#else
#define DEBUG_NLA(fmt, ...) do { } while (0)
#endif

#endif
