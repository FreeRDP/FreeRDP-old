/* -*- c-basic-offset: 8 -*-
   FreeRDP: A Remote Desktop Protocol client.
   Device Redirection

   Copyright (C) Marc-Andre Moreau <marcandre.moreau@gmail.com> 2010

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

#ifndef __RDPDR_TYPES_H
#define __RDPDR_TYPES_H

#include <freerdp/types_ui.h>
#include <freerdp/vchan.h>
#include "chan_plugin.h"
#include "chan_stream.h"

#define LOG_LEVEL 1
#define LLOG(_level, _args) \
  do { if (_level < LOG_LEVEL) { printf _args ; } } while (0)
#define LLOGLN(_level, _args) \
  do { if (_level < LOG_LEVEL) { printf _args ; printf("\n"); } } while (0)

typedef uint32 RD_NTSTATUS;
typedef uint32 RD_NTHANDLE;

typedef struct _SERVICE SERVICE;
typedef struct _DEVICE DEVICE;
typedef struct _DEVMAN DEVMAN;
typedef struct _IRP IRP;

struct _SERVICE
{
	uint32 type;
	uint32 (*create) (IRP * irp, const char * path);
	uint32 (*close) (IRP * irp);
	uint32 (*read) (IRP * irp);
	uint32 (*write) (IRP * irp);
	uint32 (*control) (IRP * irp);
	uint32 (*query_volume_info) (IRP * irp);
	uint32 (*query_info) (IRP * irp);
	uint32 (*set_info) (IRP * irp);
	uint32 (*query_directory) (IRP * irp, uint8 initialQuery, const char * path);
	uint32 (*notify_change_directory) (IRP * irp);
	uint32 (*lock_control) (IRP * irp);
	uint32 (*free) (DEVICE * dev);
	uint32 (*process_data) (SERVICE * srv, int type, const char * data, int data_len);
	int    (*get_event) (IRP * irp, uint32 * result);
	int    (*file_descriptor) (IRP *irp);
	void   (*get_timeouts) (IRP * irp, uint32 * timeout, uint32 * interval_timeout);
};
typedef SERVICE * PSERVICE;

struct _DEVICE
{
	uint32 id;
	char* name;
	void* info;
	void* prev;
	void* next;
	SERVICE* service;
	int data_len;
	char* data;
};
typedef DEVICE * PDEVICE;

struct _DEVMAN
{
	int count; /* device count */
	int id_sequence; /* generate unique device id */
	DEVICE* idev; /* iterator device */
	DEVICE* head; /* head device in linked list */
	DEVICE* tail; /* tail device in linked list */
	void* pDevmanEntryPoints; /* entry points for device services */
};
typedef DEVMAN * PDEVMAN;

struct _IRP
{
	DEVICE * dev;
	uint32 fileID;
	uint32 completionID;
	uint32 majorFunction;
	uint32 minorFunction;
	RD_BOOL rwBlocking;
	RD_NTHANDLE ioStatus;
	char * inputBuffer;
	int inputBufferLength;
	uint32 outputResult;
	char * outputBuffer;
	int outputBufferLength;
	int infoClass;
	uint32 desiredAccess;
	uint32 fileAttributes;
	uint32 sharedAccess;
	uint32 createDisposition;
	uint32 createOptions;
	uint32 ioControlCode;
	uint8 watchTree;
	uint32 completionFilter;
	uint32 length;
	uint64 offset;
	uint32 operation;
	uint8 waitOperation;
};

#endif
