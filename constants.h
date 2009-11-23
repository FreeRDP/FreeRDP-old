/*
   rdesktop: A Remote Desktop Protocol client.
   Miscellaneous protocol constants
   Copyright (C) Matthew Chapman 1999-2008
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

#ifndef __CONSTANTS_H
#define __CONSTANTS_H

#include "constants_ui.h"

/* TCP port for Remote Desktop Protocol */
#define TCP_PORT_RDP 3389

#define DEFAULT_CODEPAGE	"UTF-8"
#define WINDOWS_CODEPAGE	"UTF-16LE"

/* ISO PDU codes */
enum ISO_PDU_CODE
{
	ISO_PDU_CR = 0xE0,	/* Connection Request */
	ISO_PDU_CC = 0xD0,	/* Connection Confirm */
	ISO_PDU_DR = 0x80,	/* Disconnect Request */
	ISO_PDU_DT = 0xF0,	/* Data */
	ISO_PDU_ER = 0x70	/* Error */
};

/* MCS PDU codes */
enum MCS_PDU_TYPE
{
	MCS_EDRQ = 1,		/* Erect Domain Request */
	MCS_DPUM = 8,		/* Disconnect Provider Ultimatum */
	MCS_AURQ = 10,		/* Attach User Request */
	MCS_AUCF = 11,		/* Attach User Confirm */
	MCS_CJRQ = 14,		/* Channel Join Request */
	MCS_CJCF = 15,		/* Channel Join Confirm */
	MCS_SDRQ = 25,		/* Send Data Request */
	MCS_SDIN = 26		/* Send Data Indication */
};

/* Negotation message types*/
#define TYPE_RDP_NEG_REQ	0x1
#define TYPE_RDP_NEG_RSP	0x2
#define TYPE_RDP_NEG_FAILURE	0x3

/* Protocol security flags */
#define PROTOCOL_RDP		0x00000000
#define PROTOCOL_SSL		0x00000001
#define PROTOCOL_HYBRID		0x00000002

/* Protocol security negotation failure reason flags */
#define SSL_REQUIRED_BY_SERVER		0x00000001
#define SSL_NOT_ALLOWED_BY_SERVER	0x00000002
#define SSL_CERT_NOT_ON_SERVER		0x00000003
#define INCONSISTENT_FLAGS		0x00000004
#define HYBRID_REQUIRED_BY_SERVER	0x00000005

#define MCS_CONNECT_INITIAL	0x7f65
#define MCS_CONNECT_RESPONSE	0x7f66

#define BER_TAG_BOOLEAN		1
#define BER_TAG_INTEGER		2
#define BER_TAG_OCTET_STRING	4
#define BER_TAG_RESULT		10
#define MCS_TAG_DOMAIN_PARAMS	0x30

#define MCS_GLOBAL_CHANNEL	1003
#define MCS_USERCHANNEL_BASE    1001

/* RDP secure transport constants */
#define SEC_RANDOM_SIZE		32
#define SEC_MODULUS_SIZE	64
#define SEC_MAX_MODULUS_SIZE	256
#define SEC_PADDING_SIZE	8
#define SEC_EXPONENT_SIZE	4

#define SEC_CLIENT_RANDOM	0x0001
#define SEC_ENCRYPT		0x0008
#define SEC_LOGON_INFO		0x0040
#define SEC_LICENCE_NEG		0x0080
#define SEC_REDIRECT_ENCRYPT	0x0C00

#define SEC_TAG_SRV_INFO	0x0c01
#define SEC_TAG_SRV_CRYPT	0x0c02
#define SEC_TAG_SRV_CHANNELS	0x0c03

#define SEC_TAG_CLI_INFO	0xc001
#define SEC_TAG_CLI_CRYPT	0xc002
#define SEC_TAG_CLI_CHANNELS    0xc003
#define SEC_TAG_CLI_4           0xc004

#define SEC_TAG_PUBKEY		0x0006
#define SEC_TAG_KEYSIG		0x0008

#define SEC_RSA_MAGIC		0x31415352	/* RSA1 */

/* RDP licensing constants */
#define LICENCE_TOKEN_SIZE	10
#define LICENCE_HWID_SIZE	20
#define LICENCE_SIGNATURE_SIZE	16

#define LICENCE_TAG_DEMAND	0x01
#define LICENCE_TAG_AUTHREQ	0x02
#define LICENCE_TAG_ISSUE	0x03
#define LICENCE_TAG_REISSUE	0x04
#define LICENCE_TAG_PRESENT	0x12
#define LICENCE_TAG_REQUEST	0x13
#define LICENCE_TAG_AUTHRESP	0x15
#define LICENCE_TAG_RESULT	0xff

#define LICENCE_TAG_USER	0x000f
#define LICENCE_TAG_HOST	0x0010

/* RDP PDU codes */
enum RDP_PDU_TYPE
{
	RDP_PDU_DEMAND_ACTIVE = 1,
	RDP_PDU_CONFIRM_ACTIVE = 3,
	RDP_PDU_REDIRECT = 4,	/* MS Server 2003 Session Redirect */
	RDP_PDU_DEACTIVATE = 6,
	RDP_PDU_DATA = 7
};

