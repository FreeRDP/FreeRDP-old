/* -*- c-basic-offset: 8 -*-
   FreeRDP: A Remote Desktop Protocol client.
   Core RDP constants

   Copyright (C) Marc-Andre Moreau <marcandre.moreau@gmail.com> 2009

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#ifndef __CONSTANTS_CORE_H
#define __CONSTANTS_CORE_H

/* Client Core Data */

/* Color depth */
#define RNS_UD_COLOR_4BPP	0xCA00
#define RNS_UD_COLOR_8BPP	0xCA01
#define RNS_UD_COLOR_16BPP_555	0xCA02
#define RNS_UD_COLOR_16BPP_565	0xCA03
#define RNS_UD_COLOR_24BPP	0xCA04

/* Secure Access Sequence */
#define RNS_UD_SAS_DEL		0xAA03

/* Supported color depths */
#define RNS_UD_24BPP_SUPPORT	0x0001
#define RNS_UD_16BPP_SUPPORT	0x0002
#define RNS_UD_15BPP_SUPPORT	0x0004
#define RNS_UD_32BPP_SUPPORT	0x0008

/* Early Capability Flags */
#define RNS_UD_CS_SUPPORT_ERRINFO_PDU		0x0001
#define RNS_UD_CS_WANT_32BPP_SESSION		0x0002
#define RNS_UD_CS_SUPPORT_STATUSINFO_PDU	0x0004
#define RNS_UD_CS_STRONG_ASYMMETRIC_KEYS	0x0008
#define RNS_UD_CS_VALID_CONNECTION_TYPE		0x0020
#define RNS_UD_CS_SUPPORT_MONITOR_LAYOUT_PDU	0x0040

/* Encryption Methods */
#define ENCRYPTION_40BIT_FLAG	0x00000001
#define ENCRYPTION_128BIT_FLAG	0x00000002
#define ENCRYPTION_56BIT_FLAG	0x00000008
#define ENCRYPTION_FIPS_FLAG	0x00000010

/* Redirection Capabilities Flags */
#define REDIRECTION_SUPPORTED			0x00000001
#define ServerSessionRedirectionVersionMask	0x0000003C
#define REDIRECTED_SESSIONID_FIELD_VALID	0x00000002
#define REDIRECTED_SMARTCARD			0x00000040
/* The following values are shifted 2 places to fit into ServerSessionRedirectionVersionMask */
#define REDIRECTION_VERSION3	(0x02 << 2)
#define REDIRECTION_VERSION4	(0x03 << 2)
#define REDIRECTION_VERSION5	(0x04 << 2)

/* Channel Option Flags */
#define CHAN_INITIALIZED		0x80000000
#define CHAN_ENCRYPT_RDP		0x40000000
#define CHAN_ENCRYPT_SC			0x20000000
#define CHAN_ENCRYPT_CS			0x10000000
#define CHAN_PRI_HIGH			0x08000000
#define CHAN_PRI_MED			0x04000000
#define CHAN_PRI_LOW			0x02000000
#define CHAN_COMPRESS_RDP		0x00800000
#define CHAN_COMPRESS			0x00400000
#define CHAN_SHOW_PROTOCOL		0x00200000
#define CHAN_REMOTE_CONTROL_PERSISTENT	0x00100000

/* End Client Core Data */

/* Info Packet Flags */
#define INFO_MOUSE			0x00000001
#define INFO_DISABLECTRLALTDEL		0x00000002
#define INFO_AUTOLOGON			0x00000008
#define INFO_UNICODE			0x00000010
#define INFO_MAXIMIZESHELL		0x00000020
#define INFO_LOGONNOTIFY		0x00000040
#define INFO_COMPRESSION		0x00000080
#define INFO_ENABLEWINDOWSKEY		0x00000100
#define INFO_REMOTECONSOLEAUDIO		0x00002000
#define INFO_FORCE_ENCRYPTED_CS_PDU	0x00004000
#define INFO_RAIL			0x00008000
#define INFO_LOGONERRORS		0x00010000
#define INFO_MOUSE_HAS_WHEEL		0x00020000
#define INFO_PASSWORD_IS_SC_PIN		0x00040000
#define INFO_NOAUDIOPLAYBACK		0x00080000
#define INFO_USING_SAVED_CREDS		0x00100000

#define CompressionTypeMask		0x00001E00
#define PACKET_COMPR_TYPE_8K		0x00000100
#define PACKET_COMPR_TYPE_64K		0x00000200
#define PACKET_COMPR_TYPE_RDP6		0x00000300
#define PACKET_COMPR_TYPE_RDP61		0x00000400

#define INFO_NORMALLOGON	(INFO_MOUSE | INFO_DISABLECTRLALTDEL | INFO_UNICODE | INFO_MAXIMIZESHELL)

/* compression types */
#define RDP_MPPC_BIG		0x01
#define RDP_MPPC_COMPRESSED	0x20
#define RDP_MPPC_RESET		0x40
#define RDP_MPPC_FLUSH		0x80
#define RDP_MPPC_DICT_SIZE      65536

#define RDP5_COMPRESSED		0x80

/* OS Major Types */
#define OS_MAJOR_TYPE_UNSPECIFIED	0x0000
#define OS_MAJOR_TYPE_WINDOWS		0x0001
#define OS_MAJOR_TYPE_OS2		0x0002
#define OS_MAJOR_TYPE_MACINTOSH		0x0003
#define OS_MAJOR_TYPE_UNIX		0x0004

/* OS Minor Types */
#define OS_MINOR_TYPE_UNSPECIFIED	0x0000
#define OS_MINOR_TYPE_WINDOWS_31X	0x0001
#define OS_MINOR_TYPE_WINDOWS_95	0x0002
#define OS_MINOR_TYPE_WINDOWS_NT	0x0003
#define OS_MINOR_TYPE_OS2_V21		0x0004
#define OS_MINOR_TYPE_POWER_PC		0x0005
#define OS_MINOR_TYPE_MACINTOSH		0x0006
#define OS_MINOR_TYPE_NATIVE_XSERVER	0x0007
#define OS_MINOR_TYPE_PSEUDO_XSERVER	0x0008

/* Performance flags */
#define PERF_DISABLE_WALLPAPER		0x00000001
#define PERF_DISABLE_FULLWINDOWDRAG	0x00000002
#define PERF_DISABLE_MENUANIMATIONS	0x00000004
#define PERF_DISABLE_THEMING		0x00000008
#define PERF_DISABLE_CURSOR_SHADOW	0x00000020
#define PERF_DISABLE_CURSORSETTINGS	0x00000040
#define PERF_ENABLE_FONT_SMOOTHING	0x00000080
#define PERF_ENABLE_DESKTOP_COMPOSITION	0x00000100

/* Extended Info Packet clientAddressFamily */
#define CLIENT_INFO_AF_INET 0x0002
#define CLIENT_INFO_AF_INET6 0x0017

#endif /* __CONSTANTS_CORE_H */

