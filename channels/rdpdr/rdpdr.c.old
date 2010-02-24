/* -*- c-basic-offset: 8 -*-
   rdesktop: A Remote Desktop Protocol client.
   Copyright (C) Matthew Chapman 1999-2008

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

/*
  Here are some resources, for your IRP hacking pleasure:

  http://cvs.sourceforge.net/viewcvs.py/mingw/w32api/include/ddk/winddk.h?view=markup

  http://win32.mvps.org/ntfs/streams.cpp

  http://www.acc.umu.se/~bosse/ntifs.h

  http://undocumented.ntinternals.net/UserMode/Undocumented%20Functions/NT%20Objects/File/

  http://us1.samba.org/samba/ftp/specs/smb-nt01.txt

  http://www.osronline.com/
*/

#include <unistd.h>
#include <sys/types.h>
#include <sys/time.h>
#include <dirent.h>		/* opendir, closedir, readdir */
#include <time.h>
#include <errno.h>
#include "rdesktop.h"
#include "mcs.h"
#include "channels.h"
#include "secure.h"
#include "rdp.h"
#include "rdpset.h"
#include "constants.h"
#include "capabilities.h"
#include "mem.h"
#include "disk.h"

#include "rdpdr.h"
#include "irp.h"

extern rdpRdp * g_rdp;

extern DEVICE_FNS serial_fns;
extern DEVICE_FNS printer_fns;
extern DEVICE_FNS parallel_fns;
extern DEVICE_FNS disk_fns;
extern FILEINFO g_fileinfo[];
extern RD_BOOL g_notify_stamp;

VCHANNEL *rdpdr_channel;

/* If select() times out, the request for the device with handle g_min_timeout_fd is aborted */
RD_NTHANDLE g_min_timeout_fd;
uint32 g_num_devices;

/* Table with information about rdpdr devices */
RDPDR_DEVICE g_rdpdr_device[RDPDR_MAX_DEVICES];
char *g_rdpdr_clientname = NULL;
uint32 g_rdpdr_clientid = 0;
uint16 g_rdpdr_version_minor = DR_MINOR_RDP_VERSION_5_2;

/* Used to store incoming io request, until they are ready to be completed */
/* using a linked list ensures that they are processed in the right order, */
/* if multiple ios are being done on the same fd */
struct async_iorequest
{
	uint32 fd, major, minor, offset, device, id, length, partial_len;
	long timeout,		/* Total timeout */
	  itv_timeout;		/* Interval timeout (between serial characters) */
	uint8 *buffer;
	DEVICE_FNS *fns;

	struct async_iorequest *next;	/* next element in list */
};

struct async_iorequest *g_iorequest;

void
dr_out_rdpdr_header(STREAM s, uint16 component, uint16 packetID)
{
	out_uint16_le(s, component); // component
	out_uint16_le(s, packetID); // packetID
}

void
dr_out_device_io_completion_header(STREAM s, uint32 deviceID, uint32 completionID, uint32 ioStatus)
{
	dr_out_rdpdr_header(s, RDPDR_COMPONENT_TYPE_CORE, PAKID_CORE_DEVICE_IOCOMPLETION);

	out_uint32_le(s, deviceID); // deviceID
	out_uint32_le(s, completionID); // completionID
	out_uint32_le(s, ioStatus); // ioStatus
}

/* Return device_id for a given handle */
int
get_device_index(RD_NTHANDLE handle)
{
	int i;
	for (i = 0; i < RDPDR_MAX_DEVICES; i++)
	{
		if (g_rdpdr_device[i].handle == handle)
			return i;
	}
	return -1;
}

/* Converts a windows path to a unix path */
void
convert_to_unix_filename(char *filename)
{
	char *p;

	while ((p = strchr(filename, '\\')))
	{
		*p = '/';
	}
}

RD_BOOL
rdpdr_handle_ok(int device, int handle)
{
	switch (g_rdpdr_device[device].device_type)
	{
		case DEVICE_TYPE_SERIAL:
		case DEVICE_TYPE_PARALLEL:
		case DEVICE_TYPE_PRINTER:
		case DEVICE_TYPE_DISK:
			if (g_fileinfo[handle].device_id != device)
				return False;
			break;
		case DEVICE_TYPE_SMARTCARD:
			if (g_rdpdr_device[device].handle != handle)
				return False;
			break;
	}
	return True;
}

/* Add a new io request to the table containing pending io requests so it won't block rdesktop */
RD_BOOL
add_async_iorequest(uint32 device, uint32 file, uint32 id, uint32 major, uint32 length,
		    DEVICE_FNS * fns, uint32 total_timeout, uint32 interval_timeout, uint8 * buffer,
		    uint32 offset)
{
	struct async_iorequest *iorq;

	if (g_iorequest == NULL)
	{
		g_iorequest = (struct async_iorequest *) xmalloc(sizeof(struct async_iorequest));
		if (!g_iorequest)
			return False;
		g_iorequest->fd = 0;
		g_iorequest->next = NULL;
	}

	iorq = g_iorequest;

	while (iorq->fd != 0)
	{
		/* create new element if needed */
		if (iorq->next == NULL)
		{
			iorq->next =
				(struct async_iorequest *) xmalloc(sizeof(struct async_iorequest));
			if (!iorq->next)
				return False;
			iorq->next->fd = 0;
			iorq->next->next = NULL;
		}
		iorq = iorq->next;
	}
	iorq->device = device;
	iorq->fd = file;
	iorq->id = id;
	iorq->major = major;
	iorq->length = length;
	iorq->partial_len = 0;
	iorq->fns = fns;
	iorq->timeout = total_timeout;
	iorq->itv_timeout = interval_timeout;
	iorq->buffer = buffer;
	iorq->offset = offset;
	return True;
}