enum RDP_DATA_PDU_TYPE
{
	RDP_DATA_PDU_UPDATE = 2,
	RDP_DATA_PDU_CONTROL = 20,
	RDP_DATA_PDU_POINTER = 27,
	RDP_DATA_PDU_INPUT = 28,
	RDP_DATA_PDU_SYNCHRONIZE = 31,
	RDP_DATA_PDU_BELL = 34,
	RDP_DATA_PDU_CLIENT_WINDOW_STATUS = 35,
	RDP_DATA_PDU_LOGON = 38,
	RDP_DATA_PDU_FONT2 = 39,
	RDP_DATA_PDU_KEYBOARD_INDICATORS = 41,
	RDP_DATA_PDU_DISCONNECT = 47
};

enum RDP_CONTROL_PDU_TYPE
{
	RDP_CTL_REQUEST_CONTROL = 1,
	RDP_CTL_GRANT_CONTROL = 2,
	RDP_CTL_DETACH = 3,
	RDP_CTL_COOPERATE = 4
};

enum RDP_UPDATE_PDU_TYPE
{
	RDP_UPDATE_ORDERS = 0,
	RDP_UPDATE_BITMAP = 1,
	RDP_UPDATE_PALETTE = 2,
	RDP_UPDATE_SYNCHRONIZE = 3
};

enum RDP_POINTER_PDU_TYPE
{
	RDP_PTRMSGTYPE_SYSTEM = 1,
	RDP_PTRMSGTYPE_POSITION = 3,
	RDP_PTRMSGTYPE_COLOR = 6,
	RDP_PTRMSGTYPE_CACHED = 7,
        RDP_PTRMSGTYPE_POINTER = 8
};

enum RDP_SYSTEM_POINTER_TYPE
{
	RDP_SYSPTR_NULL = 0,
	RDP_SYSPTR_DEFAULT = 0x7F00
};

enum RDP_RAIL_PDU_TYPE
{
	RDP_RAIL_ORDER_EXEC 		= 0x0001,
	RDP_RAIL_ORDER_ACTIVATE 	= 0x0002,
	RDP_RAIL_ORDER_SYSPARAM 	= 0x0003,
	RDP_RAIL_ORDER_SYSCOMMAND 	= 0x0004,
	RDP_RAIL_ORDER_HANDSHAKE 	= 0x0005,
	RDP_RAIL_ORDER_NOTIFY_EVENT 	= 0x0006,
	RDP_RAIL_ORDER_WINDOWMOVE 	= 0x0008,
	RDP_RAIL_ORDER_LOCALMOVESIZE 	= 0x0009,
	RDP_RAIL_ORDER_MINMAXINFO 	= 0x000A,
	RDP_RAIL_ORDER_CLIENTSTATUS	= 0x000B,
	RDP_RAIL_ORDER_SYSMENU		= 0x000C,
	RDP_RAIL_ORDER_LANGBARINFO	= 0x000D,
	RDP_RAIL_ORDER_EXEC_RESULT	= 0x0080
};

/* RAIL PDU flags */
#define RAIL_EXEC_FLAG_EXPAND_WORKINGDIRECTORY  0x0001
#define RAIL_EXEC_FLAG_TRANSLATE_FILES          0x0002
#define RAIL_EXEC_FLAG_FILE                     0x0004
#define RAIL_EXEC_FLAG_EXPAND_ARGUMENTS         0x0008

/* Raster operation masks */
#define ROP2_S(rop3) (rop3 & 0xf)
#define ROP2_P(rop3) ((rop3 & 0x3) | ((rop3 & 0x30) >> 2))

#define ROP2_COPY	0xc
#define ROP2_XOR	0x6
#define ROP2_AND	0x8
#define ROP2_NXOR	0x9
#define ROP2_OR		0xe

#define MIX_TRANSPARENT	0
#define MIX_OPAQUE	1

#define TEXT2_VERTICAL		0x04
#define TEXT2_IMPLICIT_X	0x20

#define ALTERNATE	1
#define WINDING		2

/* RDP bitmap cache (version 2) constants */
#define BMPCACHE2_C0_CELLS	0x78
#define BMPCACHE2_C1_CELLS	0x78
#define BMPCACHE2_C2_CELLS	0x150
#define BMPCACHE2_NUM_PSTCELLS	0x9f6

#define PDU_FLAG_FIRST		0x01
#define PDU_FLAG_LAST		0x02

/* Client Core Data */

/* Color depth */
#define RNS_UD_COLOR_4BPP	0xCA00
#define RNS_UD_COLOR_8BPP	0xCA01

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
#define RNS_UD_CS_RESERVED1			0x0020
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

#define REDIRECTION_VERSION3	0x08
#define REDIRECTION_VERSION4	0x0C

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
#define INFO_RAIL			0x00008000	// RemoteApp! :)
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

