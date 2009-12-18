/* -*- c-basic-offset: 8 -*-
   FreeRDP: A Remote Desktop Protocol client.
   Capability set constants

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

#ifndef __CONSTANTS_CAPABILITIES_H
#define __CONSTANTS_CAPABILITIES_H

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

#define CAPS_PROTOCOL_VERSION			0x0200

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

#endif /* __CONSTANTS_CAPABILITIES_H */