static void
rdpdr_send_client_announce_reply()
{
	STREAM s;

	s = channel_init(g_rdp->sec->mcs->chan, rdpdr_channel, 32);

	out_uint16_le(s, RDPDR_COMPONENT_TYPE_CORE);
	out_uint16_le(s, PAKID_CORE_CLIENTID_CONFIRM);

	out_uint16_le(s, 1); // versionMajor, must be set to 1
	out_uint16_le(s, g_rdpdr_version_minor); // versionMinor
	out_uint32_be(s, g_rdpdr_clientid); // clientID, given by the server in a Server Announce Request

	printf("rdpdr_send_client_announce_reply\n");

	s_mark_end(s);
	channel_send(g_rdp->sec->mcs->chan, s, rdpdr_channel);
}

static void
rdpdr_process_server_announce_request(STREAM s)
{
	uint16 versionMinor;

	in_uint8s(s, 2); // versionMajor, must be 1
	in_uint16_le(s, versionMinor); // versionMinor
	in_uint32_le(s, g_rdpdr_clientid); // clientID

	printf("Version Minor: %d\n", versionMinor);

	switch(versionMinor)
	{
		case 0x000C:
			printf("Windows Vista, Windows Vista SP1, Windows Server 2008, Windows 7, and Windows Server 2008 R2\n");
			break;

		case 0x000A:
			printf("Windows Server 2003 SP2\n");
			break;

		case 0x0006:
			printf("Windows XP SP3\n");
			break;

		case 0x0005:
			printf("Windows XP, Windows XP SP1, Windows XP SP2, Windows Server 2003, and Windows Server 2003 SP1\n");
			break;

		case 0x0002:
			printf("Windows 2000\n");
			break;
	}

	g_rdpdr_version_minor = versionMinor;
}

static void
rdpdr_send_client_name_request()
{
	STREAM s;
	uint32 hostlen;

	if (NULL == g_rdpdr_clientname)
	{
		g_rdpdr_clientname = g_rdp->settings->hostname;
	}
	hostlen = (strlen(g_rdpdr_clientname) + 1) * 2;

	s = channel_init(g_rdp->sec->mcs->chan, rdpdr_channel, 16 + hostlen);

	out_uint16_le(s, RDPDR_COMPONENT_TYPE_CORE);
	out_uint16_le(s, PAKID_CORE_CLIENT_NAME);

	out_uint32_le(s, 1); // unicodeFlag, 0 for ASCII and 1 for Unicode
	out_uint32(s, 0); // codePage, must be set to zero
	out_uint32_le(s, hostlen); // computerNameLen
	rdp_out_unistr(g_rdp, s, g_rdpdr_clientname, hostlen - 2); // computerName

	printf("rdpdr_send_client_name_request\n");

	s_mark_end(s);
	channel_send(g_rdp->sec->mcs->chan, s, rdpdr_channel);
}

static uint32
rdpdr_drive_device_count()
{
	int i;
	uint32 deviceCount = 0;

	for(i = 0; i < g_num_devices; i++)
		if(g_rdpdr_device[i].device_type == DEVICE_TYPE_DISK)
			deviceCount++;
	
	return deviceCount;
}

static void
rdpdr_out_client_drive_device_list_announce(STREAM s)
{
	int i;
	uint32 deviceCount = rdpdr_drive_device_count();

	out_uint16_le(s, RDPDR_COMPONENT_TYPE_CORE);
	out_uint16_le(s, PAKID_CORE_DEVICELIST_ANNOUNCE);

	out_uint32_le(s, deviceCount); // deviceCount

	for(i = 0; i < g_num_devices; i++)
	{
		if(g_rdpdr_device[i].device_type == DEVICE_TYPE_DISK)
		{
			out_uint32_le(s, g_rdpdr_device[i].device_type); // deviceType
			out_uint32_le(s, i); // deviceID
			out_uint8p(s, g_rdpdr_device[i].name, 8); // preferredDosName
			out_uint32_le(s, 0); // deviceDataLength
			// deviceData

			/*
			 * It is possible to specify a device name longer than 8 characters by
			 * setting deviceDataLength to a non-zero value and putting the long
			 * device name in Unicode in the deviceData field
			 */

		}
	}
}

/* Returns the size of the payload of the announce packet */
static int
announcedata_size()
{
	int size, i;
	PRINTER *printerinfo;

	size = 8;		/* static announce size */
	size += g_num_devices * 0x14;

	for (i = 0; i < g_num_devices; i++)
	{
		if (g_rdpdr_device[i].device_type == DEVICE_TYPE_PRINTER)
		{
			printerinfo = (PRINTER *) g_rdpdr_device[i].pdevice_data;
			printerinfo->bloblen =
				printercache_load_blob(printerinfo->printer, &(printerinfo->blob));

			size += 0x18;
			size += 2 * strlen(printerinfo->driver) + 2;
			size += 2 * strlen(printerinfo->printer) + 2;
			size += printerinfo->bloblen;
		}
	}

	size += (rdpdr_drive_device_count() *  20 + 8);

	return size;
}