/* Logon flags (to be replaced by the flags above) */
#define RDP_LOGON_AUTO		0x0008
#define RDP_LOGON_NORMAL	0x0033
#define RDP_LOGON_COMPRESSION	0x0080	/* mppc compression with 8kB histroy buffer */
#define RDP_LOGON_BLOB		0x0100
#define RDP_LOGON_COMPRESSION2	0x0200	/* rdp5 mppc compression with 64kB history buffer */
#define RDP_LOGON_LEAVE_AUDIO	0x2000

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

#define CAPS_PROTOCOL_VERSION		0x0200

/* Extra Capabilities Flags */
#define FASTPATH_OUTPUT_SUPPORTED	0x0001 // RDP 5.0
#define NO_BITMAP_COMPRESSION_HDR	0x0400 // RDP 5.0
#define SHADOW_COMPRESSION_LEVEL	0x0002 // RDP 5.1
#define LONG_CREDENTIALS_SUPPORTED	0x0004 // RDP 5.1
#define AUTORECONNECT_SUPPORTED		0x0008 // RDP 5.2
#define ENC_SALTED_CHECKSUM		0x0010 // RDP 5.2

/* Remote Programs Capabilities */
#define RAIL_LEVEL_SUPPORTED			0x00000001
#define RAIL_LEVEL_DOCKED_LANGBAR_SUPPORTED	0x00000002

/* Window List Capabilities */
#define WINDOW_LEVEL_NOT_SUPPORTED	0x00000000
#define WINDOW_LEVEL_SUPPORTED		0x00000001
#define WINDOW_LEVEL_SUPPORTED_EX	0x00000002

/* User Data Header Types */
#define CS_CORE         0xC001
#define CS_SECURITY     0xC002
#define CS_NET          0xC003
#define CS_CLUSTER      0xC004
#define CS_MONITOR      0xC005
#define SC_CORE         0x0C01
#define SC_SECURITY     0x0C02
#define SC_NET          0x0C03

/* Capability Set Types */
#define CAPSET_TYPE_GENERAL                     0x0001
#define CAPSET_TYPE_BITMAP                      0x0002
#define CAPSET_TYPE_ORDER                       0x0003
#define CAPSET_TYPE_BITMAPCACHE                 0x0004
#define CAPSET_TYPE_CONTROL                     0x0005
#define CAPSET_TYPE_ACTIVATION                  0x0007
#define CAPSET_TYPE_POINTER                     0x0008
#define CAPSET_TYPE_SHARE                       0x0009
#define CAPSET_TYPE_COLORCACHE                  0x000A
#define CAPSET_TYPE_SOUND                       0x000C
#define CAPSET_TYPE_INPUT                       0x000D
#define CAPSET_TYPE_FONT                        0x000E
#define CAPSET_TYPE_BRUSH                       0x000F
#define CAPSET_TYPE_GLYPHCACHE                  0x0010
#define CAPSET_TYPE_OFFSCREENCACHE              0x0011
#define CAPSET_TYPE_BITMAPCACHE_HOSTSUPPORT     0x0012
#define CAPSET_TYPE_BITMAPCACHE_REV2            0x0013
#define CAPSET_TYPE_VIRTUALCHANNEL              0x0014
#define CAPSET_TYPE_DRAWNINEGRIDCACHE           0x0015
#define CAPSET_TYPE_DRAWGDIPLUS                 0x0016
#define CAPSET_TYPE_RAIL                        0x0017
#define CAPSET_TYPE_WINDOW                      0x0018
#define CAPSET_TYPE_COMPDESK                    0x0019
#define CAPSET_TYPE_MULTIFRAGMENTUPDATE         0x001A
#define CAPSET_TYPE_LARGE_POINTER               0x001B
#define CAPSET_TYPE_SURFACE_COMMANDS            0x001C
#define CAPSET_TYPE_BITMAP_CODECS               0x001D

#define CAPSET_LEN_GENERAL                      0x0018
#define CAPSET_LEN_ORDER                        0x0058
#define CAPSET_LEN_BITMAP                       0x001C
#define CAPSET_LEN_BITMAPCACHE_REV2             0x0028
#define CAPSET_LEN_BITMAPCACHE                  0x0028
#define CAPSET_LEN_INPUT                        0x0058
#define CAPSET_LEN_CONTROL                      0x000C
#define CAPSET_LEN_ACTIVATION                   0x000C
#define CAPSET_LEN_POINTER                      0x0008
#define CAPSET_LEN_SHARE                        0x0008
#define CAPSET_LEN_COLORCACHE                   0x0008
#define CAPSET_LEN_BRUSH                        0x0008
#define CAPSET_LEN_GLYPHCACHE                   0x0034
#define CAPSET_LEN_SOUND                        0x0008
#define CAPSET_LEN_FONT                         0x0008
#define CAPSET_LEN_OFFSCREENCACHE               0x000C
#define CAPSET_LEN_BITMAPCACHE_HOSTSUPPORT      0x0008
#define CAPSET_LEN_VIRTUALCHANNEL               0x000C
#define CAPSET_LEN_MULTIFRAGMENTUPDATE          0x0008
#define CAPSET_LEN_LARGE_POINTER                0x0006
#define CAPSET_LEN_COMPDESK                     0x0006
#define CAPSET_LEN_SURFACE_COMMANDS             0x000C
#define CAPSET_LEN_BITMAP_CODECS                0x0005
#define CAPSET_LEN_DRAWNINEGRIDCACHE            0x000C
#define CAPSET_LEN_DRAWGDIPLUS			0x0028
#define CAPSET_LEN_RAIL                         0x0008
#define CAPSET_LEN_WINDOW                       0x000B

