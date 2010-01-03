
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <netdb.h>
#include <unistd.h>
#include <sys/un.h>
#include "types_ui.h"
#include "vchan.h"
#include "chan_stream.h"

#define SNDC_CLOSE         1
#define SNDC_WAVE          2
#define SNDC_SETVOLUME     3
#define SNDC_SETPITCH      4
#define SNDC_WAVECONFIRM   5
#define SNDC_TRAINING      6
#define SNDC_FORMATS       7
#define SNDC_CRYPTKEY      8
#define SNDC_WAVEENCRYPT   9
#define SNDC_UDPWAVE       10
#define SNDC_UDPWAVELAST   11
#define SNDC_QUALITYMODE   12

#define TSSNDCAPS_ALIVE  1
#define TSSNDCAPS_VOLUME 2
#define TSSNDCAPS_PITCH  4

struct _RD_WAVEFORMATEX
{
	uint16 wFormatTag;
	uint16 nChannels;
	uint32 nSamplesPerSec;
	uint32 nAvgBytesPerSec;
	uint16 nBlockAlign;
	uint16 wBitsPerSample;
	uint16 cbSize;
};
typedef struct _RD_WAVEFORMATEX RD_WAVEFORMATEX;


struct wait_obj
{
	int sock;
	struct sockaddr_un sa;
};

struct data_in_item
{
	struct data_in_item * next;
	char * data;
	int data_size;
};

static CHANNEL_ENTRY_POINTS g_ep;
static void * g_han;
static CHANNEL_DEF g_channel_def[2];
static uint32 g_open_handle[2];
static char * g_data_in[2];
static int g_data_in_size[2];
static int g_data_in_read[2];
static struct wait_obj g_term_event;
static struct wait_obj g_data_in_event;
static struct wait_obj g_thread_done_event;
static struct data_in_item * volatile g_list_head;
static struct data_in_item * volatile g_list_tail;
/* for locking the linked list */
static pthread_mutex_t * g_mutex;

static int g_cBlockNo;

static int
init_wait_obj(struct wait_obj * obj, const char * name)
{
	int pid;
	int size;

	pid = getpid();
	obj->sock = socket(PF_UNIX, SOCK_DGRAM, 0);
	if (obj->sock < 0)
	{
		printf("init_wait_obj: socket failed\n");
		return 1;
	}
	obj->sa.sun_family = AF_UNIX;
	size = sizeof(obj->sa.sun_path) - 1;
	snprintf(obj->sa.sun_path, size, "/tmp/%s%8.8x", name, pid);
	obj->sa.sun_path[size] = 0;
	size = sizeof(obj->sa);
	if (bind(obj->sock, (struct sockaddr*)(&(obj->sa)), size) < 0)
	{
		printf("init_wait_obj: bind failed\n");
		close(obj->sock);
		obj->sock = -1;
		unlink(obj->sa.sun_path);
		return 1;
	}
	return 0;
}

static int
deinit_wait_obj(struct wait_obj * obj)
{
	if (obj->sock != -1)
	{
		close(obj->sock);
		obj->sock = -1;
		unlink(obj->sa.sun_path);
	}
	return 0;
}

static int
set_wait_obj(struct wait_obj * obj)
{
	int len;

	len = sendto(obj->sock, "sig", 4, 0, (struct sockaddr*)(&(obj->sa)),
		sizeof(obj->sa));
	if (len != 4)
	{
		printf("set_wait_obj: error\n");
		return 1;
	}
	return 0;
}

static int
is_wait_obj_set(struct wait_obj * obj)
{
	fd_set rfds;
	int num_set;
	struct timeval time;

	FD_ZERO(&rfds);
	FD_SET(obj->sock, &rfds);
	memset(&time, 0, sizeof(time));
	num_set = select(obj->sock + 1, &rfds, 0, 0, &time);
	return (num_set == 1);
}

static int
clear_wait_obj(struct wait_obj * obj)
{
	int len;

	while (is_wait_obj_set(obj))
	{
		len = recvfrom(obj->sock, &len, 4, 0, 0, 0);
		if (len != 4)
		{
			printf("chan_man_clear_ev: error\n");
			return 1;
		}
	}
	return 0;
}

