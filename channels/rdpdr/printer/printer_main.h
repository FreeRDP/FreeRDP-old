/* -*- c-basic-offset: 8 -*-
   FreeRDP: A Remote Desktop Protocol client.
   Redirected Printer Device Service

   Copyright (C) Marc-Andre Moreau <marcandre.moreau@gmail.com> 2010
   Copyright (C) Vic Lee <llyzs@163.com> 2010

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

#ifndef __PRINTER_MAIN_H
#define __PRINTER_MAIN_H

/* SERVER_PRINTER_CACHE_EVENT.cachedata */
#define RDPDR_ADD_PRINTER_EVENT             0x00000001
#define RDPDR_UPDATE_PRINTER_EVENT          0x00000002
#define RDPDR_DELETE_PRINTER_EVENT          0x00000003
#define RDPDR_RENAME_PRINTER_EVENT          0x00000004

/* DR_PRN_DEVICE_ANNOUNCE.Flags */
#define RDPDR_PRINTER_ANNOUNCE_FLAG_ASCII           0x00000001
#define RDPDR_PRINTER_ANNOUNCE_FLAG_DEFAULTPRINTER  0x00000002
#define RDPDR_PRINTER_ANNOUNCE_FLAG_NETWORKPRINTER  0x00000004
#define RDPDR_PRINTER_ANNOUNCE_FLAG_TSPRINTER       0x00000008
#define RDPDR_PRINTER_ANNOUNCE_FLAG_XPSFORMAT       0x00000010

int
printer_register_device(PDEVMAN pDevman, PDEVMAN_ENTRY_POINTS pEntryPoints, SERVICE * srv);
uint32
printer_create(IRP * irp, const char * path);
uint32
printer_close(IRP * irp);
uint32
printer_write(IRP * irp);
uint32
printer_free(DEVICE * dev);

#endif

