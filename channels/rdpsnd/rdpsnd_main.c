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
#include <unistd.h>
#include <sys/time.h>
#include <freerdp/types_ui.h>
#include <freerdp/vchan.h>
#include "chan_stream.h"
#include "chan_plugin.h"
#include "wait_obj.h"
#include "rdpsnd_types.h"
#include "rdpsnd_dsp.h"

#ifdef _WIN32
#define DLOPEN(f) LoadLibraryA(f)
#define DLSYM(f, n) GetProcAddress(f, n)
#define DLCLOSE(f) FreeLibrary(f)
#define PATH_SEPARATOR '\\'
#define PLUGIN_EXT "dll"
#else
#include <dlfcn.h>
#define DLOPEN(f) dlopen(f, RTLD_LOCAL | RTLD_LAZY)
#define DLSYM(f, n) dlsym(f, n)
#define DLCLOSE(f) dlclose(f)
#define PATH_SEPARATOR '/'
#define PLUGIN_EXT "so"
#endif

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

struct data_in_item
{
	struct data_in_item * next;
	char * data;
	int data_size;
};

struct data_out_item
{
	struct data_out_item * next;
	char * data;
	int data_size;
	uint32 out_time_stamp;
};

struct rdpsnd_plugin
{
	rdpChanPlugin chan_plugin;

	CHANNEL_ENTRY_POINTS ep;
	CHANNEL_DEF channel_def;
	uint32 open_handle;
	char * data_in;
	int data_in_size;
	int data_in_read;
	struct wait_obj * term_event;
	struct wait_obj * data_in_event;
	struct data_in_item * in_list_head;
	struct data_in_item * in_list_tail;
	/* for locking the linked list */
	pthread_mutex_t * in_mutex;

	char * data_out;
	int data_out_size;
	int delay_ms;
	struct data_out_item * out_list_head;
	struct data_out_item * out_list_tail;

	int cBlockNo;
	char * supported_formats;
	int supported_formats_size;
	int current_format;
	int expectingWave; /* boolean */
	char waveData[4];
	int waveDataSize;
	uint32 wTimeStamp; /* server timestamp */
	uint32 local_time_stamp; /* client timestamp */
	int thread_status;

	/* Device plugin */
	rdpsndDevicePlugin * device_plugin;
};

/* get time in milliseconds */
static uint32
get_mstime(void)
{
	struct timeval tp;

	gettimeofday(&tp, 0);
	return (tp.tv_sec * 1000) + (tp.tv_usec / 1000);
}

/* called by main thread
   add item to linked list and inform worker thread that there is data */
static void
signal_data_in(rdpsndPlugin * plugin)
{
	struct data_in_item * item;

	item = (struct data_in_item *) malloc(sizeof(struct data_in_item));
	item->next = 0;
	item->data = plugin->data_in;
	plugin->data_in = 0;
	item->data_size = plugin->data_in_size;
	plugin->data_in_size = 0;
	pthread_mutex_lock(plugin->in_mutex);
	if (plugin->in_list_tail == 0)
	{
		plugin->in_list_head = item;
		plugin->in_list_tail = item;
	}
	else
	{
		plugin->in_list_tail->next = item;
		plugin->in_list_tail = item;
	}
	pthread_mutex_unlock(plugin->in_mutex);
	wait_obj_set(plugin->data_in_event);
}

static void
queue_data_out(rdpsndPlugin * plugin)
{
	struct data_out_item * item;

	LLOGLN(10, ("queue_data_out: "));
	item = (struct data_out_item *) malloc(sizeof(struct data_out_item));
	item->next = 0;
	item->data = plugin->data_out;
	plugin->data_out = 0;
	item->data_size = plugin->data_out_size;
	plugin->data_out_size = 0;
	item->out_time_stamp = plugin->local_time_stamp + plugin->delay_ms;
	if (plugin->out_list_tail == 0)
	{
		plugin->out_list_head = item;
		plugin->out_list_tail = item;
	}
	else
	{
		plugin->out_list_tail->next = item;
		plugin->out_list_tail = item;
	}
}