/* Device redirection capability set types */
#define DR_CAPSET_TYPE_GENERAL			0x0001
#define DR_CAPSET_TYPE_PRINTER			0x0002
#define DR_CAPSET_TYPE_PORT			0x0003
#define DR_CAPSET_TYPE_DRIVE			0x0004
#define DR_CAPSET_TYPE_SMARTCARD		0x0005

#define DR_CAPSET_LEN_GENERAL			0x0028
#define DR_CAPSET_LEN_PRINTER			0x0008
#define DR_CAPSET_LEN_PORT			0x0008
#define DR_CAPSET_LEN_DRIVE			0x0008
#define DR_CAPSET_LEN_SMARTCARD			0x0008

/* Device redirection general capability set */
#define DR_GENERAL_CAPABILITY_VERSION_01	0x00000001
#define DR_GENERAL_CAPABILITY_VERSION_02	0x00000002

#define DR_MINOR_RDP_VERSION_5_0		0x0002
#define DR_MINOR_RDP_VERSION_5_1		0x0005
#define DR_MINOR_RDP_VERSION_5_2		0x000A
#define DR_MINOR_RDP_VERSION_6_X		0x000C

#define DR_DEVICE_REMOVE_PDUS			0x00000001
#define	DR_CLIENT_DISPLAY_NAME_PDU		0x00000002
#define DR_USER_LOGGEDON_PDU			0x00000004

#define DR_ENABLE_ASYNCIO			0x00000000

#define ORDER_CAP_NEGOTIATE	2
#define ORDER_CAP_NOSUPPORT	4

#define BMPCACHE2_FLAG_PERSIST	((uint32)1<<31)

#define RDP_SOURCE		"MSTSC"

/* Order capability set flags */
#define NEGOTIATEORDERSUPPORT	0x0002
#define ZEROBOUNDSDELTASSUPPORT	0x0008
#define COLORINDEXSUPPORT	0x0020
#define SOLIDPATTERNBRUSHONLY	0x0040

/* Indexes 5, 6, 10, 12, 13, 14, 23, 28, 29, 30, 31, 32 are unused */
#define NEG_DSTBLT_INDEX		0x00
#define NEG_PATBLT_INDEX		0x01
#define NEG_SCRBLT_INDEX		0x02
#define NEG_MEMBLT_INDEX		0x03
#define NEG_MEM3BLT_INDEX		0x04
#define NEG_DRAWNINEGRID_INDEX		0x07
#define NEG_LINETO_INDEX		0x08
#define NEG_MULTI_DRAWNINEGRID_INDEX	0x09
#define NEG_SAVEBITMAP_INDEX		0x0B
#define NEG_MULTIDSTBLT_INDEX		0x0F
#define NEG_MULTIPATBLT_INDEX		0x10
#define NEG_MULTISCRBLT_INDEX		0x11
#define NEG_MULTIOPAQUERECT_INDEX	0x12
#define NEG_FAST_INDEX_INDEX		0x13
#define NEG_POLYGON_SC_INDEX		0x14
#define NEG_POLYGON_CB_INDEX		0x15
#define NEG_POLYLINE_INDEX		0x16
#define NEG_FAST_GLYPH_INDEX		0x18
#define NEG_ELLIPSE_SC_INDEX		0x19
#define NEG_ELLIPSE_CB_INDEX		0x1A
#define NEG_INDEX_INDEX			0x1B

/* orderSupportExFlags */
#define ORDERFLAGS_EX_CACHE_BITMAP_REV3_SUPPORT		0x0002
#define ORDERFLAGS_EX_ALTSEC_FRAME_MARKER_SUPPORT	0x0004

/* Bitmap capability set flags */
#define DRAW_ALLOW_DYNAMIC_COLOR_FIDELITY	0x02
#define DRAW_ALLOW_COLOR_SUBSAMPLING		0x04
#define DRAW_ALLOW_SKIP_ALPHA			0x08

/* Brush capability set flags */
#define BRUSH_DEFAULT		0x00000000
#define BRUSH_COLOR_8x8		0x00000001
#define BRUSH_COLOR_FULL	0x00000002

