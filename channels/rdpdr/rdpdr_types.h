/*
   FreeRDP: A Remote Desktop Protocol client.
   Device Redirection Types

   Copyright 2010 Marc-Andre Moreau <marcandre.moreau@gmail.com>

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

#ifndef __RDPDR_TYPES_H
#define __RDPDR_TYPES_H

#include <freerdp/types_ui.h>
#include <freerdp/vchan.h>
#include <freerdp/utils.h>

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

typedef struct irp_queue IRPQueue;

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
	uint8 abortIO;
};

#endif
