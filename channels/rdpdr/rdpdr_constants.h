/*
   FreeRDP: A Remote Desktop Protocol client.
   Device Redirection Constants

   Copyright 2009 Marc-Andre Moreau <marcandre.moreau@gmail.com>

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

#ifndef __RDPDR_CONSTANTS_H
#define __RDPDR_CONSTANTS_H

/* RDPDR_HEADER.Component */
#define RDPDR_CTYP_CORE 0x4472 // "sD" "Ds" ???
#define RDPDR_CTYP_PRN  0x5052 // "RP" "PR" (PR)inting

/* RDPDR_HEADER.PacketId */
#define PAKID_CORE_SERVER_ANNOUNCE      0x496E // "nI" "nI" ???
#define PAKID_CORE_CLIENTID_CONFIRM     0x4343 // "CC" "CC" (C)lientID (C)onfirm
#define PAKID_CORE_CLIENT_NAME          0x434E // "NC" "CN" (C)lient (N)ame
#define PAKID_CORE_DEVICELIST_ANNOUNCE  0x4441 // "AD" "DA" (D)evice (A)nnounce
#define PAKID_CORE_DEVICE_REPLY         0x6472 // "rd" "dr" (d)evice (r)eply
#define PAKID_CORE_DEVICE_IOREQUEST     0x4952 // "RI" "IR" (I)O (R)equest
#define PAKID_CORE_DEVICE_IOCOMPLETION  0x4943 // "CI" "IC" (I)O (C)ompletion
#define PAKID_CORE_SERVER_CAPABILITY    0x5350 // "PS" "SP" (S)erver (C)apability
#define PAKID_CORE_CLIENT_CAPABILITY    0x4350 // "PC" "CP" (C)lient (C)apability
#define PAKID_CORE_DEVICELIST_REMOVE    0x444D // "MD" "DM" (D)evice list (R)emove
#define PAKID_PRN_CACHE_DATA            0x5043 // "CP" "PC" (P)rinter (C)ache data
#define PAKID_CORE_USER_LOGGEDON        0x554C // "LU" "UL" (U)ser (L)ogged on
#define PAKID_PRN_USING_XPS             0x5543 // "CU" "UC" (U)sing (?)XPS

/* CAPABILITY_HEADER.CapabilityType */
#define CAP_GENERAL_TYPE     0x0001
#define CAP_PRINTER_TYPE     0x0002
#define CAP_PORT_TYPE        0x0003
#define CAP_DRIVE_TYPE       0x0004
#define CAP_SMARTCARD_TYPE   0x0005

/* CAPABILITY_HEADER.Version */
#define GENERAL_CAPABILITY_VERSION_01   0x00000001
#define GENERAL_CAPABILITY_VERSION_02   0x00000002
#define PRINT_CAPABILITY_VERSION_01     0x00000001
#define PORT_CAPABILITY_VERSION_01      0x00000001
#define DRIVE_CAPABILITY_VERSION_01     0x00000001
#define DRIVE_CAPABILITY_VERSION_02     0x00000002
#define SMARTCARD_CAPABILITY_VERSION_01 0x00000001

/* DEVICE_ANNOUNCE.DeviceType */
#define RDPDR_DTYP_SERIAL               0x00000001
#define RDPDR_DTYP_PARALLEL             0x00000002
#define RDPDR_DTYP_PRINT                0x00000004
#define RDPDR_DTYP_FILESYSTEM           0x00000008
#define RDPDR_DTYP_SMARTCARD            0x00000020

/* DR_DEVICE_IOREQUEST.MajorFunction */
#define IRP_MJ_CREATE                   0x00000000
#define IRP_MJ_CLOSE                    0x00000002
#define IRP_MJ_READ                     0x00000003
#define IRP_MJ_WRITE                    0x00000004
#define IRP_MJ_DEVICE_CONTROL           0x0000000E
#define IRP_MJ_QUERY_VOLUME_INFORMATION 0x0000000A
#define IRP_MJ_SET_VOLUME_INFORMATION   0x0000000B
#define IRP_MJ_QUERY_INFORMATION        0x00000005
#define IRP_MJ_SET_INFORMATION          0x00000006
#define IRP_MJ_DIRECTORY_CONTROL        0x0000000C
#define IRP_MJ_LOCK_CONTROL             0x00000011