/* Input sapability set flags */
#define INPUT_FLAG_SCANCODES            0x0001
#define INPUT_FLAG_MOUSEX               0x0004
#define INPUT_FLAG_FASTPATH_INPUT       0x0008
#define INPUT_FLAG_UNICODE              0x0010
#define INPUT_FLAG_FASTPATH_INPUT2      0x0020

#define KBD_TYPE_IBM_PC_XT           0x00000001
#define KBD_TYPE_OLIVETTI_ICO        0x00000002
#define KBD_TYPE_IBM_PC_AT           0x00000003
#define KBD_TYPE_IBM_ENHANCED        0x00000004
#define KBD_TYPE_NOKIA_1050          0x00000005
#define KBD_TYPE_NOKIA_9140          0x00000006
#define KBD_TYPE_JAPANESE            0x00000007

/* Glyph cache capability set flags */
#define GLYPH_SUPPORT_NONE	0x0000
#define GLYPH_SUPPORT_PARTIAL	0x0001
#define GLYPH_SUPPORT_FULL	0x0002
#define GLYPH_SUPPORT_ENCODE	0x0003

/* Sound capability set flags */
#define SOUND_BEEPS_FLAG	0x0001

/* Font capability set flags */
#define FONTSUPPORT_FONTLIST	0x0001

/* Control capability set flags */
#define CONTROLPRIORITY_NEVER	0x0002

/* Bitmap cache host support capability set flags */
#define BITMAPCACHE_REV2        0x01

/* Virtual channel capability set flags */
#define VCCAPS_NO_COMPR         0x00000000
#define VCCAPS_COMPR_SC         0x00000001
#define VCCAPS_COMPR_CS_8K      0x00000002

/* Draw NineGrid cache capability set flags */
#define DRAW_NINEGRID_NO_SUPPORT        0x00000000
#define DRAW_NINEGRID_SUPPORTED         0x00000001
#define DRAW_NINEGRID_SUPPORTED_REV2    0x00000002

/* Draw GDI+ capability set flags */
#define DRAW_GDIPLUS_DEFAULT		0x00000000
#define DRAW_GDIPLUS_SUPPORTED		0x00000001
#define DRAW_GDIPLUS_CACHE_LEVEL_DEFAULT	0x00000000
#define DRAW_GDIPLUS_CACHE_LEVEL_ONE		0x00000001

/* Large pointer capability set flags */
#define LARGE_POINTER_FLAG_96x96        0x0001

/* Desktop composition capability set flags */
#define COMPDESK_NOT_SUPPORTED  0x0000
#define COMPDESK_SUPPORTED      0x0001

/* Surface commands capability set flags */
#define SURFCMDS_SETSURFACEBITS         0x00000002

/* Window Order Header Flags */
#define WINDOW_ORDER_TYPE_WINDOW			0x01000000
#define WINDOW_ORDER_TYPE_NOTIFY			0x02000000
#define WINDOW_ORDER_TYPE_DESKTOP			0x04000000
#define WINDOW_ORDER_STATE_NEW				0x10000000
#define WINDOW_ORDER_STATE_DELETED			0x20000000
#define WINDOW_ORDER_FIELD_OWNER			0x00000002
#define WINDOW_ORDER_FIELD_STYLE			0x00000008
#define WINDOW_ORDER_FIELD_SHOW				0x00000010
#define WINDOW_ORDER_FIELD_TITLE			0x00000004
#define WINDOW_ORDER_FIELD_CLIENTAREAOFFSET		0x00004000
#define WINDOW_ORDER_FIELD_CLIENTAREASIZE		0x00010000
#define WINDOW_ORDER_FIELD_RPCONTENT			0x00020000
#define WINDOW_ORDER_FIELD_ROOTPARENT			0x00040000
#define WINDOW_ORDER_FIELD_WNDOFFSET			0x00000800
#define WINDOW_ORDER_FIELD_WNDCLIENTDELTA		0x00008000
#define WINDOW_ORDER_FIELD_WNDSIZE			0x00000400
#define WINDOW_ORDER_FIELD_WNDRECTS			0x00000100
#define WINDOW_ORDER_FIELD_VISOFFSET			0x00001000
#define WINDOW_ORDER_FIELD_VISIBILITY			0x00000200
#define WINDOW_ORDER_FIELD_ICON_BIG			0x00002000
#define WINDOW_ORDER_ICON				0x40000000
#define WINDOW_ORDER_CACHEDICON				0x80000000
#define WINDOW_ORDER_FIELD_NOTIFY_VERSION		0x00000008
#define WINDOW_ORDER_FIELD_NOTIFY_TIP			0x00000001
#define WINDOW_ORDER_FIELD_NOTIFY_INFO_TIP		0x00000002
#define WINDOW_ORDER_FIELD_NOTIFY_STATE			0x00000004
#define WINDOW_ORDER_FIELD_DESKTOP_NONE			0x00000001
#define WINDOW_ORDER_FIELD_DESKTOP_HOOKED		0x00000002
#define WINDOW_ORDER_FIELD_DESKTOP_ARC_COMPLETED	0x00000004
#define WINDOW_ORDER_FIELD_DESKTOP_ARC_BEGAN		0x00000008
#define WINDOW_ORDER_FIELD_DESKTOP_ZORDER		0x00000010
#define WINDOW_ORDER_FIELD_DESKTOP_ACTIVEWND		0x00000020

