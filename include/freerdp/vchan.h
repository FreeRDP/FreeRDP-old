/*
   Copyright (c) 2009 Jay Sorg

   Permission is hereby granted, free of charge, to any person obtaining a
   copy of this software and associated documentation files (the "Software"),
   to deal in the Software without restriction, including without limitation
   the rights to use, copy, modify, merge, publish, distribute, sublicense,
   and/or sell copies of the Software, and to permit persons to whom the
   Software is furnished to do so, subject to the following conditions:

   The above copyright notice and this permission notice shall be included
   in all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
   OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
   AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
   FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
   DEALINGS IN THE SOFTWARE.

   these are the types needed for the ui interface
   self contained file, requires no other includes

*/

#ifndef __VCHAN_H
#define __VCHAN_H

#define CHANNEL_CHUNK_LENGTH 1600
#define CHANNEL_PDU_LENGTH (CHANNEL_CHUNK_LENGTH + sizeof(CHANNEL_PDU_HEADER))

#define CHANNEL_FLAG_FIRST 0x01
#define CHANNEL_FLAG_LAST 0x02
#define CHANNEL_FLAG_ONLY (CHANNEL_FLAG_FIRST | CHANNEL_FLAG_LAST)
#define CHANNEL_FLAG_MIDDLE 0
#define CHANNEL_FLAG_FAIL 0x100
#define CHANNEL_FLAG_SHOW_PROTOCOL 0x10
#define CHANNEL_FLAG_SUSPEND 0x20
#define CHANNEL_FLAG_RESUME 0x40

#define CHANNEL_OPTION_INITIALIZED   0x80000000
#define CHANNEL_OPTION_ENCRYPT_RDP   0x40000000
#define CHANNEL_OPTION_ENCRYPT_SC    0x20000000
#define CHANNEL_OPTION_ENCRYPT_CS    0x10000000
#define CHANNEL_OPTION_PRI_HIGH      0x08000000
#define CHANNEL_OPTION_PRI_MED       0x04000000
#define CHANNEL_OPTION_PRI_LOW       0x02000000
#define CHANNEL_OPTION_COMPRESS_RDP  0x00800000
#define CHANNEL_OPTION_COMPRESS      0x00400000
#define CHANNEL_OPTION_SHOW_PROTOCOL 0x00200000

#define CHANNEL_MAX_COUNT 30

#define CHANNEL_NAME_LEN 7

struct _CHANNEL_DEF
{
	char name[CHANNEL_NAME_LEN + 1];
	uint32 options;
};
typedef struct _CHANNEL_DEF CHANNEL_DEF;
typedef CHANNEL_DEF * PCHANNEL_DEF;
typedef CHANNEL_DEF ** PPCHANNEL_DEF;

struct _CHANNEL_PDU_HEADER
{
	uint32 length;
	uint32 flags;
};
typedef struct _CHANNEL_PDU_HEADER CHANNEL_PDU_HEADER;
typedef CHANNEL_PDU_HEADER * PCHANNEL_PDU_HEADER;

typedef void (*PCHANNEL_INIT_EVENT_FN)(void * pInitHandle,
                                       uint32 event,
                                       void * pData,
                                       uint32 dataLength);

#define CHANNEL_EVENT_INITIALIZED  0
#define CHANNEL_EVENT_CONNECTED    1
#define CHANNEL_EVENT_V1_CONNECTED 2
#define CHANNEL_EVENT_DISCONNECTED 3
#define CHANNEL_EVENT_TERMINATED   4

typedef void (*PCHANNEL_OPEN_EVENT_FN)(uint32  openHandle,
                                       uint32  event,
                                       void *  pData,
                                       uint32  dataLength,
                                       uint32  totalLength,
                                       uint32  dataFlags);

#define CHANNEL_EVENT_DATA_RECEIVED   10
#define CHANNEL_EVENT_WRITE_COMPLETE  11
#define CHANNEL_EVENT_WRITE_CANCELLED 12

#define CHANNEL_RC_OK                             0
#define CHANNEL_RC_ALREADY_INITIALIZED            1
#define CHANNEL_RC_NOT_INITIALIZED                2
#define CHANNEL_RC_ALREADY_CONNECTED              3
#define CHANNEL_RC_NOT_CONNECTED                  4
#define CHANNEL_RC_TOO_MANY_CHANNELS              5
#define CHANNEL_RC_BAD_CHANNEL                    6
#define CHANNEL_RC_BAD_CHANNEL_HANDLE             7
#define CHANNEL_RC_NO_BUFFER                      8
#define CHANNEL_RC_BAD_INIT_HANDLE                9
#define CHANNEL_RC_NOT_OPEN                      10
#define CHANNEL_RC_BAD_PROC                      11
#define CHANNEL_RC_NO_MEMORY                     12
#define CHANNEL_RC_UNKNOWN_CHANNEL_NAME          13
#define CHANNEL_RC_ALREADY_OPEN                  14
#define CHANNEL_RC_NOT_IN_VIRTUALCHANNELENTRY    15
#define CHANNEL_RC_NULL_DATA                     16
#define CHANNEL_RC_ZERO_LENGTH                   17

#define VIRTUAL_CHANNEL_VERSION_WIN2000         1

typedef uint32 (*PVIRTUALCHANNELINIT)(void ** ppInitHandle,
                               PCHANNEL_DEF pChannel,
                               int channelCount,
                               uint32 versionRequested,
                               PCHANNEL_INIT_EVENT_FN pChannelInitEventProc);
typedef uint32 (*PVIRTUALCHANNELOPEN(void * pInitHandle,
                                     uint32 * pOpenHandle,
                                     char * pChannelName,
                                     PCHANNEL_OPEN_EVENT_FN pChannelOpenEventProc);
typedef uint32 (*PVIRTUALCHANNELCLOSE)(uint32 openHandle);

typedef uint32 (*PVIRTUALCHANNELWRITE(uint32  openHandle,
                                      void *  pData,
                                      uint32  dataLength,
                                      void *  pUserData);

struct _CHANNEL_ENTRY_POINTS
{
    uint32 cbSize;
    uint32 protocolVersion;
    PVIRTUALCHANNELINIT  pVirtualChannelInit;
    PVIRTUALCHANNELOPEN  pVirtualChannelOpen;
    PVIRTUALCHANNELCLOSE pVirtualChannelClose;
    PVIRTUALCHANNELWRITE pVirtualChannelWrite;
};
typedef struct _CHANNEL_ENTRY_POINTS CHANNEL_ENTRY_POINTS;
typedef CHANNEL_ENTRY_POINTS * PCHANNEL_ENTRY_POINTS;

typedef int (*PVIRTUALCHANNELENTRY)(PCHANNEL_ENTRY_POINTS pEntryPoints);

#endif