/* DR_DEVICE_IOREQUEST.MinorFunction */
#define IRP_MN_QUERY_DIRECTORY          0x00000001
#define IRP_MN_NOTIFY_CHANGE_DIRECTORY  0x00000002

/* DR_CREATE_REQ.CreateDisposition */
#define FILE_SUPERSEDE                  0x00000000
#define FILE_OPEN                       0x00000001
#define FILE_CREATE                     0x00000002
#define FILE_OPEN_IF                    0x00000003
#define FILE_OVERWRITE                  0x00000004
#define FILE_OVERWRITE_IF               0x00000005

/* DR_CREATE_REQ.CreateOptions [MS-SMB2] */
#define FILE_DIRECTORY_FILE             0x00000001
#define FILE_NON_DIRECTORY_FILE         0x00000040
#define FILE_COMPLETE_IF_OPLOCKED       0x00000100
#define FILE_DELETE_ON_CLOSE            0x00001000
#define FILE_OPEN_FOR_FREE_SPACE_QUERY  0x00800000

/* DR_CREATE_REQ.DesiredAccess [MS-SMB2] */
#define FILE_READ_DATA                  0x00000001
#define FILE_WRITE_DATA                 0x00000002
#define FILE_APPEND_DATA                0x00000004
#define FILE_READ_EA                    0x00000008
#define FILE_WRITE_EA                   0x00000010
#define FILE_EXECUTE                    0x00000020
#define FILE_READ_ATTRIBUTES            0x00000080
#define FILE_WRITE_ATTRIBUTES           0x00000100
#define DELETE                          0x00010000
#define READ_CONTROL                    0x00020000
#define WRITE_DAC                       0x00040000
#define WRITE_OWNER                     0x00080000
#define SYNCHRONIZE                     0x00100000
#define ACCESS_SYSTEM_SECURITY          0x01000000
#define MAXIMUM_ALLOWED                 0x02000000
#define GENERIC_ALL                     0x10000000
#define GENERIC_EXECUTE                 0x20000000
#define GENERIC_WRITE                   0x40000000
#define GENERIC_READ                    0x80000000

/* DR_CREATE_RSP.Information */
/* DR_DRIVE_CREATE_RSP.DeviceCreateResponse */
#define FILE_SUPERSEDED                 0x00000000
#define FILE_OPENED                     0x00000001
#define FILE_OVERWRITTEN                0x00000003

/* DR_CORE_CLIENT_ANNOUNCE_RSP.VersionMinor */
#define RDPDR_MINOR_RDP_VERSION_5_0     0x0002
#define RDPDR_MINOR_RDP_VERSION_5_1     0x0005
#define RDPDR_MINOR_RDP_VERSION_5_2     0x000A
#define RDPDR_MINOR_RDP_VERSION_6_X     0x000C

/* DR_CORE_CLIENT_NAME_REQ.UnicodeFlag */
#define RDPDR_CLIENT_NAME_UNICODE       0x00000001
#define RDPDR_CLIENT_NAME_ASCII         0x00000000

