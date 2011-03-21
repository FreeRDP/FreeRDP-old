/*
   FreeRDP: A Remote Desktop Protocol client.
   Video Redirection Virtual Channel

   Copyright 2010-2011 Vic Lee

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
*/

#ifndef __TSMF_CONSTANTS_H
#define __TSMF_CONSTANTS_H

/* Interface IDs defined in [MS-RDPEV]. There's no constant names in the MS
   documentation, so we create them on our own. */
#define TSMF_INTERFACE_DEFAULT                  0x00000000
#define TSMF_INTERFACE_CLIENT_NOTIFICATIONS     0x00000001
#define TSMF_INTERFACE_CAPABILITIES             0x00000002

/* Interface ID Mask */
#define STREAM_ID_STUB      0x80000000
#define STREAM_ID_PROXY     0x40000000
#define STREAM_ID_NONE      0x00000000

/* Functon ID */
/* Common IDs for all interfaces are as follows. */
#define RIMCALL_RELEASE                     0x00000001
#define RIMCALL_QUERYINTERFACE              0x00000002
/* Capabilities Negotiator Interface IDs are as follows. */
#define RIM_EXCHANGE_CAPABILITY_REQUEST     0x00000100
/* The Client Notifications Interface ID is as follows. */
#define PLAYBACK_ACK                        0x00000100
#define CLIENT_EVENT_NOTIFICATION           0x00000101
/* Server Data Interface IDs are as follows. */
#define EXCHANGE_CAPABILITIES_REQ           0x00000100
#define SET_CHANNEL_PARAMS                  0x00000101
#define ADD_STREAM                          0x00000102
#define ON_SAMPLE                           0x00000103
#define SET_VIDEO_WINDOW                    0x00000104
#define ON_NEW_PRESENTATION                 0x00000105
#define SHUTDOWN_PRESENTATION REQ           0x00000106
#define SET_TOPOLOGY_REQ                    0x00000107
#define CHECK_FORMAT_SUPPORT_REQ            0x00000108
#define ON_PLAYBACK_STARTED                 0x00000109
#define ON_PLAYBACK_PAUSED                  0x0000010a
#define ON_PLAYBACK_STOPPED                 0x0000010b
#define ON_PLAYBACK_RESTARTED               0x0000010c
#define ON_PLAYBACK_RATE_CHANGED            0x0000010d
#define ON_FLUSH                            0x0000010e
#define ON_STREAM_VOLUME                    0x0000010f
#define ON_CHANNEL_VOLUME                   0x00000110
#define ON_END_OF_STREAM                    0x00000111
#define SET_ALLOCATOR                       0x00000112
#define NOTIFY_PREROLL                      0x00000113
#define UPDATE_GEOMETRY_INFO                0x00000114
#define REMOVE_STREAM                       0x00000115

#endif