/* Notification Icon Balloon Tooltip */
#define NIIF_NONE		0x00000000
#define NIIF_INFO		0x00000001
#define NIIF_WARNING		0x00000002
#define NIIF_ERROR		0x00000003
#define NIIF_NOSOUND		0x00000010
#define NIIF_LARGE_ICON		0x00000020

/* Client Execute PDU Flags */
#define RAIL_EXEC_FLAG_EXPAND_WORKING_DIRECTORY	0x0001
#define RAIL_EXEC_FLAG_TRANSLATE_FILES		0x0002
#define RAIL_EXEC_FLAG_FILE			0x0004
#define RAIL_EXEC_FLAG_EXPAND_ARGUMENTS		0x0008

/* Server Execute Result PDU */
#define RAIL_EXEC_S_OK			0x0000
#define RAIL_EXEC_E_HOOK_NOT_LOADED	0x0001
#define RAIL_EXEC_E_DECODE_FAILED	0x0002
#define RAIL_EXEC_E_NOT_IN_ALLOWLIST	0x0003
#define RAIL_EXEC_E_FILE_NOT_FOUND	0x0005
#define RAIL_EXEC_E_FAIL		0x0006
#define RAIL_EXEC_E_SESSION_LOCKED	0x0007

/* Client System Parameters Update PDU */
#define SPI_SETDRAGFULLWINDOWS	0x00000025
#define SPI_SETKEYBOARDCUES	0x0000100B
#define SPI_SETKEYBOARDPREF	0x00000045
#define SPI_SETWORKAREA		0x0000002F
#define RAIL_SPI_DISPLAYCHANGE	0x0000F001
#define SPI_SETMOUSEBUTTONSWAP	0x00000021
#define RAIL_SPI_TASKBARPOS	0x0000F000
#define SPI_SETHIGHCONTRAST	0x00000043

/* Server System Parameters Update PDU */
#define SPI_SETSCREENSAVEACTIVE		0x00000011
#define SPI_SETSCREENSAVESECURE		0x00000077

/* Client System Command PDU */
#define SC_SIZE		0xF000
#define SC_MOVE		0xF010
#define SC_MINIMIZE	0xF020
#define SC_MAXIMIZE	0xF030
#define SC_CLOSE	0xF060
#define SC_KEYMENU	0xF100
#define SC_RESTORE	0xF120
#define SC_DEFAULT	0xF160

/* Client Notify Event PDU */
#define WM_LBUTTONDOWN		0x00000201
#define WM_LBUTTONUP		0x00000202
#define WM_RBUTTONDOWN		0x00000204
#define WM_RBUTTONUP		0x00000205
#define WM_CONTEXTMENU		0x0000007B
#define WM_LBUTTONDBLCLK	0x00000203
#define WM_RBUTTONDBLCLK	0x00000206
#define NIN_SELECT		0x00000400
#define NIN_KEYSELECT		0x00000401
#define NIN_BALLOONSHOW		0x00000402
#define NIN_BALLOONHIDE		0x00000403
#define NIN_BALLOONTIMEOUT	0x00000404
#define NIN_BALLOONUSERCLICK	0x00000405

/* Server Move/Size Start PDU */
#define RAIL_WMSZ_LEFT		0x0001
#define RAIL_WMSZ_RIGHT		0x0002
#define RAIL_WMSZ_TOP		0x0003
#define RAIL_WMSZ_TOPLEFT	0x0004
#define RAIL_WMSZ_TOPRIGHT	0x0005
#define RAIL_WMSZ_BOTTOM	0x0006
#define RAIL_WMSZ_BOTTOMLEFT	0x0007
#define RAIL_WMSZ_BOTTOMRIGHT	0x0008
#define RAIL_WMSZ_MOVE		0x0009
#define RAIL_WMSZ_KEYMOVE	0x000A
#define RAIL_WMSZ_KEYSIZE	0x000B

/* Language Bar Information PDU */
#define TF_SFT_SHOWNORMAL		0x00000001
#define TF_SFT_DOCK			0x00000002
#define TF_SFT_MINIMIZED		0x00000004
#define TF_SFT_HIDDEN			0x00000008
#define TF_SFT_NOTRANSPARENCY		0x00000010
#define TF_SFT_LOWTRANSPARENCY		0x00000020
#define TF_SFT_HIGHTRANSPARENCY		0x00000040
#define TF_SFT_LABELS			0x00000080
#define TF_SFT_NOLABELS			0x00000100
#define TF_SFT_EXTRAICONSONMINIMIZED	0x00000200
#define TF_SFT_NOEXTRAICONSONMINIMIZED	0x00000400
#define TF_SFT_DESKBAND			0x00000800