static int
wait(int timeout, int numr, int * listr)
{
	int max;
	int rv;
	int index;
	int sock;
	struct timeval time;
	struct timeval * ptime;
	fd_set fds;

	ptime = 0;
	if (timeout >= 0)
	{
		time.tv_sec = timeout / 1000;
		time.tv_usec = (timeout * 1000) % 1000000;
		ptime = &time;
	}
	max = 0;
	FD_ZERO(&fds);
	for (index = 0; index < numr; index++)
	{
		sock = listr[index];
		FD_SET(sock, &fds);
		if (sock > max)
		{
			max = sock;
		}
	}
	rv = select(max + 1, &fds, 0, 0, ptime);
	return rv;
}

/* called by main thread
   add item to linked list and inform worker thread that there is data */
static void
signal_data_in(void)
{
	struct data_in_item * item;

	item = (struct data_in_item *) malloc(sizeof(struct data_in_item));
	item->next = 0;
	item->data = g_data_in[0];
	g_data_in[0] = 0;
	item->data_size = g_data_in_size[0];
	g_data_in_size[0] = 0;
	pthread_mutex_lock(g_mutex);
	if (g_list_tail == 0)
	{
		g_list_head = item;
		g_list_tail = item;
	}
	else
	{
		g_list_tail->next = item;
		g_list_tail = item;
	}
	pthread_mutex_unlock(g_mutex);
	set_wait_obj(&g_data_in_event);
}

static int
thread_process_message_formats(char * data, int data_size)
{
	int index;
	int format_count;
	int wf_size;
	int out_format_count;
	int size;
	char * ldata;
	char * out_data;
	RD_WAVEFORMATEX * formats;
	RD_WAVEFORMATEX wf;
	uint32 error;

	/* skip:
		dwFlags (4 bytes),
		dwVolume (4 bytes),
		dwPitch (4 bytes),
		wDGramPort (2 bytes) */
	format_count = GET_UINT16(data, 14); /* wNumberOfFormats */
	if ((format_count < 1) || (format_count > 1000))
	{
		printf("thread_process_message_formats: bad format_count %d\n",
			format_count);
		return 1;
	}
	g_cBlockNo = GET_UINT8(data, 16); /* cLastBlockConfirmed */
	/* skip:
		wVersion (2 bytes)
		bPad (1 byte) */
	/* remainder is sndFormats (variable) */
	ldata = data + 20;
	out_format_count = 0;
	wf_size = 18; /* packed size of wf */
	size = sizeof(RD_WAVEFORMATEX) * format_count;
	formats = (RD_WAVEFORMATEX *) malloc(size);
	memset(&wf, 0, sizeof(wf));
	for (index = 0; index < format_count; index++)
	{
		if ((ldata + wf_size) > (data + data_size))
		{
			printf("thread_process_message_formats: error\n");
			break;
		}
		wf.wFormatTag = GET_UINT16(ldata, 0);
		wf.nChannels = GET_UINT16(ldata, 2);
		wf.nSamplesPerSec = GET_UINT32(ldata, 4);
		wf.nAvgBytesPerSec = GET_UINT32(ldata, 8);
		wf.nBlockAlign = GET_UINT16(ldata, 12);
		wf.wBitsPerSample = GET_UINT16(ldata, 14);
		wf.cbSize = GET_UINT16(ldata, 16);
		ldata += wf.cbSize;
		ldata += wf_size;
		//if (alsa_format_supported(&wf))
		if (index == 0)
		{
			formats[out_format_count] = wf;
			out_format_count++;
		}
	}
	size = 24 + out_format_count * wf_size;
	out_data = (char *) malloc(size + 16);
	SET_UINT8(out_data, 0, SNDC_FORMATS); /* Header (4 bytes) */
	SET_UINT8(out_data, 1, 0);
	SET_UINT16(out_data, 2, size - 4);
	SET_UINT32(out_data, 4, TSSNDCAPS_ALIVE | TSSNDCAPS_VOLUME); /* dwFlags */
	SET_UINT32(out_data, 8, 0xffffffff); /* dwVolume */
	SET_UINT32(out_data, 12, 0); /* dwPitch */
	SET_UINT16(out_data, 16, 0); /* wDGramPort */
	SET_UINT16(out_data, 18, out_format_count); /* wNumberOfFormats */
	SET_UINT8(out_data, 20, 0); /* padding */
	SET_UINT16(out_data, 21, 2); /* version */
	SET_UINT8(out_data, 23, 0); /* padding */
	ldata = out_data + 24;
	for (index = 0; index < out_format_count; index++)
	{
		wf = formats[index];
		SET_UINT16(ldata, 0, wf.wFormatTag);
		SET_UINT16(ldata, 2, wf.nChannels);
		SET_UINT32(ldata, 4, wf.nSamplesPerSec);
		SET_UINT32(ldata, 8, wf.nAvgBytesPerSec);
		SET_UINT16(ldata, 12, wf.nBlockAlign);
		SET_UINT16(ldata, 14, wf.wBitsPerSample);
		SET_UINT16(ldata, 16, wf.cbSize);
		ldata += wf.cbSize;
		ldata += wf_size;
	}
	free(formats);
	error = g_ep.pVirtualChannelWrite(g_open_handle[0],
		out_data, size, out_data);
	if (error != CHANNEL_RC_OK)
	{
		printf("thread_process_message_formats: VirtualChannelWrite "
			"failed %d\n", error);
		return 1;
	}
	return 0;
}

