/*
   FreeRDP: A Remote Desktop Protocol client.
   Audio Input Redirection Virtual Channel - ALSA implementation

   Copyright 2010-2011 Vic Lee

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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <alsa/asoundlib.h>
#include "drdynvc_types.h"
#include "wait_obj.h"
#include "audin_types.h"

struct alsa_device_data
{
	char device_name[32];
	uint32 frames_per_packet;
	uint32 rrate;
	snd_pcm_format_t format;
	int num_channels;
	int bytes_per_channel;

	audin_receive_func receive_func;
	void * user_data;

	struct wait_obj * term_event;
	int thread_status;
};

static int
audin_alsa_set_params(struct alsa_device_data * alsa_data, snd_pcm_t * capture_handle)
{
	snd_pcm_hw_params_t * hw_params;
	int error;

	if ((error = snd_pcm_hw_params_malloc(&hw_params)) < 0)
	{
		LLOGLN(0, ("audin_alsa_set_params: snd_pcm_hw_params_malloc (%s)",
			 snd_strerror(error)));
		return 1;
	}
	snd_pcm_hw_params_any(capture_handle, hw_params);
	snd_pcm_hw_params_set_access(capture_handle, hw_params,
		SND_PCM_ACCESS_RW_INTERLEAVED);
	snd_pcm_hw_params_set_format(capture_handle, hw_params,
		alsa_data->format);
	snd_pcm_hw_params_set_rate_near(capture_handle, hw_params,
		&alsa_data->rrate, NULL);
	snd_pcm_hw_params_set_channels(capture_handle, hw_params, alsa_data->num_channels);
	snd_pcm_hw_params(capture_handle, hw_params);
	snd_pcm_hw_params_free(hw_params);
	snd_pcm_prepare(capture_handle);

	return 0;
}

static void *
audin_alsa_thread_func(void * arg)
{
	struct alsa_device_data * alsa_data = (struct alsa_device_data *) arg;
	snd_pcm_t * capture_handle = NULL;
	char * buffer;
	int buffer_size;
	int bytes_per_frame;
	char * pindex;
	int frames;
	int error;

	LLOGLN(10, ("audin_alsa_thread_func: in"));

	bytes_per_frame = alsa_data->num_channels * alsa_data->bytes_per_channel;
	buffer_size = bytes_per_frame * alsa_data->frames_per_packet;
	buffer = (char *) malloc(buffer_size);
	do
	{
		if ((error = snd_pcm_open(&capture_handle, "default", SND_PCM_STREAM_CAPTURE, 0)) < 0)
		{
			LLOGLN(0, ("audin_alsa_thread_func: snd_pcm_open (%s)",
				snd_strerror(error)));
			break;
		}
		if (audin_alsa_set_params(alsa_data, capture_handle) != 0)
		{
			break;
		}

		frames = alsa_data->frames_per_packet;
		pindex = buffer;
		while (1)
		{
			if (wait_obj_is_set(alsa_data->term_event))
			{
				break;
			}
			error = snd_pcm_readi(capture_handle, pindex, frames);
			if (error == -EPIPE)
			{
				LLOGLN(0, ("audin_alsa_thread_func: overrun occurred"));
				snd_pcm_recover(capture_handle, error, 0);
				continue;
			}
			else if (error < 0)
			{
				LLOGLN(0, ("audin_alsa_thread_func: snd_pcm_readi (%s)",
					 snd_strerror(error)));
				break;
			}
			frames -= error;
			pindex += error * bytes_per_frame;
			if (frames <= 0)
			{
				if (alsa_data->receive_func(buffer, buffer_size, alsa_data->user_data) != 0)
					break;
				frames = alsa_data->frames_per_packet;
				pindex = buffer;
			}
		}
	} while (0);

	free(buffer);
	if (capture_handle)
		snd_pcm_close(capture_handle);

	alsa_data->thread_status = -1;
	LLOGLN(10, ("audin_alsa_thread_func: out"));

	return NULL;
}

void
audin_alsa_free(audinDevicePlugin * devplugin)
{
	struct alsa_device_data * alsa_data = (struct alsa_device_data *) devplugin->device_data;

	wait_obj_free(alsa_data->term_event);
	free(alsa_data);
}

int
audin_alsa_format_supported(audinDevicePlugin * devplugin, char * snd_format, int size)
{
	int nChannels;
	int wBitsPerSample;
	int nSamplesPerSec;
	int cbSize;
	int wFormatTag;

	wFormatTag = GET_UINT16(snd_format, 0);
	nChannels = GET_UINT16(snd_format, 2);
	nSamplesPerSec = GET_UINT32(snd_format, 4);
	wBitsPerSample = GET_UINT16(snd_format, 14);
	cbSize = GET_UINT16(snd_format, 16);
	LLOGLN(10, ("audin_alsa_format_supported: size=%d wFormatTag=%d nChannels=%d nSamplesPerSec=%d wBitsPerSample=%d cbSize=%d",
		size, wFormatTag, nChannels, nSamplesPerSec, wBitsPerSample, cbSize));
	if (cbSize == 0 &&
		(nSamplesPerSec == 22050 || nSamplesPerSec == 44100) &&
		(wBitsPerSample == 8 || wBitsPerSample == 16) &&
		(nChannels == 1 || nChannels == 2) &&
		wFormatTag == 1) /* WAVE_FORMAT_PCM */
	{
		LLOGLN(0, ("audin_alsa_format_supported: ok."));
		return 1;
	}
	return 0;
}