/* GENERAL_CAPS_SET.ioCode1 */
#define RDPDR_IRP_MJ_CREATE                   0x00000001
#define RDPDR_IRP_MJ_CLEANUP                  0x00000002
#define RDPDR_IRP_MJ_CLOSE                    0x00000004
#define RDPDR_IRP_MJ_READ                     0x00000008
#define RDPDR_IRP_MJ_WRITE                    0x00000010
#define RDPDR_IRP_MJ_FLUSH_BUFFERS            0x00000020
#define RDPDR_IRP_MJ_SHUTDOWN                 0x00000040
#define RDPDR_IRP_MJ_DEVICE_CONTROL           0x00000080
#define RDPDR_IRP_MJ_QUERY_VOLUME_INFORMATION 0x00000100
#define RDPDR_IRP_MJ_SET_VOLUME_INFORMATION   0x00000200
#define RDPDR_IRP_MJ_QUERY_INFORMATION        0x00000400
#define RDPDR_IRP_MJ_SET_INFORMATION          0x00000800
#define RDPDR_IRP_MJ_DIRECTORY_CONTROL        0x00001000
#define RDPDR_IRP_MJ_LOCK_CONTROL             0x00002000
#define RDPDR_IRP_MJ_QUERY_SECURITY           0x00004000
#define RDPDR_IRP_MJ_SET_SECURITY             0x00008000

/* GENERAL_CAPS_SET.extendedPDU */
#define RDPDR_DEVICE_REMOVE_PDUS        0x00000001
#define RDPDR_CLIENT_DISPLAY_NAME_PDU   0x00000002
#define RDPDR_USER_LOGGEDON_PDU         0x00000004

/* GENERAL_CAPS_SET.extraFlags1 */
#define ENABLE_ASYNCIO                  0x00000001

/* DR_DRIVE_LOCK_REQ.Operation */
#define RDP_LOWIO_OP_SHAREDLOCK         0x00000002
#define RDP_LOWIO_OP_EXCLUSIVELOCK      0x00000003
#define RDP_LOWIO_OP_UNLOCK             0x00000004
#define RDP_LOWIO_OP_UNLOCK_MULTIPLE    0x00000005

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

#define RDPDR_MAX_DEVICES               0x10

#define RDPDR_PRINTER_ANNOUNCE_FLAG_ASCII		0x00000001
#define RDPDR_PRINTER_ANNOUNCE_FLAG_DEFAULTPRINTER	0x00000002
#define RDPDR_PRINTER_ANNOUNCE_FLAG_NETWORKPRINTER	0x00000004
#define RDPDR_PRINTER_ANNOUNCE_FLAG_TSPRINTER		0x00000008
#define RDPDR_PRINTER_ANNOUNCE_FLAG_XPSFORMAT		0x00000010

/* Smartcard constants */
#define SCARD_LOCK_TCP		0
#define SCARD_LOCK_SEC		1
#define SCARD_LOCK_CHANNEL	2
#define SCARD_LOCK_RDPDR	3
#define SCARD_LOCK_LAST		4

/* IO constants */
#define RDPDR_ABORT_IO_NONE		0
#define RDPDR_ABORT_IO_WRITE	1
#define RDPDR_ABORT_IO_READ		2

/* [MS-FSCC] FileAttributes */
#define FILE_ATTRIBUTE_ARCHIVE              0x00000020
#define FILE_ATTRIBUTE_COMPRESSED           0x00000800
#define FILE_ATTRIBUTE_DIRECTORY            0x00000010
#define FILE_ATTRIBUTE_ENCRYPTED            0x00004000
#define FILE_ATTRIBUTE_HIDDEN               0x00000002
#define FILE_ATTRIBUTE_NORMAL               0x00000080
#define FILE_ATTRIBUTE_NOT_CONTENT_INDEXED  0x00002000
#define FILE_ATTRIBUTE_OFFLINE              0x00001000
#define FILE_ATTRIBUTE_READONLY             0x00000001
#define FILE_ATTRIBUTE_REPARSE_POINT        0x00000400
#define FILE_ATTRIBUTE_SPARSE_FILE          0x00000200
#define FILE_ATTRIBUTE_SYSTEM               0x00000004
#define FILE_ATTRIBUTE_TEMPORARY            0x00000100