static int
thread_process_message_training(char * data, int data_size)
{
	int wTimeStamp;
	int wPackSize;
	int size;
	char * out_data;
	uint32 error;

	wTimeStamp = GET_UINT16(data, 0);
	wPackSize = GET_UINT16(data, 2);
	if (wPackSize != data_size - 4)
	{
		printf("thread_process_message_training: size error\n");
	}
	size = 8;
	out_data = (char *) malloc(size);
	SET_UINT16(out_data, 0, SNDC_TRAINING | 0x2300);
	SET_UINT16(out_data, 2, size - 4);
	SET_UINT16(out_data, 4, wTimeStamp);
	SET_UINT16(out_data, 6, 0);
	error = g_ep.pVirtualChannelWrite(g_open_handle[0],
		out_data, size, out_data);
	if (error != CHANNEL_RC_OK)
	{
		printf("thread_process_message_training: VirtualChannelWrite "
			"failed %d\n", error);
		return 1;
	}
	return 0;
}

static int
thread_process_message_wave(char * data, int data_size)
{
	return 0;
}

static int
thread_process_message_close(char * data, int data_size)
{
	return 0;
}

static int
thread_process_message_setvolume(char * data, int data_size)
{
	return 0;
}

static int
thread_process_message(char * data, int data_size)
{
	int opcode;
	int size;

	opcode = GET_UINT8(data, 0);
	size = GET_UINT16(data, 2);
	printf("thread_process_message: data_size %d opcode %d size %d\n",
		data_size, opcode, size);
	switch (opcode)
	{
		case SNDC_FORMATS:
			thread_process_message_formats(data + 4, size);
			break;
		case SNDC_TRAINING:
			thread_process_message_training(data + 4, size);
			break;
		case SNDC_WAVE:
			thread_process_message_wave(data + 4, size);
			break;
		case SNDC_CLOSE:
			thread_process_message_close(data + 4, size);
			break;
		case SNDC_SETVOLUME:
			thread_process_message_setvolume(data + 4, size);
			break;
		default:
			printf("thread_process_message: unknown opcode\n");
			break;
	}
	return 0;
}

/* process the linked list of data that has come in */
static int
thread_process_data(void)
{
	char * data;
	int data_size;
	struct data_in_item * item;

	while (1)
	{
		pthread_mutex_lock(g_mutex);
		if (g_list_head == 0)
		{
			pthread_mutex_unlock(g_mutex);
			break;
		}
		data = g_list_head->data;
		data_size = g_list_head->data_size;
		item = g_list_head;
		g_list_head = g_list_head->next;
		if (g_list_head == 0)
		{
			g_list_tail = 0;
		}
		pthread_mutex_unlock(g_mutex);
		thread_process_message(data, data_size);
		if (data != 0)
		{
			free(data);
		}
		if (item != 0)
		{
			free(item);
		}
	}
	return 0;
}

static void *
thread_func(void * arg)
{
	int listr[2];
	int numr;

	printf("thread_func: in\n");
	while (1)
	{
		listr[0] = g_term_event.sock;
		listr[1] = g_data_in_event.sock;
		numr = 2;
		wait(-1, numr, listr);
		if (is_wait_obj_set(&g_term_event))
		{
			break;
		}
		if (is_wait_obj_set(&g_data_in_event))
		{
			clear_wait_obj(&g_data_in_event);
			/* process data in */
			thread_process_data();
		}
	}
	set_wait_obj(&g_thread_done_event);
	printf("thread_func: out\n");
	return 0;
}

