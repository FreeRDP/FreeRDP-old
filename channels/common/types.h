

#define MAX_CBSIZE 256

/* RDPSND */
typedef struct _RD_WAVEFORMATEX
{
	uint16 wFormatTag;
	uint16 nChannels;
	uint32 nSamplesPerSec;
	uint32 nAvgBytesPerSec;
	uint16 nBlockAlign;
	uint16 wBitsPerSample;
	uint16 cbSize;
	uint8 cb[MAX_CBSIZE];
} RD_WAVEFORMATEX;


/* RDPDR */
typedef uint32 RD_NTSTATUS;
typedef uint32 RD_NTHANDLE;

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


typedef struct rdpdr_device_info
{
	uint32 deviceType;
	RD_NTHANDLE handle;
	char name[8];
	char *local_path;
	void *pdevice_data;
}
RDPDR_DEVICE;

typedef struct rdpdr_serial_device_info
{
	int dtr;
	int rts;
	uint32 control, xonoff, onlimit, offlimit;
	uint32 baud_rate,
		queue_in_size,
		queue_out_size,
		wait_mask,
		read_interval_timeout,
		read_total_timeout_multiplier,
		read_total_timeout_constant,
		write_total_timeout_multiplier, write_total_timeout_constant, posix_wait_mask;
	uint8 stop_bits, parity, word_length;
	uint8 chars[6];
	struct termios *ptermios, *pold_termios;
	int event_txempty, event_cts, event_dsr, event_rlsd, event_pending;
}
SERIAL_DEVICE;

typedef struct rdpdr_parallel_device_info
{
	char *driver, *printer;
	uint32 queue_in_size,
		queue_out_size,
		wait_mask,
		read_interval_timeout,
		read_total_timeout_multiplier,
		read_total_timeout_constant,
		write_total_timeout_multiplier,
		write_total_timeout_constant, posix_wait_mask, bloblen;
	uint8 *blob;
}
PARALLEL_DEVICE;

typedef struct rdpdr_printer_info
{
	FILE *printer_fp;
	char *driver, *printer;
	uint32 bloblen;
	uint8 *blob;
	RD_BOOL default_printer;
}
PRINTER;

typedef struct notify_data
{
	time_t modify_time;
	time_t status_time;
	time_t total_time;
	unsigned int num_entries;
}
NOTIFY;

#ifndef PATH_MAX
#define PATH_MAX 256
#endif

typedef struct fileinfo
{
	uint32 device_id, flags_and_attributes, accessmask;
	char path[PATH_MAX];
	//DIR *pdir;
	struct dirent *pdirent;
	char pattern[PATH_MAX];
	RD_BOOL delete_on_close;
	NOTIFY notify;
	uint32 info_class;
}
FILEINFO;

typedef RD_BOOL(*str_handle_lines_t) (const char *line, void *data);

/*
 * The FILETIME structure is a 64-bit value that represents the number of 100-nanosecond intervals
 * that have elapsed since January 1, 1601, in Coordinated Universal Time (UTC) format.
 */

typedef struct FILETIME
{
	uint32 dwLowDateTime;
	uint32 dwHighDateTime;
}
FILETIME;

typedef struct _FILE_BASIC_INFORMATION
{
	FILETIME creationTime;
	FILETIME lastAccessTime;
	FILETIME lastWriteTime;
	FILETIME changeTime;
	uint32 fileAttributes;
	uint32 reserved;
}
FILE_BASIC_INFORMATION;

typedef struct _FILE_STANDARD_INFORMATION
{
	uint32 allocationSizeLow;
	uint32 allocationSizeHigh;
	uint32 endOfFileLow;
	uint32 endOfFileHigh;
	uint32 numberOfLinks;
	uint8 deletePending;
	uint8 directory;
	uint16 reserved;
}
FILE_STANDARD_INFORMATION;

typedef struct _FILE_BOTH_DIR_INFORMATION
{
	uint32 nextEntryOffset;
	uint32 fileIndex;
	FILETIME creationTime;
	FILETIME lastAccessTime;
	FILETIME lastWriteTime;
	FILETIME changeTime;
	uint32 endOfFileLow;
	uint32 endOfFileHigh;
	uint32 allocationSizeLow;
	uint32 allocationSizeHigh;
	uint32 fileAttributes;
	uint32 fileNameLength;
	uint32 eaSize;
	uint8 shortNameLength;
	uint8 reserved;
	char shortName[24];
	char* fileName;
}
FILE_BOTH_DIR_INFORMATION;

typedef struct _FILE_DIRECTORY_INFORMATION
{
	uint32 nextEntryOffset;
	uint32 fileIndex;
	FILETIME creationTime;
	FILETIME lastAccessTime;
	FILETIME lastWriteTime;
	FILETIME changeTime;
	uint32 endOfFileLow;
	uint32 endOfFileHigh;
	uint32 allocationSizeLow;
	uint32 allocationSizeHigh;
	uint32 fileAttributes;
	uint32 fileNameLength;
	char* fileName;
}
FILE_DIRECTORY_INFORMATION;

typedef struct _FILE_FULL_DIR_INFORMATION
{
	uint32 nextEntryOffset;
	uint32 fileIndex;
	FILETIME creationTime;
	FILETIME lastAccessTime;
	FILETIME lastWriteTime;
	FILETIME changeTime;
	uint32 endOfFileLow;
	uint32 endOfFileHigh;
	uint32 allocationSizeLow;
	uint32 allocationSizeHigh;
	uint32 fileAttributes;
	uint32 fileNameLength;
	uint32 eaSize;
	char* fileName;
}
FILE_FULL_DIR_INFORMATION;

typedef struct _FILE_LINK_INFORMATION
{
	uint8 replaceIfExists;
	uint8 reserved1;
	uint32 reserved2;
	uint32 reserved3;
	uint32 rootDirectoryLow;
	uint32 rootDirectoryHigh;
	uint32 fileNameLength;
	char* fileName;
}
FILE_LINK_INFORMATION;

typedef struct _FILE_MODE_INFORMATION
{
	uint32 mode;
}
FILE_MODE_INFORMATION;

typedef struct _FILE_NAME_INFORMATION
{
	uint32 fileNameLength;
	char* filename;
}
FILE_NAME_INFORMATION;

typedef struct _FILE_NAMES_INFORMATION
{
	uint32 nextEntryOffset;
	uint32 fileIndex;
	uint32 fileNameLength;
	char* filename;
}
FILE_NAMES_INFORMATION;

typedef struct _FILE_NETWORK_OPEN_INFORMATION
{
	FILETIME creationTime;
	FILETIME lastAccessTime;
	FILETIME lastWriteTime;
	FILETIME changeTime;
	uint32 allocationSizeLow;
	uint32 allocationSizeHigh;
	uint32 endOfFileLow;
	uint32 endOfFileHigh;
	uint32 fileAttributes;
	uint32 reserved;
}
FILE_NETWORK_OPEN_INFORMATION;