int
audin_alsa_set_format(audinDevicePlugin * devplugin, uint32 FramesPerPacket, char * snd_format, int size)
{
	struct alsa_device_data * alsa_data = (struct alsa_device_data *) devplugin->device_data;
	int nChannels;
	int wBitsPerSample;
	int nSamplesPerSec;

	nChannels = GET_UINT16(snd_format, 2);
	nSamplesPerSec = GET_UINT32(snd_format, 4);
	wBitsPerSample = GET_UINT16(snd_format, 14);
	LLOGLN(0, ("audin_alsa_set_format: nChannels %d "
		"nSamplesPerSec %d wBitsPerSample %d",
		nChannels, nSamplesPerSec, wBitsPerSample));

	if (FramesPerPacket > 0)
	{
		alsa_data->frames_per_packet = FramesPerPacket;
	}
	alsa_data->rrate = nSamplesPerSec;
	alsa_data->num_channels = nChannels;
	switch (wBitsPerSample)
	{
		case 8:
			alsa_data->format = SND_PCM_FORMAT_S8;
			alsa_data->bytes_per_channel = 1;
			break;
		case 16:
			alsa_data->format = SND_PCM_FORMAT_S16_LE;
			alsa_data->bytes_per_channel = 2;
			break;
	}
	return 0;
}

int
audin_alsa_open(audinDevicePlugin * devplugin, audin_receive_func receive_func, void * user_data)
{
	struct alsa_device_data * alsa_data = (struct alsa_device_data *) devplugin->device_data;
	pthread_t thread;

	LLOGLN(10, ("audin_alsa_open:"));
	alsa_data->receive_func = receive_func;
	alsa_data->user_data = user_data;

	alsa_data->thread_status = 1;
	pthread_create(&thread, 0, audin_alsa_thread_func, alsa_data);
	pthread_detach(thread);

	return 0;
}

int
audin_alsa_close(audinDevicePlugin * devplugin)
{
	struct alsa_device_data * alsa_data = (struct alsa_device_data *) devplugin->device_data;
	int index;

	LLOGLN(10, ("audin_alsa_close:"));
	wait_obj_set(alsa_data->term_event);
	index = 0;
	while ((alsa_data->thread_status > 0) && (index < 100))
	{
		index++;
		usleep(250 * 1000);
	}
	wait_obj_clear(alsa_data->term_event);
	alsa_data->receive_func = NULL;
	alsa_data->user_data = NULL;
	return 0;
}

int
FreeRDPAudinDeviceEntry(PFREERDP_AUDIN_DEVICE_ENTRY_POINTS pEntryPoints)
{
	audinDevicePlugin * devplugin;
	struct alsa_device_data * alsa_data;
	RD_PLUGIN_DATA * data;
	int i;

	devplugin = pEntryPoints->pRegisterAudinDevice(pEntryPoints->plugin);
	if (devplugin == NULL)
	{
		LLOGLN(0, ("audin_alsa: unable to register device."));
		return 1;
	}

	devplugin->open = audin_alsa_open;
	devplugin->format_supported = audin_alsa_format_supported;
	devplugin->set_format = audin_alsa_set_format;
	devplugin->close = audin_alsa_close;
	devplugin->free = audin_alsa_free;

	alsa_data = (struct alsa_device_data *) malloc(sizeof(struct alsa_device_data));
	memset(alsa_data, 0, sizeof(struct alsa_device_data));

	data = (RD_PLUGIN_DATA *) pEntryPoints->data;
	if (data && strcmp(data->data[0], "audin") == 0 && strcmp(data->data[1], "alsa") == 0)
	{
		for (i = 2; i < 4 && data->data[i]; i++)
		{
			if (i > 2)
			{
				strncat(alsa_data->device_name, ":",
					sizeof(alsa_data->device_name) - strlen(alsa_data->device_name));
			}
			strncat(alsa_data->device_name, (char*)data->data[i],
				sizeof(alsa_data->device_name) - strlen(alsa_data->device_name));
		}
	}
	if (alsa_data->device_name[0] == '\0')
	{
		strcpy(alsa_data->device_name, "default");
	}
	alsa_data->frames_per_packet = 128;
	alsa_data->rrate = 22050;
	alsa_data->format = SND_PCM_FORMAT_S16_LE;
	alsa_data->num_channels = 2;
	alsa_data->bytes_per_channel = 2;
	alsa_data->term_event = wait_obj_new("freerdpaudinterm");
	devplugin->device_data = alsa_data;

	LLOGLN(0, ("audin_alsa: alsa device '%s' registered.", alsa_data->device_name));

	return 0;
}

