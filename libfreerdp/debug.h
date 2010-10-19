
#ifndef __DEBUG_H
#define __DEBUG_H
#ifdef WITH_DEBUG
#define DEBUG(fmt, ...)	fprintf(stderr, "DBG %s (%d): " fmt, __FUNCTION__, __LINE__, ## __VA_ARGS__)
#else
#define DEBUG(fmt, ...) do { } while (0)
#endif

#ifdef WITH_DEBUG_KBD
#define DEBUG_KBD(fmt, ...) fprintf(stderr, "DBG (KBD) %s (%d): " fmt, __FUNCTION__, __LINE__, ## __VA_ARGS__)
#else
#define DEBUG_KBD(fmt, ...) do { } while (0)
#endif

#ifdef WITH_DEBUG_RDP5
#define DEBUG_RDP5(fmt, ...) fprintf(stderr, "DBG (RDP5) %s (%d): " fmt, __FUNCTION__, __LINE__, ## __VA_ARGS__)
#else
#define DEBUG_RDP5(fmt, ...) do { } while (0)
#endif

#ifdef WITH_DEBUG_SOUND
#define DEBUG_SOUND(fmt, ...) fprintf(stderr, "DBG (SOUND) %s (%d): " fmt, __FUNCTION__, __LINE__, ## __VA_ARGS__)
#else
#define DEBUG_SOUND(fmt, ...) do { } while (0)
#endif

#ifdef WITH_DEBUG_SERIAL
#define DEBUG_SERIAL(fmt, ...) fprintf(stderr, "DBG (SERIAL) %s (%d): " fmt, __FUNCTION__, __LINE__, ## __VA_ARGS__)
#else
#define DEBUG_SERIAL(fmt, ...) do { } while (0)
#endif

#ifdef WITH_DEBUG_CHANNEL
#define DEBUG_CHANNEL(fmt, ...) fprintf(stderr, "DBG (CHANNEL) %s (%d): " fmt, __FUNCTION__, __LINE__, ## __VA_ARGS__)
#else
#define DEBUG_CHANNEL(fmt, ...) do { } while (0)
#endif

#ifdef WITH_DEBUG_SCARD
#define DEBUG_SCARD(fmt, ...) fprintf(stderr, "DBG (SCARD) %s (%d): " fmt, __FUNCTION__, __LINE__, ## __VA_ARGS__)
#else
#define DEBUG_SCARD(fmt, ...) do { } while (0)
#endif

#endif