static void
rdpdr_send_device_list(void)
{
	uint32 driverlen, printerlen, bloblen;
	int i;
	STREAM s;
	PRINTER *printerinfo;

	s = channel_init(g_rdp->sec->mcs->chan, rdpdr_channel, announcedata_size() + 50);

	out_uint16_le(s, RDPDR_COMPONENT_TYPE_CORE);
	out_uint16_le(s, PAKID_CORE_DEVICELIST_ANNOUNCE);

	out_uint32_le(s, g_num_devices); // deviceCount

	for (i = 0; i < g_num_devices; i++)
	{
		out_uint32_le(s, g_rdpdr_device[i].device_type); // deviceType
		out_uint32_le(s, i); // deviceID
		out_uint8p(s, g_rdpdr_device[i].name, 8); // preferredDosName, Max 8 characters, may not be null terminated

		switch (g_rdpdr_device[i].device_type)
		{
			case DEVICE_TYPE_PRINTER:
				printerinfo = (PRINTER *) g_rdpdr_device[i].pdevice_data;

				driverlen = 2 * strlen(printerinfo->driver) + 2;
				printerlen = 2 * strlen(printerinfo->printer) + 2;
				bloblen = printerinfo->bloblen;

				out_uint32_le(s, 24 + driverlen + printerlen + bloblen); // deviceDataLength
				out_uint32_le(s, printerinfo->default_printer ?
					RDPDR_PRINTER_ANNOUNCE_FLAG_DEFAULTPRINTER : 0); // flags
				out_uint32_le(s, 0); // codePage, must be set to zero
				out_uint32_le(s, 0); // PnPNameLen
				out_uint32_le(s, driverlen); // driverNameLen
				out_uint32_le(s, printerlen); // printNameLen
				out_uint32_le(s, bloblen); // cachedFieldsLen
				// PnPName, which we have set to length 0
				rdp_out_unistr(g_rdp, s, printerinfo->driver, driverlen - 2); // driverName
				rdp_out_unistr(g_rdp, s, printerinfo->printer, printerlen - 2); // printName
				out_uint8a(s, printerinfo->blob, bloblen); // cachedPrinterConfigData

				if (printerinfo->blob)
					xfree(printerinfo->blob);	/* Blob is sent twice if reconnecting */
				break;

			case DEVICE_TYPE_DISK:

				out_uint32(s, rdpdr_drive_device_count() *  20 + 8); // deviceDataLength
				rdpdr_out_client_drive_device_list_announce(s);

				break;

			case DEVICE_TYPE_SMARTCARD:

				/*
				 * According to [MS-RDPEFS] the deviceDataLength field for
				 * the smart card device type must be set to zero
				 */
				
				out_uint32(s, 0); // deviceDataLength
				break;

			default:
				out_uint32(s, 0);
		}
	}
#if 0
	out_uint32_le(s, 0x20);	/* Device type 0x20 - smart card */
	out_uint32_le(s, 0);
	out_uint8p(s, "SCARD", 5);
	out_uint8s(s, 3);
	out_uint32(s, 0);
#endif

	printf("rdpdr_send_device_list\n");

	s_mark_end(s);
	channel_send(g_rdp->sec->mcs->chan, s, rdpdr_channel);
}

void
rdpdr_send_completion(uint32 device, uint32 id, uint32 status, uint32 result, uint8 * buffer, uint32 length)
{
	STREAM s;

	s = channel_init(g_rdp->sec->mcs->chan, rdpdr_channel, 20 + length);

	out_uint16_le(s, RDPDR_COMPONENT_TYPE_CORE);
	out_uint16_le(s, PAKID_CORE_DEVICE_IOCOMPLETION);
	
	out_uint32_le(s, device); // deviceID
	out_uint32_le(s, id); // completionID
	out_uint32_le(s, status); // ioStatus
	out_uint32_le(s, result);
	out_uint8p(s, buffer, length);
	s_mark_end(s);
	
	/* JIF */
#ifdef WITH_DEBUG_RDP5
	printf("--> rdpdr_send_completion\n");
	/* hexdump(s->channel_hdr + 8, s->end - s->channel_hdr - 8); */
#endif
	channel_send(g_rdp->sec->mcs->chan, s, rdpdr_channel);
}

