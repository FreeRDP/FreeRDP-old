/* -*- c-basic-offset: 8 -*-
   FreeRDP: A Remote Desktop Protocol client.
   RAIL (Remote Application Integrated Locally) constants

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

#ifndef __CONSTANTS_RAIL_H
#define __CONSTANTS_RAIL_H

enum RDP_RAIL_PDU_TYPE
{
	RDP_RAIL_ORDER_EXEC			= 0x0001,
	RDP_RAIL_ORDER_ACTIVATE		= 0x0002,
	RDP_RAIL_ORDER_SYSPARAM		= 0x0003,
	RDP_RAIL_ORDER_SYSCOMMAND	= 0x0004,
	RDP_RAIL_ORDER_HANDSHAKE	= 0x0005,
	RDP_RAIL_ORDER_NOTIFY_EVENT	= 0x0006,
	RDP_RAIL_ORDER_WINDOWMOVE	= 0x0008,
	RDP_RAIL_ORDER_LOCALMOVESIZE	= 0x0009,
	RDP_RAIL_ORDER_MINMAXINFO	= 0x000A,
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
#ifndef _WIN32
#define SPI_SETDRAGFULLWINDOWS	0x00000025
#define SPI_SETKEYBOARDCUES	0x0000100B
#define SPI_SETKEYBOARDPREF	0x00000045
#define SPI_SETWORKAREA		0x0000002F
#define SPI_SETMOUSEBUTTONSWAP	0x00000021
#define SPI_SETHIGHCONTRAST	0x00000043
#endif
#define RAIL_SPI_DISPLAYCHANGE	0x0000F001
#define RAIL_SPI_TASKBARPOS	0x0000F000

/* Server System Parameters Update PDU */
#ifndef _WIN32
#define SPI_SETSCREENSAVEACTIVE		0x00000011
#define SPI_SETSCREENSAVESECURE		0x00000077
#endif

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
#ifndef _WIN32
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
#endif

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

#endif /* __CONSTANTS_RAIL_H */