/* Keymap flags */
#define MapRightShiftMask   (1<<0)
#define MapLeftShiftMask    (1<<1)
#define MapShiftMask (MapRightShiftMask | MapLeftShiftMask)

#define MapRightAltMask     (1<<2)
#define MapLeftAltMask      (1<<3)
#define MapAltGrMask MapRightAltMask

#define MapRightCtrlMask    (1<<4)
#define MapLeftCtrlMask     (1<<5)
#define MapCtrlMask (MapRightCtrlMask | MapLeftCtrlMask)

#define MapRightWinMask     (1<<6)
#define MapLeftWinMask      (1<<7)
#define MapWinMask (MapRightWinMask | MapLeftWinMask)

#define MapNumLockMask      (1<<8)
#define MapCapsLockMask     (1<<9)

#define MapLocalStateMask   (1<<10)

#define MapInhibitMask      (1<<11)

#define MASK_ADD_BITS(var, mask) (var |= mask)
#define MASK_REMOVE_BITS(var, mask) (var &= ~mask)
#define MASK_HAS_BITS(var, mask) ((var & mask)>0)
#define MASK_CHANGE_BIT(var, mask, active) (var = ((var & ~mask) | (active ? mask : 0)))

/* Clipboard constants, "borrowed" from GCC system headers in 
   the w32 cross compiler
   this is the CF_ set when WINVER is 0x0400 */

#ifndef CF_TEXT
#define CF_TEXT         1
#define CF_BITMAP       2
#define CF_METAFILEPICT 3
#define CF_SYLK         4
#define CF_DIF          5
#define CF_TIFF         6
#define CF_OEMTEXT      7
#define CF_DIB          8
#define CF_PALETTE      9
#define CF_PENDATA      10
#define CF_RIFF         11
#define CF_WAVE         12
#define CF_UNICODETEXT  13
#define CF_ENHMETAFILE  14
#define CF_HDROP        15
#define CF_LOCALE       16
#define CF_MAX          17
#define CF_OWNERDISPLAY 128
#define CF_DSPTEXT      129
#define CF_DSPBITMAP    130
#define CF_DSPMETAFILEPICT      131
#define CF_DSPENHMETAFILE       142
#define CF_PRIVATEFIRST 512
#define CF_PRIVATELAST  767
#define CF_GDIOBJFIRST  768
#define CF_GDIOBJLAST   1023
#endif

/* Sound format constants */
#define WAVE_FORMAT_PCM		1
#define WAVE_FORMAT_ADPCM	2
#define WAVE_FORMAT_ALAW	6
#define WAVE_FORMAT_MULAW	7

/* Virtual channel options */
#define CHANNEL_OPTION_INITIALIZED	0x80000000
#define CHANNEL_OPTION_ENCRYPT_RDP	0x40000000
#define CHANNEL_OPTION_COMPRESS_RDP	0x00800000
#define CHANNEL_OPTION_SHOW_PROTOCOL	0x00200000

/* NT status codes for RDPDR */
#define RD_STATUS_SUCCESS                  0x00000000
#define RD_STATUS_NOT_IMPLEMENTED          0x00000001
#define RD_STATUS_PENDING                  0x00000103

#define RD_STATUS_NO_MORE_FILES            0x80000006
#define RD_STATUS_DEVICE_PAPER_EMPTY       0x8000000e
#define RD_STATUS_DEVICE_POWERED_OFF       0x8000000f
#define RD_STATUS_DEVICE_OFF_LINE          0x80000010
#define RD_STATUS_DEVICE_BUSY              0x80000011

#define RD_STATUS_INVALID_HANDLE           0xc0000008
#define RD_STATUS_INVALID_PARAMETER        0xc000000d
#define RD_STATUS_NO_SUCH_FILE             0xc000000f
#define RD_STATUS_INVALID_DEVICE_REQUEST   0xc0000010
#define RD_STATUS_ACCESS_DENIED            0xc0000022
#define RD_STATUS_OBJECT_NAME_COLLISION    0xc0000035
#define RD_STATUS_DISK_FULL                0xc000007f
#define RD_STATUS_FILE_IS_A_DIRECTORY      0xc00000ba
#define RD_STATUS_NOT_SUPPORTED            0xc00000bb
#define RD_STATUS_TIMEOUT                  0xc0000102
#define RD_STATUS_NOTIFY_ENUM_DIR          0xc000010c
#define RD_STATUS_CANCELLED                0xc0000120


/* RDPDR constants */

					       // <le> <be> <meaning>
#define RDPDR_COMPONENT_TYPE_CORE	0x4472 // "sD" "Ds" ???
#define RDPDR_COMPONENT_TYPE_PRINTING	0x5052 // "RP" "PR" (PR)inting