static void
rdpdr_process_irp(STREAM s)
{
	IRP irp;
	memset((void*)&irp, '\0', sizeof(IRP));

	irp.ioStatus = RD_STATUS_SUCCESS;

	/* Device I/O Request Header */
	in_uint32_le(s, irp.deviceID); // deviceID
	in_uint32_le(s, irp.fileID); // fileID
	in_uint32_le(s, irp.completionID); // completionID
	in_uint32_le(s, irp.majorFunction); // majorFunction
	in_uint32_le(s, irp.minorFunction); // minorFunction

	switch(g_rdpdr_device[irp.deviceID].device_type)
	{
		case DEVICE_TYPE_SERIAL:
			irp.fns = &serial_fns;
			irp.rwBlocking = False;
			break;

		case DEVICE_TYPE_PARALLEL:
			irp.fns = &parallel_fns;
			irp.rwBlocking = False;
			break;

		case DEVICE_TYPE_PRINTER:
			irp.fns = &printer_fns;
			irp.rwBlocking = False;
			break;

		case DEVICE_TYPE_DISK:
			irp.fns = &disk_fns;
			irp.rwBlocking = False;
			break;

		case DEVICE_TYPE_SMARTCARD:

		default:
			ui_error(NULL, "IRP bad deviceID %ld\n", irp.deviceID);
			return;
	}

	printf("IRP MAJOR: %d MINOR: %d\n", irp.majorFunction, irp.minorFunction);
	switch(irp.majorFunction)
	{
		case IRP_MJ_CREATE:
			printf("IRP_MJ_CREATE\n");
			irp_process_create_request(s, &irp);
			irp_send_create_response(&irp);
			break;

		case IRP_MJ_CLOSE:
			printf("IRP_MJ_CLOSE\n");
			irp_process_close_request(s, &irp);
			irp_send_close_response(&irp);
			break;

		case IRP_MJ_READ:
			printf("IRP_MJ_READ\n");
			irp_process_read_request(s, &irp);
			break;

		case IRP_MJ_WRITE:
			printf("IRP_MJ_WRITE\n");
			irp_process_write_request(s, &irp);
			break;

		case IRP_MJ_QUERY_INFORMATION:
			printf("IRP_MJ_QUERY_INFORMATION\n");
			irp_process_query_information_request(s, &irp);
			irp_send_query_information_response(&irp);
			break;

		case IRP_MJ_SET_INFORMATION:
			printf("IRP_MJ_SET_INFORMATION\n");
			irp_process_set_volume_information_request(s, &irp);
			break;

		case IRP_MJ_QUERY_VOLUME_INFORMATION:
			printf("IRP_MJ_QUERY_VOLUME_INFORMATION\n");
			irp_process_query_volume_information_request(s, &irp);
			break;

		case IRP_MJ_DIRECTORY_CONTROL:
			printf("IRP_MJ_DIRECTORY_CONTROL\n");
			irp_process_directory_control_request(s, &irp);
			break;

		case IRP_MJ_DEVICE_CONTROL:
			printf("IRP_MJ_DEVICE_CONTROL\n");
			irp_process_device_control_request(s, &irp);
			break;

		case IRP_MJ_LOCK_CONTROL:
			printf("IRP_MJ_LOCK_CONTROL\n");
			irp_process_file_lock_control_request(s, &irp);
			break;

		default:
			ui_unimpl(NULL, "IRP majorFunction=0x%x minorFunction=0x%x\n", irp.majorFunction, irp.minorFunction);
			return;
	}

	if (irp.buffer)
		xfree(irp.buffer);
}

