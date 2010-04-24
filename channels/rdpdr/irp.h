/* -*- c-basic-offset: 8 -*-
   FreeRDP: A Remote Desktop Protocol client.
   Device Redirection - Interrupt Request Packet (IRP) Processing

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

#ifndef __IRP_H
#define __IRP_H

typedef uint32 RD_NTSTATUS;
typedef uint32 RD_NTHANDLE;

#if 0
typedef struct _DEVICE_FNS
{
	RD_NTSTATUS(*create) (uint32 device, uint32 desired_access, uint32 share_mode,
			      uint32 create_disposition, uint32 flags_and_attributes,
			      char *filename, RD_NTHANDLE * handle);
	RD_NTSTATUS(*close) (RD_NTHANDLE handle);
	RD_NTSTATUS(*read) (RD_NTHANDLE handle, uint8 * data, uint32 length, uint32 offset,
			    uint32 * result);
	RD_NTSTATUS(*write) (RD_NTHANDLE handle, uint8 * data, uint32 length, uint32 offset,
			     uint32 * result);
	//RD_NTSTATUS(*device_control) (RD_NTHANDLE handle, uint32 request, STREAM in, STREAM out);
}
DEVICE_FNS;
#endif

enum FILE_INFORMATION_CLASS
{
	FileDirectoryInformation = 1,
	FileFullDirectoryInformation,
	FileBothDirectoryInformation,
	FileBasicInformation,
	FileStandardInformation,
	FileInternalInformation,
	FileEaInformation,
	FileAccessInformation,
	FileNameInformation,
	FileRenameInformation,
	FileLinkInformation,
	FileNamesInformation,
	FileDispositionInformation,
	FilePositionInformation,
	FileFullEaInformation,
	FileModeInformation,
	FileAlignmentInformation,
	FileAllInformation,
	FileAllocationInformation,
	FileEndOfFileInformation,
	FileAlternateNameInformation,
	FileStreamInformation,
	FilePipeInformation,
	FilePipeLocalInformation,
	FilePipeRemoteInformation,
	FileMailslotQueryInformation,
	FileMailslotSetInformation,
	FileCompressionInformation,
	FileCopyOnWriteInformation,
	FileCompletionInformation,
	FileMoveClusterInformation,
	FileOleClassIdInformation,
	FileOleStateBitsInformation,
	FileNetworkOpenInformation,
	FileObjectIdInformation,
	FileOleAllInformation,
	FileOleDirectoryInformation,
	FileContentIndexInformation,
	FileInheritContentIndexInformation,
	FileOleInformation,
	FileMaximumInformation
};

enum FS_INFORMATION_CLASS
{
	FileFsVolumeInformation = 1,
	FileFsLabelInformation,
	FileFsSizeInformation,
	FileFsDeviceInformation,
	FileFsAttributeInformation,
	FileFsControlInformation,
	FileFsFullSizeInformation,
	FileFsObjectIdInformation,
	FileFsDriverPathInformation,
	FileFsMaximumInformation
};

struct _IRP
{
	DEVICE* dev;
	CHANNEL_ENTRY_POINTS ep;
	uint32 open_handle;
	uint32 fileID;
	uint32 completionID;
	uint32 majorFunction;
	uint32 minorFunction;
	RD_BOOL rwBlocking;
	RD_NTHANDLE ioStatus;
	char* buffer;
	int buffer_size;
	int infoClass;
	uint32 desiredAccess;
	uint32 fileAttributes;
	uint32 sharedAccess;
	uint32 createDisposition;
	uint32 createOptions;
};

void
irp_process_create_request(IRP* irp, char* data, int data_size);
void
irp_send_create_response(IRP* irp);
void
irp_process_close_request(IRP* irp, char* data, int data_size);
void
irp_send_close_response(IRP* irp);
void
irp_process_read_request(IRP* irp, char* data, int data_size);
void
irp_send_read_response(IRP* irp);
void
irp_process_write_request(IRP* irp, char* data, int data_size);
void
irp_process_query_volume_information_request(IRP* irp, char* data, int data_size);
void
irp_send_query_volume_information_response(IRP* irp);
void
irp_process_set_volume_information_request(IRP* irp, char* data, int data_size);
void
irp_process_query_information_request(IRP* irp, char* data, int data_size);
void
irp_send_query_information_response(IRP* irp);
void
irp_process_set_information_request(IRP* irp, char* data, int data_size);
void
irp_process_directory_control_request(IRP* irp, char* data, int data_size);
void
irp_process_device_control_request(IRP* irp, char* data, int data_size);
void
irp_process_file_lock_control_request(IRP* irp, char* data, int data_size);
void
irp_process_query_directory_request(IRP* irp, char* data, int data_size);
void
irp_send_query_directory_response(IRP* irp);
void
irp_process_notify_change_directory_request(IRP* irp, char* data, int data_size);

#endif // __IRP_H