#define PAKID_CORE_SERVER_ANNOUNCE	0x496E // "nI" "nI" ???
#define PAKID_CORE_CLIENTID_CONFIRM	0x4343 // "CC" "CC" (C)lientID (C)onfirm
#define PAKID_CORE_CLIENT_NAME		0x434E // "NC" "CN" (C)lient (N)ame
#define PAKID_CORE_DEVICELIST_ANNOUNCE	0x4441 // "AD" "DA" (D)evice (A)nnounce
#define PAKID_CORE_DEVICE_REPLY		0x6472 // "rd" "dr" (d)evice (r)eply
#define PAKID_CORE_DEVICE_IOREQUEST	0x4952 // "RI" "IR" (I)O (R)equest
#define PAKID_CORE_DEVICE_IOCOMPLETION	0x4943 // "CI" "IC" (I)O (C)ompletion
#define PAKID_CORE_SERVER_CAPABILITY	0x5350 // "PS" "SP" (S)erver (C)apability
#define PAKID_CORE_CLIENT_CAPABILITY	0x4350 // "PC" "CP" (C)lient (C)apability
#define PAKID_CORE_DEVICELIST_REMOVE	0x444D // "MD" "DM" (D)evice list (R)emove
#define PAKID_PRN_CACHE_DATA		0x5043 // "CP" "PC" (P)rinter (C)ache data
#define PAKID_CORE_USER_LOGGEDON	0x554C // "LU" "UL" (U)ser (L)ogged on
#define PAKID_PRN_USING_XPS		0x5543 // "CU" "UC" (U)sing (?)XPS

#define RDPDR_MAX_DEVICES               0x10

#define DEVICE_TYPE_SERIAL              0x00000001
#define DEVICE_TYPE_PARALLEL            0x00000002
#define DEVICE_TYPE_PRINTER             0x00000004
#define DEVICE_TYPE_DISK                0x00000008
#define DEVICE_TYPE_SMARTCARD		0x00000020

#define IRP_MJ_CREATE			0x00000000
#define IRP_MJ_CLOSE			0x00000002
#define IRP_MJ_READ			0x00000003
#define IRP_MJ_WRITE			0x00000004
#define	IRP_MJ_QUERY_INFORMATION	0x00000005
#define IRP_MJ_SET_INFORMATION		0x00000006
#define IRP_MJ_QUERY_VOLUME_INFORMATION	0x0000000A
#define IRP_MJ_DIRECTORY_CONTROL	0x0000000C
#define IRP_MJ_DEVICE_CONTROL		0x0000000E
#define IRP_MJ_LOCK_CONTROL             0x00000011

#define IRP_MN_QUERY_DIRECTORY          0x00000001
#define IRP_MN_NOTIFY_CHANGE_DIRECTORY  0x00000002

#define FILE_DIRECTORY_FILE             0x00000001
#define FILE_NON_DIRECTORY_FILE         0x00000040
#define FILE_COMPLETE_IF_OPLOCKED       0x00000100
#define FILE_DELETE_ON_CLOSE            0x00001000
#define FILE_OPEN_FOR_FREE_SPACE_QUERY  0x00800000

#define RDPDR_PRINTER_ANNOUNCE_FLAG_ASCII		0x00000001
#define RDPDR_PRINTER_ANNOUNCE_FLAG_DEFAULTPRINTER	0x00000002
#define RDPDR_PRINTER_ANNOUNCE_FLAG_NETWORKPRINTER	0x00000004
#define RDPDR_PRINTER_ANNOUNCE_FLAG_TSPRINTER		0x00000008
#define RDPDR_PRINTER_ANNOUNCE_FLAG_XPSFORMAT		0x00000010

/* RDP5 disconnect PDU */
#define exDiscReasonNoInfo				0x0000
#define exDiscReasonAPIInitiatedDisconnect		0x0001
#define exDiscReasonAPIInitiatedLogoff			0x0002
#define exDiscReasonServerIdleTimeout			0x0003
#define exDiscReasonServerLogonTimeout			0x0004
#define exDiscReasonReplacedByOtherConnection		0x0005
#define exDiscReasonOutOfMemory				0x0006
#define exDiscReasonServerDeniedConnection		0x0007
#define exDiscReasonServerDeniedConnectionFips		0x0008
#define exDiscReasonLicenseInternal			0x0100
#define exDiscReasonLicenseNoLicenseServer		0x0101
#define exDiscReasonLicenseNoLicense			0x0102
#define exDiscReasonLicenseErrClientMsg			0x0103
#define exDiscReasonLicenseHwidDoesntMatchLicense	0x0104
#define exDiscReasonLicenseErrClientLicense		0x0105
#define exDiscReasonLicenseCantFinishProtocol		0x0106
#define exDiscReasonLicenseClientEndedProtocol		0x0107
#define exDiscReasonLicenseErrClientEncryption		0x0108
#define exDiscReasonLicenseCantUpgradeLicense		0x0109
#define exDiscReasonLicenseNoRemoteConnections		0x010a

/* Smartcard constants */
#define SCARD_LOCK_TCP		0
#define SCARD_LOCK_SEC		1
#define SCARD_LOCK_CHANNEL	2
#define SCARD_LOCK_RDPDR	3
#define SCARD_LOCK_LAST		4

#endif // __CONSTANTS_H