#if 0
static void
dr_process_irp(STREAM s)
{
	uint32 result = 0,
		length = 0,
		desired_access = 0,
		request,
		fileID,
		info_level,
		buffer_len,
		completionID,
		majorFunction,
		minorFunction,
		deviceID,
		offset,
		bytes_in,
		bytes_out,
		error_mode,
		share_mode, disposition, total_timeout, interval_timeout, flags_and_attributes = 0;

	char filename[PATH_MAX];
	uint8 *buffer, *pst_buf;
	struct stream out;
	DEVICE_FNS *fns;
	RD_BOOL rw_blocking = True;
	RD_NTSTATUS status = RD_STATUS_INVALID_DEVICE_REQUEST;

	/* Device I/O Request Header */
	in_uint32_le(s, deviceID); // deviceID
	in_uint32_le(s, fileID); // fileID
	in_uint32_le(s, completionID); // completionID
	in_uint32_le(s, majorFunction); // majorFunction
	in_uint32_le(s, minorFunction); // minorFunction

	buffer_len = 0;
	buffer = (uint8*)xmalloc(1024);
	buffer[0] = 0;

	switch (g_rdpdr_device[deviceID].device_type)
	{
		case DEVICE_TYPE_SERIAL:

			fns = &serial_fns;
			rw_blocking = False;
			break;

		case DEVICE_TYPE_PARALLEL:

			fns = &parallel_fns;
			rw_blocking = False;
			break;

		case DEVICE_TYPE_PRINTER:

			fns = &printer_fns;
			break;

		case DEVICE_TYPE_DISK:

			fns = &disk_fns;
			rw_blocking = False;
			break;

		case DEVICE_TYPE_SMARTCARD:
		default:

			ui_error(NULL, "IRP for bad device %ld\n", deviceID);
			return;
	}

	switch (majorFunction)
	{
		case IRP_MJ_CREATE:

			in_uint32_be(s, desired_access);
			in_uint8s(s, 0x08);	/* unknown */
			in_uint32_le(s, error_mode);
			in_uint32_le(s, share_mode);
			in_uint32_le(s, disposition);
			in_uint32_le(s, flags_and_attributes);
			in_uint32_le(s, length);

			if (length && (length / 2) < 256)
			{
				rdp_in_unistr(g_rdp, s, filename, sizeof(filename), length);
				convert_to_unix_filename(filename);
			}
			else
			{
				filename[0] = 0;
			}

			if (!fns->create)
			{
				status = RD_STATUS_NOT_SUPPORTED;
				break;
			}

			status = fns->create(deviceID, desired_access, share_mode, disposition,
				flags_and_attributes, filename, &result);
			buffer_len = 1;
			break;

		case IRP_MJ_CLOSE:
			if (!fns->close)
			{
				status = RD_STATUS_NOT_SUPPORTED;
				break;
			}

			status = fns->close(fileID);
			break;

		case IRP_MJ_READ:

			if (!fns->read)
			{
				status = RD_STATUS_NOT_SUPPORTED;
				break;
			}

			in_uint32_le(s, length);
			in_uint32_le(s, offset);
			DEBUG_RDP5("RDPDR IRP Read (length: %d, offset: %d)\n", length, offset);
			if (!rdpdr_handle_ok(deviceID, fileID))
			{
				status = RD_STATUS_INVALID_HANDLE;
				break;
			}

			if (rw_blocking)	/* Complete read immediately */
			{
				buffer = (uint8 *) xrealloc((void *) buffer, length);
				if (!buffer)
				{
					status = RD_STATUS_CANCELLED;
					break;
				}
				status = fns->read(fileID, buffer, length, offset, &result);
				buffer_len = result;
				break;
			}

			/* Add request to table */
			pst_buf = (uint8 *) xmalloc(length);
			if (!pst_buf)
			{
				status = RD_STATUS_CANCELLED;
				break;
			}
			serial_get_timeout(fileID, length, &total_timeout, &interval_timeout);
			if (add_async_iorequest
			    (deviceID, fileID, completionID, majorFunction, length, fns, total_timeout, interval_timeout,
			     pst_buf, offset))
			{
				status = RD_STATUS_PENDING;
				break;
			}

			status = RD_STATUS_CANCELLED;
			break;
		case IRP_MJ_WRITE:

			buffer_len = 1;

			if (!fns->write)
			{
				status = RD_STATUS_NOT_SUPPORTED;
				break;
			}

			in_uint32_le(s, length);
			in_uint32_le(s, offset);
			in_uint8s(s, 0x18);
			DEBUG_RDP5("RDPDR IRP Write (length: %d)\n", result);

			if (!rdpdr_handle_ok(deviceID, fileID))
			{
				status = RD_STATUS_INVALID_HANDLE;
				break;
			}

			if (rw_blocking)	/* Complete immediately */
			{
				status = fns->write(fileID, s->p, length, offset, &result);
				break;
			}

			/* Add to table */
			pst_buf = (uint8 *) xmalloc(length);
			if (!pst_buf)
			{
				status = RD_STATUS_CANCELLED;
				break;
			}

			in_uint8a(s, pst_buf, length);

			if (add_async_iorequest
			    (deviceID, fileID, completionID, majorFunction, length, fns, 0, 0, pst_buf, offset))
			{
				status = RD_STATUS_PENDING;
				break;
			}

			status = RD_STATUS_CANCELLED;
			break;

		case IRP_MJ_QUERY_INFORMATION:

			if (g_rdpdr_device[deviceID].device_type != DEVICE_TYPE_DISK)
			{
				status = RD_STATUS_INVALID_HANDLE;
				break;
			}
			in_uint32_le(s, info_level);

			out.data = out.p = buffer;
			out.size = sizeof(buffer);
			status = disk_query_information(fileID, info_level, &out);
			result = buffer_len = out.p - out.data;

			break;

		case IRP_MJ_SET_INFORMATION:

			if (g_rdpdr_device[deviceID].device_type != DEVICE_TYPE_DISK)
			{
				status = RD_STATUS_INVALID_HANDLE;
				break;
			}

			in_uint32_le(s, info_level);

			out.data = out.p = buffer;
			out.size = sizeof(buffer);
			status = disk_set_information(fileID, info_level, s, &out);
			result = buffer_len = out.p - out.data;
			break;

		case IRP_MJ_QUERY_VOLUME_INFORMATION:

			if (g_rdpdr_device[deviceID].device_type != DEVICE_TYPE_DISK)
			{
				status = RD_STATUS_INVALID_HANDLE;
				break;
			}

			in_uint32_le(s, info_level);

			out.data = out.p = buffer;
			out.size = sizeof(buffer);
			status = disk_query_volume_information(fileID, info_level, &out);
			result = buffer_len = out.p - out.data;
			break;

		case IRP_MJ_DIRECTORY_CONTROL:

			if (g_rdpdr_device[deviceID].device_type != DEVICE_TYPE_DISK)
			{
				status = RD_STATUS_INVALID_HANDLE;
				break;
			}

			switch (minorFunction)
			{
				case IRP_MN_QUERY_DIRECTORY:

					in_uint32_le(s, info_level);
					in_uint8s(s, 1);
					in_uint32_le(s, length);
					in_uint8s(s, 0x17);

					if (length && length < 2 * 255)
					{
						rdp_in_unistr(g_rdp, s, filename, sizeof(filename), length);
						convert_to_unix_filename(filename);
					}
					else
					{
						filename[0] = 0;
					}
					out.data = out.p = buffer;
					out.size = sizeof(buffer);
					status = disk_query_directory(fileID, info_level, filename, &out);
					result = buffer_len = out.p - out.data;
					if (!buffer_len)
						buffer_len++;
					break;

				case IRP_MN_NOTIFY_CHANGE_DIRECTORY:

					/* JIF
					   ui_unimpl(NULL, "IRP majorFunction=0x%x minorFunction=0x%x: IRP_MN_NOTIFY_CHANGE_DIRECTORY\n", majorFunction, minorFunction);  */

					in_uint32_le(s, info_level);	/* notify mask */

					status = disk_create_notify(fileID, info_level);
					result = 0;

					if (status == RD_STATUS_PENDING)
						add_async_iorequest(deviceID, fileID, completionID, majorFunction, length,
								    fns, 0, 0, NULL, 0);
					break;

				default:

					status = RD_STATUS_INVALID_PARAMETER;
					/* JIF */
					ui_unimpl(NULL, "IRP majorFunction=0x%x minorFunction=0x%x\n", majorFunction, minorFunction);
			}
			break;

		case IRP_MJ_DEVICE_CONTROL:

			if (!fns->device_control)
			{
				status = RD_STATUS_NOT_SUPPORTED;
				break;
			}

			in_uint32_le(s, bytes_out);
			in_uint32_le(s, bytes_in);
			in_uint32_le(s, request);
			in_uint8s(s, 0x14);

			buffer = (uint8 *) xrealloc((void *) buffer, bytes_out + 0x14);
			if (!buffer)
			{
				status = RD_STATUS_CANCELLED;
				break;
			}

			out.data = out.p = buffer;
			out.size = sizeof(buffer);

			/*
			 * FIXME: device_control() returns RD_STATUS_INVALID_PARAMETER when trying
			 * to copy a file that is not in the root directory on Windows Server 2008
			 */

			status = fns->device_control(fileID, request, s, &out);
			result = buffer_len = out.p - out.data;

			/* TODO: Change this dirty fix for Windows Server 2008 */
			status = RD_STATUS_SUCCESS;

			/* Serial SERIAL_WAIT_ON_MASK */
			if (status == RD_STATUS_PENDING)
			{
				if (add_async_iorequest
				    (deviceID, fileID, completionID, majorFunction, length, fns, 0, 0, NULL, 0))
				{
					status = RD_STATUS_PENDING;
					break;
				}
			}
			break;


		case IRP_MJ_LOCK_CONTROL:

			if (g_rdpdr_device[deviceID].device_type != DEVICE_TYPE_DISK)
			{
				status = RD_STATUS_INVALID_HANDLE;
				break;
			}

			in_uint32_le(s, info_level);

			out.data = out.p = buffer;
			out.size = sizeof(buffer);
			/* FIXME: Perhaps consider actually *do*
			   something here :-) */
			status = RD_STATUS_SUCCESS;
			result = buffer_len = out.p - out.data;
			break;

		default:
			ui_unimpl(NULL, "IRP majorFunction=0x%x minorFunction=0x%x\n", majorFunction, minorFunction);
			break;
	}

	if (status != RD_STATUS_PENDING)
	{
		rdpdr_send_completion(deviceID, completionID, status, result, buffer, buffer_len);
	}
	if (buffer)
		xfree(buffer);
	buffer = NULL;
}
#endif


