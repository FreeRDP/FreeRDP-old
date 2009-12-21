
#ifndef __DEBUG_H
#define __DEBUG_H
#ifdef WITH_DEBUG
#define DEBUG(fmt, args...)	fprintf(stderr, "DBG %s (%d): " fmt, __FUNCTION__, __LINE__, ## args)
#else
#define DEBUG(fmt, args...) do { } while (0)
#endif

//#ifdef WITH_DEBUG
#define NEW_DEBUG(args) do { printf("DBG %s (%d) ", __FUNCTION__, __LINE__); printf args ; } while (0)
//#else
//#define NEW_DEBUG(args)
//#endif

#ifdef WITH_DEBUG_KBD
#define DEBUG_KBD(fmt, args...) fprintf(stderr, "DBG (KBD) %s (%d): " fmt, __FUNCTION__, __LINE__, ## args)
#else
#define DEBUG_KBD(fmt, args...) do { } while (0)
#endif

#ifdef WITH_DEBUG_RDP5
#define DEBUG_RDP5(fmt, args...) fprintf(stderr, "DBG (RDP5) %s (%d): " fmt, __FUNCTION__, __LINE__, ## args)
#else
#define DEBUG_RDP5(fmt, args...) do { } while (0)
#endif

#ifdef WITH_DEBUG_CLIPBOARD
#define DEBUG_CLIPBOARD(fmt, args...) fprintf(stderr, "DBG (CLIBBOARD) %s (%d): " fmt, __FUNCTION__, __LINE__, ## args)
#else
#define DEBUG_CLIPBOARD(fmt, args...) do { } while (0)
#endif

#ifdef WITH_DEBUG_SOUND
#define DEBUG_SOUND(fmt, args...) fprintf(stderr, "DBG (SOUND) %s (%d): " fmt, __FUNCTION__, __LINE__, ## args)
#else
#define DEBUG_SOUND(fmt, args...) do { } while (0)
#endif

#ifdef WITH_DEBUG_SERIAL
#define DEBUG_SERIAL(fmt, args...) fprintf(stderr, "DBG (SERIAL) %s (%d): " fmt, __FUNCTION__, __LINE__, ## args)
#else
#define DEBUG_SERIAL(fmt, args...) do { } while (0)
#endif

#ifdef WITH_DEBUG_CHANNEL
#define DEBUG_CHANNEL(fmt, args...) fprintf(stderr, "DBG (CHANNEL) %s (%d): " fmt, __FUNCTION__, __LINE__, ## args)
#else
#define DEBUG_CHANNEL(fmt, args...) do { } while (0)
#endif

#ifdef WITH_DEBUG_SCARD
#define DEBUG_SCARD(fmt, args...) fprintf(stderr, "DBG (SCARD) %s (%d): " fmt, __FUNCTION__, __LINE__, ## args)
#else
#define DEBUG_SCARD(fmt, args...) do { } while (0)
#endif

#endif