/* process the linked list of data that has queued to be sent */
static int
thread_process_data_out(rdpsndPlugin * plugin)
{
	char * data;
	int data_size;
	struct data_out_item * item;
	uint32 cur_time;
	uint32 error;

	LLOGLN(10, ("thread_process_data_out: "));
	while (1)
	{
		if (wait_obj_is_set(plugin->term_event))
		{
			break;
		}
		if (plugin->out_list_head == 0)
		{
			break;
		}
		cur_time = get_mstime();
		if (cur_time <= plugin->out_list_head->out_time_stamp)
		{
			break;
		}
		data = plugin->out_list_head->data;
		data_size = plugin->out_list_head->data_size;
		item = plugin->out_list_head;
		plugin->out_list_head = plugin->out_list_head->next;
		if (plugin->out_list_head == 0)
		{
			plugin->out_list_tail = 0;
		}

		error = plugin->ep.pVirtualChannelWrite(plugin->open_handle,
			data, data_size, data);
		if (error != CHANNEL_RC_OK)
		{
			LLOGLN(0, ("thread_process_data_out: "
				"VirtualChannelWrite "
				"failed %d", error));
		}
		LLOGLN(10, ("thread_process_data_out: confirm sent"));

		if (item != 0)
		{
			free(item);
		}
	}
	return 0;
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
thread_process_message_formats(rdpsndPlugin * plugin, char * data, int data_size)
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
	plugin->cBlockNo = GET_UINT8(data, 16); /* cLastBlockConfirmed */
	version = GET_UINT16(data, 17); /* wVersion */
	if (version < 2)
	{
		LLOGLN(0, ("thread_process_message_formats: warning, old server"));
	}
	LLOGLN(0, ("thread_process_message_formats: version %d", version));
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
		if (plugin->device_plugin && plugin->device_plugin->format_supported(plugin->device_plugin, ldata, size))
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
		plugin->supported_formats = (char *) malloc(out_format_size);
		memcpy(plugin->supported_formats, out_formats, out_format_size);
		plugin->supported_formats_size = out_format_size;
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
	SET_UINT16(out_data, 21, 6); /* wVersion */
	SET_UINT8(out_data, 23, 0); /* bPad */
	error = plugin->ep.pVirtualChannelWrite(plugin->open_handle,
		out_data, size, out_data);
	if (error != CHANNEL_RC_OK)
	{
		LLOGLN(0, ("thread_process_message_formats: "
			"VirtualChannelWrite "
			"failed %d", error));
		return 1;
	}
	if (version >= 6)
	{
		size = 8;
		out_data = (char *) malloc(size);
		SET_UINT8(out_data, 0, SNDC_QUALITYMODE); /* Header (4 bytes) */
		SET_UINT8(out_data, 1, 0);
		SET_UINT16(out_data, 2, size - 4);
		SET_UINT16(out_data, 4, 2); /* HIGH_QUALITY */
		SET_UINT16(out_data, 6, 0); /* Reserved (2 bytes) */
		error = plugin->ep.pVirtualChannelWrite(plugin->open_handle,
			out_data, size, out_data);
		if (error != CHANNEL_RC_OK)
		{
			LLOGLN(0, ("thread_process_message_formats: "
				"VirtualChannelWrite "
				"failed %d", error));
			return 1;
		}
	}
	return 0;
}

/* called by worker thread
   server is getting a feel of the round trip time */