static void
rdpdr_send_capabilities(void)
{
	STREAM s;

	s = channel_init(g_rdp->sec->mcs->chan, rdpdr_channel, 0x50);

	out_uint16_le(s, RDPDR_COMPONENT_TYPE_CORE);
	out_uint16_le(s, PAKID_CORE_CLIENT_CAPABILITY);

	out_uint16_le(s, 5); // numCapabilities
	out_uint16(s, 0); // pad

	rdp_out_dr_general_capset(s);
	rdp_out_dr_printer_capset(s);
	rdp_out_dr_port_capset(s);
	rdp_out_dr_drive_capset(s);
	rdp_out_dr_smartcard_capset(s);

	printf("rdpdr_send_capabilities\n");

	s_mark_end(s);
	channel_send(g_rdp->sec->mcs->chan, s, rdpdr_channel);
}

static void
rdpdr_process_capabilities(STREAM s)
{
	int i;
	uint16 numCapabilities;
	uint16 capabilityType;

	in_uint16_le(s, numCapabilities); // numCapabilities
	in_uint8s(s, 2); // pad

	for(i = 0; i < numCapabilities; i++)
	{
		in_uint16_le(s, capabilityType);

		switch (capabilityType)
		{
			case DR_CAPSET_TYPE_GENERAL:
				rdp_process_dr_general_capset(s);
				break;

			case DR_CAPSET_TYPE_PRINTER:
				rdp_process_dr_printer_capset(s);
				break;

			case DR_CAPSET_TYPE_PORT:
				rdp_process_dr_port_capset(s);
				break;

			case DR_CAPSET_TYPE_DRIVE:
				rdp_process_dr_drive_capset(s);
				break;

			case DR_CAPSET_TYPE_SMARTCARD:
				rdp_process_dr_smartcard_capset(s);
				break;

			default:
				ui_unimpl(NULL, "Device redirection capability set type %d\n", capabilityType);
				break;
		}
	}
}

static void
rdpdr_process(STREAM s)
{
	uint16 component;
	uint16 packetID;

	uint32 deviceID;
	uint32 status;

	DEBUG_RDP5("--- rdpdr_process ---\n");
#if WITH_DEBUG_RDP5
	hexdump(s->p, s->end - s->p);
#endif
	in_uint16_le(s, component);
	in_uint16_le(s, packetID);

	if (component == RDPDR_COMPONENT_TYPE_CORE)
	{
		printf("RDPDR_COMPONENT_TYPE_CORE\n");
		switch (packetID)
		{
			case PAKID_CORE_SERVER_ANNOUNCE:
				printf("PAKID_CORE_SERVER_ANNOUNCE\n");
				rdpdr_process_server_announce_request(s);
				rdpdr_send_client_announce_reply();
				rdpdr_send_client_name_request();
				break;

			case PAKID_CORE_CLIENTID_CONFIRM:
				printf("PAKID_CORE_CLIENTID_CONFIRM\n");
				rdpdr_send_device_list();
				break;

			case PAKID_CORE_DEVICE_REPLY:
				/* connect to a specific resource */
				printf("PAKID_CORE_DEVICE_REPLY\n");
				in_uint32(s, deviceID);
				in_uint32(s, status);
				printf("NTSTATUS: %d\n", status);
				DEBUG_RDP5("RDPDR: Server connected to resource %d\n", handle);
				break;

			case PAKID_CORE_DEVICE_IOREQUEST:
				printf("PAKID_CORE_DEVICE_IOREQUEST\n");
				rdpdr_process_irp(s);
				break;

			case PAKID_CORE_SERVER_CAPABILITY:
				/* server capabilities */
				printf("PAKID_CORE_SERVER_CAPABILITY\n");
				rdpdr_process_capabilities(s);
				rdpdr_send_capabilities();
				break;

			default:
				ui_unimpl(NULL, "RDPDR core component, packetID: 0x%02X\n", packetID);
				break;

		}
	}
	else if (component == RDPDR_COMPONENT_TYPE_PRINTING)
	{
		printf("RDPDR_COMPONENT_TYPE_PRINTING\n");
		switch (packetID)
		{
			case PAKID_PRN_CACHE_DATA:
				printf("PAKID_PRN_CACHE_DATA\n");
				printercache_process(s);
				break;

			default:
				ui_unimpl(NULL, "RDPDR printer component, packetID: 0x%02X\n", packetID);
				break;
		}
	}
	else
		ui_unimpl(NULL, "RDPDR component: 0x%02X packetID: 0x%02X\n", component, packetID);
}