/* [MS-FSCC] FSCTL Structures */
#define FSCTL_GET_REPARSE_POINT                 0x900a8
#define FSCTL_GET_RETRIEVAL_POINTERS            0x90073
#define FSCTL_IS_PATHNAME_VALID                 0x9002c
#define FSCTL_LMR_SET_LINK_TRACKING_INFORMATION 0x1400ec
#define FSCTL_PIPE_PEEK                         0x11400c
#define FSCTL_PIPE_TRANSCEIVE                   0x11c017
#define FSCTL_PIPE_WAIT                         0x110018
#define FSCTL_QUERY_FAT_BPB                     0x90058
#define FSCTL_QUERY_ALLOCATED_RANGES            0x940cf
#define FSCTL_QUERY_ON_DISK_VOLUME_INFO         0x9013c
#define FSCTL_QUERY_SPARING_INFO                0x90138
#define FSCTL_READ_FILE_USN_DATA                0x900eb
#define FSCTL_RECALL_FILE                       0x90117
#define FSCTL_SET_COMPRESSION                   0x9c040
#define FSCTL_SET_DEFECT_MANAGEMENT             0x98134
#define FSCTL_SET_ENCRYPTION                    0x900D7
#define FSCTL_SET_OBJECT_ID                     0x90098
#define FSCTL_SET_OBJECT_ID_EXTENDED            0x900bc
#define FSCTL_SET_REPARSE_POINT                 0x900a4
#define FSCTL_SET_SPARSE                        0x900c4
#define FSCTL_SET_ZERO_DATA                     0x980c8
#define FSCTL_SET_ZERO_ON_DEALLOCATION          0x90194
#define FSCTL_SIS_COPYFILE                      0x90100
#define FSCTL_WRITE_USN_CLOSE_RECORD            0x900ef

/* [MS-FSCC] FileFsAttributeInformation.FileSystemAttributes */
#define FILE_SUPPORTS_USN_JOURNAL           0x02000000
#define FILE_SUPPORTS_OPEN_BY_FILE_ID       0x01000000
#define FILE_SUPPORTS_EXTENDED_ATTRIBUTES   0x00800000
#define FILE_SUPPORTS_HARD_LINKS            0x00400000
#define FILE_SUPPORTS_TRANSACTIONS          0x00200000
#define FILE_SEQUENTIAL_WRITE_ONCE          0x00100000
#define FILE_READ_ONLY_VOLUME               0x00080000
#define FILE_NAMED_STREAMS                  0x00040000
#define FILE_SUPPORTS_ENCRYPTION            0x00020000
#define FILE_SUPPORTS_OBJECT_IDS            0x00010000
#define FILE_VOLUME_IS_COMPRESSED           0x00008000
#define FILE_SUPPORTS_REMOTE_STORAGE        0x00000100
#define FILE_SUPPORTS_REPARSE_POINTS        0x00000080
#define FILE_SUPPORTS_SPARSE_FILES          0x00000040
#define FILE_VOLUME_QUOTAS                  0x00000020
#define FILE_FILE_COMPRESSION               0x00000010
#define FILE_PERSISTENT_ACLS                0x00000008
#define FILE_UNICODE_ON_DISK                0x00000004
#define FILE_CASE_PRESERVED_NAMES           0x00000002
#define FILE_CASE_SENSITIVE_SEARCH          0x00000001

/* [MS-FSCC] FileFsDeviceInformation.DeviceType */
#define FILE_DEVICE_CD_ROM                  0x00000002
#define FILE_DEVICE_DISK                    0x00000007

/* [MS-FSCC] FileFsDeviceInformation.Characteristics */
#define FILE_REMOVABLE_MEDIA                0x00000001
#define FILE_READ_ONLY_DEVICE               0x00000002
#define FILE_FLOPPY_DISKETTE                0x00000004
#define FILE_WRITE_ONCE_MEDIA               0x00000008
#define FILE_REMOTE_DEVICE                  0x00000010
#define FILE_DEVICE_IS_MOUNTED              0x00000020
#define FILE_VIRTUAL_VOLUME                 0x00000040
#define FILE_DEVICE_SECURE_OPEN             0x00000100

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

#endif /* __CONSTANTS_RDPDR_H */