static void
OpenEventProcessReceived(uint32 openHandle, void * pData, uint32 dataLength,
	uint32 totalLength, uint32 dataFlags)
{
	int index;

	index = (openHandle == g_open_handle[0]) ? 0 : 1;
	printf("OpenEventProcessReceived: receive openHandle %d dataLength %d "
		"totalLength %d dataFlags %d\n",
		openHandle, dataLength, totalLength, dataFlags);
	if (dataFlags & CHANNEL_FLAG_FIRST)
	{
		g_data_in_read[index] = 0;
		if (g_data_in[index] != 0)
		{
			free(g_data_in[index]);
		}
		g_data_in[index] = (char *) malloc(totalLength);
		g_data_in_size[index] = totalLength;
	}
	memcpy(g_data_in[index] + g_data_in_read[index], pData, dataLength);
	g_data_in_read[index] += dataLength;
	if (dataFlags & CHANNEL_FLAG_LAST)
	{
		if (g_data_in_read[index] != g_data_in_size[index])
		{
			printf("OpenEventProcessReceived: read error\n");
		}
		if (index == 0)
		{
			signal_data_in();
		}
	}
}

static void
OpenEvent(uint32 openHandle, uint32 event, void * pData, uint32 dataLength,
	uint32 totalLength, uint32 dataFlags)
{
	printf("OpenEvent: event %d\n", event);
	switch (event)
	{
		case CHANNEL_EVENT_DATA_RECEIVED:
			OpenEventProcessReceived(openHandle, pData, dataLength,
				totalLength, dataFlags);
			break;
		case CHANNEL_EVENT_WRITE_COMPLETE:
			free(pData);
			break;
	}
}

static void
InitEventProcessConnected(void * pInitHandle, void * pData, uint32 dataLength)
{
	uint32 error;
	pthread_t thread;

	if (pInitHandle != g_han)
	{
		printf("InitEventProcessConnected: error no match\n");
	}
	error = g_ep.pVirtualChannelOpen(g_han, &(g_open_handle[0]),
		"rdpsnd", OpenEvent);
	if (error != CHANNEL_RC_OK)
	{
		printf("InitEventProcessConnected: Open failed\n");
	}
	error = g_ep.pVirtualChannelOpen(g_han, &(g_open_handle[1]),
		"snddbg", OpenEvent);
	if (error != CHANNEL_RC_OK)
	{
		printf("InitEventProcessConnected: Open failed\n");
	}
	pthread_create(&thread, 0, thread_func, 0);
	pthread_detach(thread);
}

static void
InitEventProcessTerminated(void)
{
	int listr[1];

	set_wait_obj(&g_term_event);
	listr[0] = g_thread_done_event.sock;
	wait(-1, 1, listr);
	deinit_wait_obj(&g_term_event);
	deinit_wait_obj(&g_data_in_event);
	deinit_wait_obj(&g_thread_done_event);
}

static void
InitEvent(void * pInitHandle, uint32 event, void * pData, uint32 dataLength)
{
	printf("InitEvent: event %d\n", event);
	switch (event)
	{
		case CHANNEL_EVENT_CONNECTED:
			InitEventProcessConnected(pInitHandle, pData, dataLength);
			break;
		case CHANNEL_EVENT_DISCONNECTED:
			break;
		case CHANNEL_EVENT_TERMINATED:
			InitEventProcessTerminated();
			break;
	}
}

/* this registers two channels but only rdpsnd is used */
int
VirtualChannelEntry(PCHANNEL_ENTRY_POINTS pEntryPoints)
{
	printf("VirtualChannelEntry:\n");
	g_data_in_size[0] = 0;
	g_data_in_size[1] = 0;
	g_data_in[0] = 0;
	g_data_in[1] = 0;
	g_ep = *pEntryPoints;
	memset(&(g_channel_def[0]), 0, sizeof(g_channel_def));
	g_channel_def[0].options = CHANNEL_OPTION_INITIALIZED |
		CHANNEL_OPTION_ENCRYPT_RDP;
	strcpy(g_channel_def[0].name, "rdpsnd");
	g_channel_def[1].options = CHANNEL_OPTION_INITIALIZED |
		CHANNEL_OPTION_ENCRYPT_RDP;
	strcpy(g_channel_def[1].name, "snddbg");
	g_mutex = (pthread_mutex_t *) malloc(sizeof(pthread_mutex_t));
	pthread_mutex_init(g_mutex, 0);
	g_list_head = 0;
	g_list_tail = 0;
	init_wait_obj(&g_term_event, "freerdprdpsndterm");
	init_wait_obj(&g_data_in_event, "freerdprdpsnddatain");
	init_wait_obj(&g_thread_done_event, "freerdprdpsndtdone");
	g_ep.pVirtualChannelInit(&g_han, g_channel_def, 2,
		VIRTUAL_CHANNEL_VERSION_WIN2000, InitEvent);
	return 1;
}