RD_BOOL
rdpdr_init()
{
	rdpdr_channel =	channel_register(g_rdp->sec->mcs->chan, "rdpdr",
					 CHANNEL_OPTION_INITIALIZED | CHANNEL_OPTION_COMPRESS_RDP,
					 rdpdr_process);

	return (rdpdr_channel != NULL);
}

/* Add file descriptors of pending io request to select() */
void
rdpdr_add_fds(int *n, fd_set * rfds, fd_set * wfds, struct timeval *tv, RD_BOOL * timeout)
{
	uint32 select_timeout = 0;	/* Timeout value to be used for select() (in millisecons). */
	struct async_iorequest *iorq;
	char c;

	iorq = g_iorequest;
	while (iorq != NULL)
	{
		if (iorq->fd != 0)
		{
			switch (iorq->major)
			{
				case IRP_MJ_READ:
					/* Is this FD valid? FDs will
					   be invalid when
					   reconnecting. FIXME: Real
					   support for reconnects. */

					FD_SET(iorq->fd, rfds);
					*n = MAX(*n, iorq->fd);

					/* Check if io request timeout is smaller than current (but not 0). */
					if (iorq->timeout
					    && (select_timeout == 0
						|| iorq->timeout < select_timeout))
					{
						/* Set new timeout */
						select_timeout = iorq->timeout;
						g_min_timeout_fd = iorq->fd;	/* Remember fd */
						tv->tv_sec = select_timeout / 1000;
						tv->tv_usec = (select_timeout % 1000) * 1000;
						*timeout = True;
						break;
					}
					if (iorq->itv_timeout && iorq->partial_len > 0
					    && (select_timeout == 0
						|| iorq->itv_timeout < select_timeout))
					{
						/* Set new timeout */
						select_timeout = iorq->itv_timeout;
						g_min_timeout_fd = iorq->fd;	/* Remember fd */
						tv->tv_sec = select_timeout / 1000;
						tv->tv_usec = (select_timeout % 1000) * 1000;
						*timeout = True;
						break;
					}
					break;

				case IRP_MJ_WRITE:
					/* FD still valid? See above. */
					if ((write(iorq->fd, &c, 0) != 0) && (errno == EBADF))
						break;

					FD_SET(iorq->fd, wfds);
					*n = MAX(*n, iorq->fd);
					break;

				case IRP_MJ_DEVICE_CONTROL:
					if (select_timeout > 5)
						select_timeout = 5;	/* serial event queue */
					break;

			}

		}

		iorq = iorq->next;
	}
}

struct async_iorequest *
rdpdr_remove_iorequest(struct async_iorequest *prev, struct async_iorequest *iorq)
{
	if (!iorq)
		return NULL;

	if (iorq->buffer)
		xfree(iorq->buffer);
	if (prev)
	{
		prev->next = iorq->next;
		xfree(iorq);
		iorq = prev->next;
	}
	else
	{
		/* Even if NULL */
		g_iorequest = iorq->next;
		xfree(iorq);
		iorq = NULL;
	}
	return iorq;
}

