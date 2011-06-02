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
#include "audin_types.h"
#include <freerdp/utils/stream.h>
#include <freerdp/utils/wait_obj.h>

struct alsa_device_data
{
	char device_name[32];
	uint32 frames_per_packet;
	uint32 target_rate;
	uint32 actual_rate;
	snd_pcm_format_t format;
	uint32 target_channels;
	uint32 actual_channels;
	int bytes_per_channel;
	int wformat;
	int block_size;
	audinDspAdpcm adpcm;

	char * buffer;
	int buffer_frames;

	audin_receive_func receive_func;
	void * user_data;

	struct wait_obj * term_event;
	int thread_status;

	PAUDINDSPRESAMPLE pResample;
	PAUDINDSPENCODEIMAADPCM pEncodeImaAdpcm;
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
		&alsa_data->actual_rate, NULL);
	snd_pcm_hw_params_set_channels_near(capture_handle, hw_params,
		&alsa_data->actual_channels);
	snd_pcm_hw_params(capture_handle, hw_params);
	snd_pcm_hw_params_free(hw_params);
	snd_pcm_prepare(capture_handle);

	if ((alsa_data->actual_rate != alsa_data->target_rate) ||
		(alsa_data->actual_channels != alsa_data->target_channels))
	{
		LLOGLN(0, ("audin_alsa_set_params: actual rate %d / channel %d is "
			"different from target rate %d / channel %d, resampling required.",
			alsa_data->actual_rate, alsa_data->actual_channels,
			alsa_data->target_rate, alsa_data->target_channels));
	}
	return 0;
}

static int
audin_alsa_thread_receive(struct alsa_device_data * alsa_data,
	char * src, int size)
{
	uint8 * resampled_data;
	int rbytes_per_frame;
	int tbytes_per_frame;
	int frames;
	int cframes;
	char * encoded_data;
	int encoded_size;
	int ret = 0;

	rbytes_per_frame = alsa_data->actual_channels * alsa_data->bytes_per_channel;
	tbytes_per_frame = alsa_data->target_channels * alsa_data->bytes_per_channel;

	if ((alsa_data->target_rate == alsa_data->actual_rate) &&
		(alsa_data->target_channels == alsa_data->actual_channels))
	{
		resampled_data = NULL;
		frames = size / rbytes_per_frame;
	}
	else
	{
		resampled_data = alsa_data->pResample((uint8 *) src, alsa_data->bytes_per_channel,
			alsa_data->actual_channels, alsa_data->actual_rate, size / rbytes_per_frame,
			alsa_data->target_channels, alsa_data->target_rate, &frames);
		LLOGLN(10, ("audin_alsa_thread_receive: resampled %d frames at %d to %d frames at %d",
			size / rbytes_per_frame, alsa_data->actual_rate, frames, alsa_data->target_rate));
		size = frames * tbytes_per_frame;
		src = (char *) resampled_data;
	}

	while (frames > 0)
	{
		cframes = alsa_data->frames_per_packet - alsa_data->buffer_frames;
		if (cframes > frames)
			cframes = frames;
		memcpy(alsa_data->buffer + alsa_data->buffer_frames * tbytes_per_frame,
			src, cframes * tbytes_per_frame);
		alsa_data->buffer_frames += cframes;
		if (alsa_data->buffer_frames >= alsa_data->frames_per_packet)
		{
			if (alsa_data->wformat == 0x11)
			{
				encoded_data = (char *) alsa_data->pEncodeImaAdpcm(&alsa_data->adpcm,
					(uint8 *) alsa_data->buffer, alsa_data->buffer_frames * tbytes_per_frame,
					alsa_data->target_channels, alsa_data->block_size, &encoded_size);
				LLOGLN(10, ("audin_alsa_thread_receive: encoded %d to %d",
					alsa_data->buffer_frames * tbytes_per_frame, encoded_size));
			}
			else
			{
				encoded_data = alsa_data->buffer;
				encoded_size = alsa_data->buffer_frames * tbytes_per_frame;
			}

			ret = alsa_data->receive_func(encoded_data, encoded_size, alsa_data->user_data);
			alsa_data->buffer_frames = 0;
			if (encoded_data != alsa_data->buffer)
				free(encoded_data);
			if (ret)
				break;
		}
		src += cframes * tbytes_per_frame;
		frames -= cframes;
	}

	if (resampled_data)
		free(resampled_data);

	return ret;
}