static int
thread_process_message_training(rdpsndPlugin * plugin, char * data, int data_size)
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
	error = plugin->ep.pVirtualChannelWrite(plugin->open_handle,
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
set_format(rdpsndPlugin * plugin)
{
	char * snd_format;
	int size;
	int index;

	LLOGLN(10, ("set_format:"));
	snd_format = plugin->supported_formats;
	size = 18 + GET_UINT16(snd_format, 16);
	index = 0;
	while (index < plugin->current_format)
	{
		snd_format += size;
		size = 18 + GET_UINT16(snd_format, 16);
		index++;
	}
	if (plugin->device_plugin)
		plugin->device_plugin->set_format(plugin->device_plugin, snd_format, size);
	return 0;
}

static int
thread_process_message_wave_info(rdpsndPlugin * plugin, char * data, int data_size)
{
	int wFormatNo;
	int error;

	if (plugin->device_plugin)
		error = plugin->device_plugin->open(plugin->device_plugin);
	else
		error = 1;

	plugin->wTimeStamp = GET_UINT16(data, 0); /* time in ms */
	plugin->local_time_stamp = get_mstime(); /* time in ms */
	wFormatNo = GET_UINT16(data, 2);
	LLOGLN(10, ("thread_process_message_wave_info: data_size %d "
		"wFormatNo %d", data_size, wFormatNo));
	plugin->cBlockNo = GET_UINT8(data, 4);
	plugin->waveDataSize = data_size - 8;
	memcpy(plugin->waveData, data + 8, 4);
	if (wFormatNo != plugin->current_format && !error)
	{
		plugin->current_format = wFormatNo;
		set_format(plugin);
	}
	plugin->expectingWave = 1;
	return error;
}

/* header is not removed from data in this function */
static int
thread_process_message_wave(rdpsndPlugin * plugin, char * data, int data_size)
{
	int size;
	int wTimeStamp;
	char * out_data;
	uint32 process_ms;

	plugin->expectingWave = 0;
	memcpy(data, plugin->waveData, 4);
	if (data_size != plugin->waveDataSize)
	{
		LLOGLN(0, ("thread_process_message_wave: "
			"size error"));
	}
	if (plugin->device_plugin)
		plugin->device_plugin->play(plugin->device_plugin, data, data_size);
	size = 8;
	out_data = (char *) malloc(size);
	SET_UINT8(out_data, 0, SNDC_WAVECONFIRM);
	SET_UINT8(out_data, 1, 0);
	SET_UINT16(out_data, 2, size - 4);
	process_ms = get_mstime() - plugin->local_time_stamp;
	plugin->delay_ms = 300;
	LLOGLN(10, ("thread_process_message_wave: "
		"data_size %d delay_ms %d process_ms %u",
		data_size, plugin->delay_ms, process_ms));
	wTimeStamp = plugin->wTimeStamp + plugin->delay_ms;
	SET_UINT16(out_data, 4, wTimeStamp);
	SET_UINT8(out_data, 6, plugin->cBlockNo);
	SET_UINT8(out_data, 7, 0);
	plugin->data_out = out_data;
	plugin->data_out_size = size;
	queue_data_out(plugin);
	return 0;
}

static int
thread_process_message_close(rdpsndPlugin * plugin, char * data, int data_size)
{
	LLOGLN(10, ("thread_process_message_close: "
		"data_size %d", data_size));
	if (plugin->device_plugin)
		plugin->device_plugin->close(plugin->device_plugin);
	return 0;
}

static int
thread_process_message_setvolume(rdpsndPlugin * plugin, char * data, int data_size)
{
	uint32 dwVolume;

	LLOGLN(10, ("thread_process_message_setvolume:"));
	dwVolume = GET_UINT32(data, 0);
	if (plugin->device_plugin)
		plugin->device_plugin->set_volume(plugin->device_plugin, dwVolume);
	return 0;
}

static int
thread_process_message(rdpsndPlugin * plugin, char * data, int data_size)
{
	int opcode;
	int size;
	static int wave_error = 0;

	if (plugin->expectingWave)
	{
		if (!wave_error)
			thread_process_message_wave(plugin, data, data_size);
		plugin->expectingWave = 0;
		return 0;
	}
	opcode = GET_UINT8(data, 0);
	size = GET_UINT16(data, 2);
	LLOGLN(10, ("thread_process_message: data_size %d opcode %d size %d",
		data_size, opcode, size));
	switch (opcode)
	{
		case SNDC_FORMATS:
			thread_process_message_formats(plugin, data + 4, size);
			break;
		case SNDC_TRAINING:
			thread_process_message_training(plugin, data + 4, size);
			break;
		case SNDC_WAVE:
			if (!wave_error)
				wave_error = thread_process_message_wave_info(plugin, data + 4, size);
			break;
		case SNDC_CLOSE:
			thread_process_message_close(plugin, data + 4, size);
			wave_error = 0;
			break;
		case SNDC_SETVOLUME:
			thread_process_message_setvolume(plugin, data + 4, size);
			break;
		case 0: /* wave PDU: ignoring it since it is received only if wave_error == 1*/
			break;
		default:
			LLOGLN(0, ("thread_process_message: unknown opcode"));
			break;
	}
	return 0;
}

/* process the linked list of data that has come in */
static int
thread_process_data_in(rdpsndPlugin * plugin)
{
	char * data;
	int data_size;
	struct data_in_item * item;

	while (1)
	{
		if (wait_obj_is_set(plugin->term_event))
		{
			break;
		}
		pthread_mutex_lock(plugin->in_mutex);
		if (plugin->in_list_head == 0)
		{
			pthread_mutex_unlock(plugin->in_mutex);
			break;
		}
		data = plugin->in_list_head->data;
		data_size = plugin->in_list_head->data_size;
		item = plugin->in_list_head;
		plugin->in_list_head = plugin->in_list_head->next;
		if (plugin->in_list_head == 0)
		{
			plugin->in_list_tail = 0;
		}
		pthread_mutex_unlock(plugin->in_mutex);
		if (data != 0)
		{
			thread_process_message(plugin, data, data_size);
			free(data);
		}
		if (item != 0)
		{
			free(item);
		}

		if (plugin->out_list_head != 0)
		{
			thread_process_data_out(plugin);
		}
	}
	return 0;
}

static void *
thread_func(void * arg)
{
	rdpsndPlugin * plugin;
	struct wait_obj * listobj[2];
	int numobj;
	int timeout;

	plugin = (rdpsndPlugin *) arg;

	plugin->thread_status = 1;
	LLOGLN(10, ("thread_func: in"));
	while (1)
	{
		listobj[0] = plugin->term_event;
		listobj[1] = plugin->data_in_event;
		numobj = 2;
		timeout = plugin->out_list_head == 0 ? -1 : 10;
		wait_obj_select(listobj, numobj, NULL, 0, timeout);
		if (wait_obj_is_set(plugin->term_event))
		{
			break;
		}
		if (wait_obj_is_set(plugin->data_in_event))
		{
			wait_obj_clear(plugin->data_in_event);
			/* process data in */
			thread_process_data_in(plugin);
		}
		if (plugin->out_list_head != 0)
		{
			thread_process_data_out(plugin);
		}
	}
	LLOGLN(10, ("thread_func: out"));
	plugin->thread_status = -1;
	return 0;
}

static void
OpenEventProcessReceived(uint32 openHandle, void * pData, uint32 dataLength,
	uint32 totalLength, uint32 dataFlags)
{
	rdpsndPlugin * plugin;

	plugin = (rdpsndPlugin *) chan_plugin_find_by_open_handle(openHandle);

	LLOGLN(10, ("OpenEventProcessReceived: receive openHandle %d dataLength %d "
		"totalLength %d dataFlags %d",
		openHandle, dataLength, totalLength, dataFlags));
	if (dataFlags & CHANNEL_FLAG_FIRST)
	{
		plugin->data_in_read = 0;
		if (plugin->data_in != 0)
		{
			free(plugin->data_in);
		}
		plugin->data_in = (char *) malloc(totalLength);
		plugin->data_in_size = totalLength;
	}
	memcpy(plugin->data_in + plugin->data_in_read, pData, dataLength);
	plugin->data_in_read += dataLength;
	if (dataFlags & CHANNEL_FLAG_LAST)
	{
		if (plugin->data_in_read != plugin->data_in_size)
		{
			LLOGLN(0, ("OpenEventProcessReceived: read error"));
		}
		signal_data_in(plugin);
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
	rdpsndPlugin * plugin;
	uint32 error;
	pthread_t thread;

	plugin = (rdpsndPlugin *) chan_plugin_find_by_init_handle(pInitHandle);
	if (plugin == NULL)
	{
		LLOGLN(0, ("InitEventProcessConnected: error no match"));
		return;
	}

	error = plugin->ep.pVirtualChannelOpen(pInitHandle, &(plugin->open_handle),
		plugin->channel_def.name, OpenEvent);
	if (error != CHANNEL_RC_OK)
	{
		LLOGLN(0, ("InitEventProcessConnected: Open failed"));
		return;
	}
	chan_plugin_register_open_handle((rdpChanPlugin *) plugin, plugin->open_handle);

	pthread_create(&thread, 0, thread_func, plugin);
	pthread_detach(thread);
}

static void
InitEventProcessTerminated(void * pInitHandle)
{
	rdpsndPlugin * plugin;
	int index;
	struct data_in_item * in_item;
	struct data_out_item * out_item;

	plugin = (rdpsndPlugin *) chan_plugin_find_by_init_handle(pInitHandle);
	if (plugin == NULL)
	{
		LLOGLN(0, ("InitEventProcessConnected: error no match"));
		return;
	}

	wait_obj_set(plugin->term_event);
	index = 0;
	while ((plugin->thread_status > 0) && (index < 100))
	{
		index++;
		usleep(250 * 1000);
	}
	wait_obj_free(plugin->term_event);
	wait_obj_free(plugin->data_in_event);

	pthread_mutex_destroy(plugin->in_mutex);
	free(plugin->in_mutex);

	/* free the un-processed in/out queue */
	while (plugin->in_list_head != 0)
	{
		in_item = plugin->in_list_head;
		plugin->in_list_head = in_item->next;
		free(in_item->data);
		free(in_item);
	}
	while (plugin->out_list_head != 0)
	{
		out_item = plugin->out_list_head;
		plugin->out_list_head = out_item->next;
		free(out_item->data);
		free(out_item);
	}

	if (plugin->device_plugin)
	{
		plugin->device_plugin->free(plugin->device_plugin);
		free(plugin->device_plugin);
		plugin->device_plugin = NULL;
	}
	chan_plugin_uninit((rdpChanPlugin *) plugin);
	free(plugin);
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
			InitEventProcessTerminated(pInitHandle);
			break;
	}
}

static rdpsndDevicePlugin *
rdpsnd_register_device_plugin(rdpsndPlugin * plugin)
{
	rdpsndDevicePlugin * devplugin;

	if (plugin->device_plugin)
	{
		LLOGLN(0, ("rdpsnd_process_plugin_data: existing device, abort."));
		return NULL;
	}

	devplugin = (rdpsndDevicePlugin *) malloc(sizeof(rdpsndDevicePlugin));
	memset(devplugin, 0, sizeof(rdpsndDevicePlugin));
	plugin->device_plugin = devplugin;
	return devplugin;
}

static int
rdpsnd_load_device_plugin(rdpsndPlugin * plugin, const char * name, RD_PLUGIN_DATA * data)
{
	FREERDP_RDPSND_DEVICE_ENTRY_POINTS entryPoints;
	char path[256];
	void * han;
	PFREERDP_RDPSND_DEVICE_ENTRY entry;

	if (strchr(name, PATH_SEPARATOR) == NULL)
	{
		snprintf(path, sizeof(path), PLUGIN_PATH "/rdpsnd_%s." PLUGIN_EXT, name);
	}
	else
	{
		snprintf(path, sizeof(path), "%s", name);
	}
	han = DLOPEN(path);
	LLOGLN(0, ("rdpsnd_load_device_plugin: %s", path));
	if (han == NULL)
	{
		LLOGLN(0, ("rdpsnd_load_device_plugin: failed to load %s", path));
		return 1;
	}
	entry = (PFREERDP_RDPSND_DEVICE_ENTRY) DLSYM(han, RDPSND_DEVICE_EXPORT_FUNC_NAME);
	if (entry == NULL)
	{
		DLCLOSE(han);
		LLOGLN(0, ("rdpsnd_load_device_plugin: failed to find export function in %s", path));
		return 1;
	}

	entryPoints.plugin = plugin;
	entryPoints.pRegisterRdpsndDevice = rdpsnd_register_device_plugin;
	entryPoints.pResample = rdpsnd_dsp_resample;
	entryPoints.pDecodeImaAdpcm = rdpsnd_dsp_decode_ima_adpcm;
	entryPoints.data = data;
	if (entry(&entryPoints) != 0)
	{
		DLCLOSE(han);
		LLOGLN(0, ("rdpsnd_load_device_plugin: %s entry returns error.", path));
		return 1;
	}
	return 0;
}

static int
rdpsnd_process_plugin_data(rdpsndPlugin * plugin, RD_PLUGIN_DATA * data)
{
	if (strcmp((char*)data->data[0], "buffer") == 0)
	{
		/* TODO: Set the sound buffer size */
		return 0;
	}
	else
	{
		return rdpsnd_load_device_plugin(plugin, (char*)data->data[0], data);
	}
}

/* this registers two channels but only rdpsnd is used */
int
VirtualChannelEntry(PCHANNEL_ENTRY_POINTS pEntryPoints)
{
	rdpsndPlugin * plugin;
	RD_PLUGIN_DATA * data;
	RD_PLUGIN_DATA default_data[2] = { { 0 }, { 0 } };
	int ret;

	LLOGLN(10, ("VirtualChannelEntry:"));

	plugin = (rdpsndPlugin *) malloc(sizeof(rdpsndPlugin));
	memset(plugin, 0, sizeof(rdpsndPlugin));

	chan_plugin_init((rdpChanPlugin *) plugin);

	plugin->data_in_size = 0;
	plugin->data_in = 0;
	plugin->ep = *pEntryPoints;
	memset(&(plugin->channel_def), 0, sizeof(plugin->channel_def));
	plugin->channel_def.options = CHANNEL_OPTION_INITIALIZED |
		CHANNEL_OPTION_ENCRYPT_RDP;
	strcpy(plugin->channel_def.name, "rdpsnd");
	plugin->in_mutex = (pthread_mutex_t *) malloc(sizeof(pthread_mutex_t));
	pthread_mutex_init(plugin->in_mutex, 0);
	plugin->in_list_head = 0;
	plugin->in_list_tail = 0;
	plugin->out_list_head = 0;
	plugin->out_list_tail = 0;
	plugin->term_event = wait_obj_new("freerdprdpsndterm");
	plugin->data_in_event = wait_obj_new("freerdprdpsnddatain");
	plugin->expectingWave = 0;
	plugin->current_format = -1;
	plugin->thread_status = 0;
	plugin->ep.pVirtualChannelInit(&plugin->chan_plugin.init_handle, &plugin->channel_def, 1,
		VIRTUAL_CHANNEL_VERSION_WIN2000, InitEvent);

	if (pEntryPoints->cbSize >= sizeof(CHANNEL_ENTRY_POINTS_EX))
	{
		data = (RD_PLUGIN_DATA *) (((PCHANNEL_ENTRY_POINTS_EX)pEntryPoints)->pExtendedData);
	}
	else
	{
		data = NULL;
	}
	while (data && data->size > 0)
	{
		rdpsnd_process_plugin_data(plugin, data);
		data = (RD_PLUGIN_DATA *) (((void *) data) + data->size);
	}
	if (plugin->device_plugin == NULL)
	{
		default_data[0].size = sizeof(RD_PLUGIN_DATA);
		default_data[0].data[0] = "pulse";
		default_data[0].data[1] = "";
		ret = rdpsnd_load_device_plugin(plugin, "pulse", default_data);
		if (ret)
		{
			if (plugin->device_plugin)
			{
				plugin->device_plugin->free(plugin->device_plugin);
				free(plugin->device_plugin);
				plugin->device_plugin = NULL;
			}
			default_data[0].size = sizeof(RD_PLUGIN_DATA);
			default_data[0].data[0] = "alsa";
			default_data[0].data[1] = "default";
			ret = rdpsnd_load_device_plugin(plugin, "alsa", default_data);
		}
	}
	if (plugin->device_plugin == NULL)
	{
		LLOGLN(0, ("rdpsnd: no sound device."));
	}

	return 1;
}