/* Check if select() returned with one of the rdpdr file descriptors, and complete io if it did */
static void
_rdpdr_check_fds(fd_set * rfds, fd_set * wfds, RD_BOOL timed_out)
{
	RD_NTSTATUS status;
	uint32 result = 0;
	DEVICE_FNS *fns;
	struct async_iorequest *iorq;
	struct async_iorequest *prev;
	uint32 req_size = 0;
	uint32 buffer_len;
	struct stream out;
	uint8 *buffer = NULL;


	if (timed_out)
	{
		/* check serial iv_timeout */

		iorq = g_iorequest;
		prev = NULL;
		while (iorq != NULL)
		{
			if (iorq->fd == g_min_timeout_fd)
			{
				if ((iorq->partial_len > 0) &&
				    (g_rdpdr_device[iorq->device].device_type ==
				     DEVICE_TYPE_SERIAL))
				{

					/* iv_timeout between 2 chars, send partial_len */
					/*printf("RDPDR: IVT total %u bytes read of %u\n", iorq->partial_len, iorq->length); */
					rdpdr_send_completion(iorq->device,
							      iorq->id, RD_STATUS_SUCCESS,
							      iorq->partial_len,
							      iorq->buffer, iorq->partial_len);
					iorq = rdpdr_remove_iorequest(prev, iorq);
					return;
				}
				else
				{
					break;
				}

			}
			else
			{
				break;
			}


			prev = iorq;
			if (iorq)
				iorq = iorq->next;

		}

		rdpdr_abort_io(g_min_timeout_fd, 0, RD_STATUS_TIMEOUT);
		return;
	}

	iorq = g_iorequest;
	prev = NULL;
	while (iorq != NULL)
	{
		if (iorq->fd != 0)
		{
			switch (iorq->major)
			{
				case IRP_MJ_READ:
					if (FD_ISSET(iorq->fd, rfds))
					{
						/* Read the data */
						fns = iorq->fns;

						req_size =
							(iorq->length - iorq->partial_len) >
							8192 ? 8192 : (iorq->length -
								       iorq->partial_len);
						/* never read larger chunks than 8k - chances are that it will block */
						status = fns->read(iorq->fd,
								   iorq->buffer + iorq->partial_len,
								   req_size, iorq->offset, &result);

						if ((long) result > 0)
						{
							iorq->partial_len += result;
							iorq->offset += result;
						}
						DEBUG_RDP5("RDPDR: %d bytes of data read\n", result);

						/* only delete link if all data has been transfered */
						/* or if result was 0 and status success - EOF      */
						if ((iorq->partial_len == iorq->length) ||
						    (result == 0))
						{
							DEBUG_RDP5("RDPDR: AIO total %u bytes read of %u\n", iorq->partial_len, iorq->length);
							rdpdr_send_completion(iorq->device,
									      iorq->id, status,
									      iorq->partial_len,
									      iorq->buffer,
									      iorq->partial_len);
							iorq = rdpdr_remove_iorequest(prev, iorq);
						}
					}
					break;
				case IRP_MJ_WRITE:
					if (FD_ISSET(iorq->fd, wfds))
					{
						/* Write data. */
						fns = iorq->fns;

						req_size =
							(iorq->length - iorq->partial_len) >
							8192 ? 8192 : (iorq->length -
								       iorq->partial_len);

						/* never write larger chunks than 8k - chances are that it will block */
						status = fns->write(iorq->fd,
								    iorq->buffer +
								    iorq->partial_len, req_size,
								    iorq->offset, &result);

						if ((long) result > 0)
						{
							iorq->partial_len += result;
							iorq->offset += result;
						}

						DEBUG_RDP5("RDPDR: %d bytes of data written\n",
						       result);
						/* only delete link if all data has been transfered */
						/* or we couldn't write */
						if ((iorq->partial_len == iorq->length)
						    || (result == 0))
						{
							DEBUG_RDP5("RDPDR: AIO total %u bytes written of %u\n", iorq->partial_len, iorq->length);
							rdpdr_send_completion(iorq->device,
									      iorq->id, status,
									      iorq->partial_len,
									      (uint8 *) "", 1);

							iorq = rdpdr_remove_iorequest(prev, iorq);
						}
					}
					break;
				case IRP_MJ_DEVICE_CONTROL:
					if (serial_get_event(iorq->fd, &result))
					{
						buffer = (uint8 *) xrealloc((void *) buffer, 0x14);
						out.data = out.p = buffer;
						out.size = sizeof(buffer);
						out_uint32_le(&out, result);
						result = buffer_len = out.p - out.data;
						status = RD_STATUS_SUCCESS;
						rdpdr_send_completion(iorq->device, iorq->id,
								      status, result, buffer,
								      buffer_len);
						xfree(buffer);
						iorq = rdpdr_remove_iorequest(prev, iorq);
					}

					break;
			}

		}
		prev = iorq;
		if (iorq)
			iorq = iorq->next;
	}

	/* Check notify */
	iorq = g_iorequest;
	prev = NULL;
	while (iorq != NULL)
	{
		if (iorq->fd != 0)
		{
			switch (iorq->major)
			{

				case IRP_MJ_DIRECTORY_CONTROL:
					if (g_rdpdr_device[iorq->device].device_type ==
					    DEVICE_TYPE_DISK)
					{

						if (g_notify_stamp)
						{
							g_notify_stamp = False;
							status = disk_check_notify(iorq->fd);
							if (status != RD_STATUS_PENDING)
							{
								rdpdr_send_completion(iorq->device,
										      iorq->id,
										      status, 0,
										      NULL, 0);
								iorq = rdpdr_remove_iorequest(prev,
											      iorq);
							}
						}
					}
					break;



			}
		}

		prev = iorq;
		if (iorq)
			iorq = iorq->next;
	}

}

void
rdpdr_check_fds(fd_set * rfds, fd_set * wfds, RD_BOOL timed_out)
{
	fd_set dummy;


	FD_ZERO(&dummy);


	/* fist check event queue only,
	   any serial wait event must be done before read block will be sent
	 */

	_rdpdr_check_fds(&dummy, &dummy, False);
	_rdpdr_check_fds(rfds, wfds, timed_out);
}


/* Abort a pending io request for a given handle and major */
RD_BOOL
rdpdr_abort_io(uint32 fd, uint32 major, RD_NTSTATUS status)
{
	uint32 result;
	struct async_iorequest *iorq;
	struct async_iorequest *prev;

	iorq = g_iorequest;
	prev = NULL;
	while (iorq != NULL)
	{
		/* Only remove from table when major is not set, or when correct major is supplied.
		   Abort read should not abort a write io request. */
		if ((iorq->fd == fd) && (major == 0 || iorq->major == major))
		{
			result = 0;
			rdpdr_send_completion(iorq->device, iorq->id, status, result, (uint8 *) "",
					      1);

			iorq = rdpdr_remove_iorequest(prev, iorq);
			return True;
		}

		prev = iorq;
		iorq = iorq->next;
	}

	return False;
}