static void *
audin_alsa_thread_func(void * arg)
{
	struct alsa_device_data * alsa_data = (struct alsa_device_data *) arg;
	snd_pcm_t * capture_handle = NULL;
	int rbytes_per_frame;
	int tbytes_per_frame;
	char * buffer;
	int error;

	LLOGLN(10, ("audin_alsa_thread_func: in"));

	rbytes_per_frame = alsa_data->actual_channels * alsa_data->bytes_per_channel;
	tbytes_per_frame = alsa_data->target_channels * alsa_data->bytes_per_channel;
	alsa_data->buffer = (char *) malloc(tbytes_per_frame * alsa_data->frames_per_packet);
	alsa_data->buffer_frames = 0;
	buffer = (char *) malloc(rbytes_per_frame * alsa_data->frames_per_packet);
	memset(&alsa_data->adpcm, 0, sizeof(audinDspAdpcm));
	do
	{
		if ((error = snd_pcm_open(&capture_handle, alsa_data->device_name, SND_PCM_STREAM_CAPTURE, 0)) < 0)
		{
			LLOGLN(0, ("audin_alsa_thread_func: snd_pcm_open (%s)",
				snd_strerror(error)));
			break;
		}
		if (audin_alsa_set_params(alsa_data, capture_handle) != 0)
		{
			break;
		}

		while (1)
		{
			if (wait_obj_is_set(alsa_data->term_event))
			{
				break;
			}
			error = snd_pcm_readi(capture_handle, buffer, alsa_data->frames_per_packet);
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
			if (audin_alsa_thread_receive(alsa_data, buffer, error * rbytes_per_frame))
				break;
		}
	} while (0);

	free(buffer);
	free(alsa_data->buffer);
	alsa_data->buffer = NULL;
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
	LLOGLN(10, ("audin_alsa_format_supported: wFormatTag=%d "
		"nChannels=%d nSamplesPerSec=%d wBitsPerSample=%d cbSize=%d",
		wFormatTag, nChannels, nSamplesPerSec, wBitsPerSample, cbSize));
	switch (wFormatTag)
	{
		case 1: /* PCM */
			if (cbSize == 0 &&
				(nSamplesPerSec <= 48000) &&
				(wBitsPerSample == 8 || wBitsPerSample == 16) &&
				(nChannels == 1 || nChannels == 2))
			{
				LLOGLN(0, ("audin_alsa_format_supported: ok"));
				return 1;
			}
			break;

		case 0x11: /* IMA ADPCM */
			if ((nSamplesPerSec <= 48000) &&
				(wBitsPerSample == 4) &&
				(nChannels == 1 || nChannels == 2))
			{
				LLOGLN(0, ("audin_alsa_format_supported: ok"));
				return 1;
			}
			break;
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
	int wFormatTag;
	int nBlockAlign;
	int bs;

	wFormatTag = GET_UINT16(snd_format, 0);
	nChannels = GET_UINT16(snd_format, 2);
	nSamplesPerSec = GET_UINT32(snd_format, 4);
	nBlockAlign = GET_UINT16(snd_format, 12);
	wBitsPerSample = GET_UINT16(snd_format, 14);
	if (FramesPerPacket > 0)
	{
		alsa_data->frames_per_packet = FramesPerPacket;
	}
	LLOGLN(0, ("audin_alsa_set_format: wFormatTag=%d nChannels=%d "
		"nSamplesPerSec=%d wBitsPerSample=%d nBlockAlign=%d FramesPerPacket=%d",
		wFormatTag, nChannels, nSamplesPerSec, wBitsPerSample, nBlockAlign,
		alsa_data->frames_per_packet));

	alsa_data->target_rate = nSamplesPerSec;
	alsa_data->actual_rate = nSamplesPerSec;
	alsa_data->target_channels = nChannels;
	alsa_data->actual_channels = nChannels;
	switch (wFormatTag)
	{
		case 1: /* PCM */
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
			break;

		case 0x11: /* IMA ADPCM */
			alsa_data->format = SND_PCM_FORMAT_S16_LE;
			alsa_data->bytes_per_channel = 2;
			bs = (nBlockAlign - 4 * nChannels) * 4;
			alsa_data->frames_per_packet = (alsa_data->frames_per_packet * nChannels * 2 /
				bs + 1) * bs / (nChannels * 2);
			LLOGLN(0, ("audin_alsa_set_format: aligned FramesPerPacket=%d",
				alsa_data->frames_per_packet));
			break;
	}
	alsa_data->wformat = wFormatTag;
	alsa_data->block_size = nBlockAlign;

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
	alsa_data->target_rate = 22050;
	alsa_data->actual_rate = 22050;
	alsa_data->format = SND_PCM_FORMAT_S16_LE;
	alsa_data->target_channels = 2;
	alsa_data->actual_channels = 2;
	alsa_data->bytes_per_channel = 2;
	alsa_data->term_event = wait_obj_new("freerdpaudinterm");
	alsa_data->pResample = pEntryPoints->pResample;
	alsa_data->pEncodeImaAdpcm = pEntryPoints->pEncodeImaAdpcm;
	devplugin->device_data = alsa_data;

	LLOGLN(0, ("audin_alsa: alsa device '%s' registered.", alsa_data->device_name));

	return 0;
}

