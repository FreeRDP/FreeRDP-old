/*
   Copyright (c) 2009-2010 Jay Sorg

   Permission is hereby granted, free of charge, to any person obtaining a
   copy of this software and associated documentation files (the "Software"),
   to deal in the Software without restriction, including without limitation
   the rights to use, copy, modify, merge, publish, distribute, sublicense,
   and/or sell copies of the Software, and to permit persons to whom the
   Software is furnished to do so, subject to the following conditions:

   The above copyright notice and this permission notice shall be included
   in all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
   OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
   AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
   FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
   DEALINGS IN THE SOFTWARE.

*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <netdb.h>
#include <unistd.h>
#include <sys/un.h>
#include <sys/time.h>
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

#define LOG_LEVEL 1
#define LLOG(_level, _args) \
  do { if (_level < LOG_LEVEL) { printf _args ; } } while (0)
#define LLOGLN(_level, _args) \
  do { if (_level < LOG_LEVEL) { printf _args ; printf("\n"); } } while (0)

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
static struct data_in_item * volatile g_list_head;
static struct data_in_item * volatile g_list_tail;
/* for locking the linked list */
static pthread_mutex_t * g_mutex;

static int g_cBlockNo;
static char * g_supported_formats;
static int g_supported_formats_size;
static int g_current_format;
static int g_expectingWave; /* boolean */
static char g_waveData[4];
static int g_waveDataSize;
static uint32 g_wTimeStamp; /* server timestamp */
static uint32 g_local_time_stamp; /* client timestamp */
static volatile int g_thread_status;

/* implementations are in the hardware file */
int
wave_out_open(void);
int
wave_out_close(void);
int
wave_out_format_supported(char * snd_format, int size);
int
wave_out_set_format(char * snd_format, int size);
int
wave_out_set_volume(uint32 value);
int
wave_out_play(char * data, int size);

/* get time in milliseconds */
static uint32
get_mstime(void)
{
	struct timeval tp;

	gettimeofday(&tp, 0);
	return (tp.tv_sec * 1000) + (tp.tv_usec / 1000);
}

static int
init_wait_obj(struct wait_obj * obj, const char * name)
{
	int pid;
	int size;

	pid = getpid();
	obj->sock = socket(PF_UNIX, SOCK_DGRAM, 0);
	if (obj->sock < 0)
	{
		LLOGLN(0, ("init_wait_obj: socket failed"));
		return 1;
	}
	obj->sa.sun_family = AF_UNIX;
	size = sizeof(obj->sa.sun_path) - 1;
	snprintf(obj->sa.sun_path, size, "/tmp/%s%8.8x", name, pid);
	obj->sa.sun_path[size] = 0;
	size = sizeof(obj->sa);
	if (bind(obj->sock, (struct sockaddr*)(&(obj->sa)), size) < 0)
	{
		LLOGLN(0, ("init_wait_obj: bind failed"));
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
set_wait_obj(struct wait_obj * obj)
{
	int len;

	if (is_wait_obj_set(obj))
	{
		return 0;
	}
	len = sendto(obj->sock, "sig", 4, 0, (struct sockaddr*)(&(obj->sa)),
		sizeof(obj->sa));
	if (len != 4)
	{
		LLOGLN(0, ("set_wait_obj: error"));
		return 1;
	}
	return 0;
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
			LLOGLN(0, ("chan_man_clear_ev: error"));
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

/*
	wFormatTag      2 byte offset 0
	nChannels       2 byte offset 2
	nSamplesPerSec  4 byte offset 4
	nAvgBytesPerSec 4 byte offset 8
	nBlockAlign     2 byte offset 12
	wBitsPerSample  2 byte offset 14
	cbSize          2 byte offset 16
	data            variable offset 18
*/

/* called by worker thread
   receives a list of server supported formats and returns a list
   of client supported formats */
static int
thread_process_message_formats(char * data, int data_size)
{
	int index;
	int format_count;
	int out_format_count;
	int out_format_size;
	int size;
	int flags;
	int version;
	char * ldata;
	char * out_data;
	char * out_formats;
	char * lout_formats;
	uint32 error;

	/* skip:
		dwFlags (4 bytes),
		dwVolume (4 bytes),
		dwPitch (4 bytes),
		wDGramPort (2 bytes) */
	format_count = GET_UINT16(data, 14); /* wNumberOfFormats */
	if ((format_count < 1) || (format_count > 1000))
	{
		LLOGLN(0, ("thread_process_message_formats: bad format_count %d",
			format_count));
		return 1;
	}
	g_cBlockNo = GET_UINT8(data, 16); /* cLastBlockConfirmed */
	version = GET_UINT16(data, 17); /* wVersion */
	if (version < 2)
	{
		LLOGLN(0, ("thread_process_message_formats: warning, old server"));
	}
	/* skip:
		bPad (1 byte) */
	/* setup output buffer */
	size = 32 + data_size;
	out_data = (char *) malloc(size);
	out_formats = out_data + 24;
	lout_formats = out_formats;
	/* remainder is sndFormats (variable) */
	ldata = data + 20;
	out_format_count = 0;
	for (index = 0; index < format_count; index++)
	{
		size = 18 + GET_UINT16(ldata, 16);
		if (wave_out_format_supported(ldata, size))
		{
			memcpy(lout_formats, ldata, size);
			lout_formats += size;
			out_format_count++;
		}
		ldata += size;
	}
	out_format_size = (int) (lout_formats - out_formats);
	if ((out_format_size > 0) && (out_format_count > 0))
	{
		g_supported_formats = (char *) malloc(out_format_size);
		memcpy(g_supported_formats, out_formats, out_format_size);
		g_supported_formats_size = out_format_size;
	}
	else
	{
		LLOGLN(0, ("thread_process_message_formats: error, "
			"no formats supported"));
	}
	size = 24 + out_format_size;
	SET_UINT8(out_data, 0, SNDC_FORMATS); /* Header (4 bytes) */
	SET_UINT8(out_data, 1, 0);
	SET_UINT16(out_data, 2, size - 4);
	flags = TSSNDCAPS_ALIVE | TSSNDCAPS_VOLUME;
	SET_UINT32(out_data, 4, flags); /* dwFlags */
	SET_UINT32(out_data, 8, 0xffffffff); /* dwVolume */
	SET_UINT32(out_data, 12, 0); /* dwPitch */
	SET_UINT16(out_data, 16, 0); /* wDGramPort */
	SET_UINT16(out_data, 18, out_format_count); /* wNumberOfFormats */
	SET_UINT8(out_data, 20, 0); /* cLastBlockConfirmed */
	SET_UINT16(out_data, 21, 2); /* wVersion */
	SET_UINT8(out_data, 23, 0); /* bPad */
	error = g_ep.pVirtualChannelWrite(g_open_handle[0],
		out_data, size, out_data);
	if (error != CHANNEL_RC_OK)
	{
		LLOGLN(0, ("thread_process_message_formats: "
			"VirtualChannelWrite "
			"failed %d", error));
		return 1;
	}
	return 0;
}

/* called by worker thread
   server is getting a feel of the round trip time */
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
	if (wPackSize != 0)
	{
		if ((wPackSize - 4) != data_size)
		{
			LLOGLN(0, ("thread_process_message_training: size error "
				"wPackSize %d data_size %d",
				wPackSize, data_size));
			return 1;
		}
	}
	size = 8;
	out_data = (char *) malloc(size);
	SET_UINT8(out_data, 0, SNDC_TRAINING);
	SET_UINT8(out_data, 1, 0);
	SET_UINT16(out_data, 2, size - 4);
	SET_UINT16(out_data, 4, wTimeStamp);
	SET_UINT16(out_data, 6, wPackSize);
	error = g_ep.pVirtualChannelWrite(g_open_handle[0],
		out_data, size, out_data);
	if (error != CHANNEL_RC_OK)
	{
		LLOGLN(0, ("thread_process_message_training: VirtualChannelWrite "
			"failed %d", error));
		return 1;
	}
	return 0;
}

static int
set_format(void)
{
	char * snd_format;
	int size;
	int index;

	LLOGLN(10, ("set_format:"));
	snd_format = g_supported_formats;
	size = 18 + GET_UINT16(snd_format, 16);
	index = 0;
	while (index < g_current_format)
	{
		snd_format += size;
		size = 18 + GET_UINT16(snd_format, 16);
		index++;
	}
	wave_out_set_format(snd_format, size);
	return 0;
}

static int
thread_process_message_wave_info(char * data, int data_size)
{
	int wFormatNo;

	wave_out_open();
	g_wTimeStamp = GET_UINT16(data, 0); /* time in ms */
	g_local_time_stamp = get_mstime(); /* time in ms */
	wFormatNo = GET_UINT16(data, 2);
	LLOGLN(10, ("thread_process_message_wave_info: data_size %d "
		"wFormatNo %d", data_size, wFormatNo));
	g_cBlockNo = GET_UINT8(data, 4);
	g_waveDataSize = data_size - 8;
	memcpy(g_waveData, data + 8, 4);
	if (wFormatNo != g_current_format)
	{
		g_current_format = wFormatNo;
		set_format();
	}
	g_expectingWave = 1;
	return 0;
}

/* header is not removed from data in this function */
static int
thread_process_message_wave(char * data, int data_size)
{
	int size;
	int wTimeStamp;
	int time_delta;
	char * out_data;
	uint32 error;
	uint32 cur_time;

	g_expectingWave = 0;
	memcpy(data, g_waveData, 4);
	if (data_size != g_waveDataSize)
	{
		LLOGLN(0, ("thread_process_message_wave: "
			"size error"));
	}
	wave_out_play(data, data_size);
	size = 8;
	out_data = (char *) malloc(size);
	SET_UINT8(out_data, 0, SNDC_WAVECONFIRM);
	SET_UINT8(out_data, 1, 0);
	SET_UINT16(out_data, 2, size - 4);
	cur_time = get_mstime();
	time_delta = cur_time - g_local_time_stamp;
	LLOGLN(0, ("thread_process_message_wave: "
		"data_size %d time_delta %d cur_time %u",
		data_size, time_delta, cur_time));
	wTimeStamp = g_wTimeStamp + time_delta;
	SET_UINT16(out_data, 4, wTimeStamp);
	SET_UINT8(out_data, 6, g_cBlockNo);
	SET_UINT8(out_data, 7, 0);
	error = g_ep.pVirtualChannelWrite(g_open_handle[0],
		out_data, size, out_data);
	if (error != CHANNEL_RC_OK)
	{
		LLOGLN(0, ("thread_process_message_wave: "
			"VirtualChannelWrite "
			"failed %d", error));
		return 1;
	}
	return 0;
}

static int
thread_process_message_close(char * data, int data_size)
{
	LLOGLN(10, ("thread_process_message_close: "
		"data_size %d", data_size));
	wave_out_close();
	return 0;
}

static int
thread_process_message_setvolume(char * data, int data_size)
{
	uint32 dwVolume;

	LLOGLN(10, ("thread_process_message_setvolume:"));
	dwVolume = GET_UINT32(data, 0);
	wave_out_set_volume(dwVolume);
	return 0;
}

static int
thread_process_message(char * data, int data_size)
{
	int opcode;
	int size;

	if (g_expectingWave)
	{
		thread_process_message_wave(data, data_size);
		g_expectingWave = 0;
		return 0;
	}
	opcode = GET_UINT8(data, 0);
	size = GET_UINT16(data, 2);
	LLOGLN(10, ("thread_process_message: data_size %d opcode %d size %d",
		data_size, opcode, size));
	switch (opcode)
	{
		case SNDC_FORMATS:
			thread_process_message_formats(data + 4, size);
			break;
		case SNDC_TRAINING:
			thread_process_message_training(data + 4, size);
			break;
		case SNDC_WAVE:
			thread_process_message_wave_info(data + 4, size);
			break;
		case SNDC_CLOSE:
			thread_process_message_close(data + 4, size);
			break;
		case SNDC_SETVOLUME:
			thread_process_message_setvolume(data + 4, size);
			break;
		default:
			LLOGLN(0, ("thread_process_message: unknown opcode"));
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
		if (data != 0)
		{
			thread_process_message(data, data_size);
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

	g_thread_status = 1;
	LLOGLN(10, ("thread_func: in"));
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
	LLOGLN(10, ("thread_func: out"));
	g_thread_status = -1;
	return 0;
}

static void
OpenEventProcessReceived(uint32 openHandle, void * pData, uint32 dataLength,
	uint32 totalLength, uint32 dataFlags)
{
	int index;

	index = (openHandle == g_open_handle[0]) ? 0 : 1;
	LLOGLN(10, ("OpenEventProcessReceived: receive openHandle %d dataLength %d "
		"totalLength %d dataFlags %d",
		openHandle, dataLength, totalLength, dataFlags));
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
			LLOGLN(0, ("OpenEventProcessReceived: read error"));
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
	LLOGLN(10, ("OpenEvent: event %d", event));
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
		LLOGLN(0, ("InitEventProcessConnected: error no match"));
	}
	error = g_ep.pVirtualChannelOpen(g_han, &(g_open_handle[0]),
		g_channel_def[0].name, OpenEvent);
	if (error != CHANNEL_RC_OK)
	{
		LLOGLN(0, ("InitEventProcessConnected: Open failed"));
	}
	error = g_ep.pVirtualChannelOpen(g_han, &(g_open_handle[1]),
		g_channel_def[1].name, OpenEvent);
	if (error != CHANNEL_RC_OK)
	{
		LLOGLN(0, ("InitEventProcessConnected: Open failed"));
	}
	pthread_create(&thread, 0, thread_func, 0);
	pthread_detach(thread);
}

static void
InitEventProcessTerminated(void)
{
	int index;

	set_wait_obj(&g_term_event);
	index = 0;
	while ((g_thread_status > 0) && (index < 100))
	{
		index++;
		usleep(250 * 1000);
	}
	deinit_wait_obj(&g_term_event);
	deinit_wait_obj(&g_data_in_event);
}

static void
InitEvent(void * pInitHandle, uint32 event, void * pData, uint32 dataLength)
{
	LLOGLN(10, ("InitEvent: event %d", event));
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
	LLOGLN(10, ("VirtualChannelEntry:"));
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
	strcpy(g_channel_def[1].name, "rdpdr");
	g_mutex = (pthread_mutex_t *) malloc(sizeof(pthread_mutex_t));
	pthread_mutex_init(g_mutex, 0);
	g_list_head = 0;
	g_list_tail = 0;
	init_wait_obj(&g_term_event, "freerdprdpsndterm");
	init_wait_obj(&g_data_in_event, "freerdprdpsnddatain");
	g_expectingWave = 0;
	g_current_format = -1;
	g_thread_status = 0;
	g_ep.pVirtualChannelInit(&g_han, g_channel_def, 2,
		VIRTUAL_CHANNEL_VERSION_WIN2000, InitEvent);
	return 1;
}
