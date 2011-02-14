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

// libasound2-dev

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <alsa/asoundlib.h>
#include <freerdp/types_ui.h>
#include "chan_stream.h"
#include "rdpsnd_types.h"

#define LOG_LEVEL 1
#define LLOG(_level, _args) \
  do { if (_level < LOG_LEVEL) { printf _args ; } } while (0)
#define LLOGLN(_level, _args) \
  do { if (_level < LOG_LEVEL) { printf _args ; printf("\n"); } } while (0)

struct alsa_device_data
{
	char device_name[32];
	snd_pcm_t * out_handle;
	uint32 source_rate;
	uint32 actual_rate;
	snd_pcm_format_t format;
	int num_channels;
	int bytes_per_channel;

	PRDPSNDDSPRESAMPLE pResample;
};

static int
set_params(struct alsa_device_data * alsa_data)
{
	snd_pcm_hw_params_t * hw_params;
	snd_pcm_sw_params_t * sw_params;
	int error;
	snd_pcm_uframes_t frames;

	error = snd_pcm_hw_params_malloc(&hw_params);
	if (error < 0)
	{
		LLOGLN(0, ("set_params: snd_pcm_hw_params_malloc failed"));
		return 1;
	}
	snd_pcm_hw_params_any(alsa_data->out_handle, hw_params);
	snd_pcm_hw_params_set_access(alsa_data->out_handle, hw_params,
		SND_PCM_ACCESS_RW_INTERLEAVED);
	snd_pcm_hw_params_set_format(alsa_data->out_handle, hw_params,
		alsa_data->format);
	snd_pcm_hw_params_set_rate_near(alsa_data->out_handle, hw_params,
		&alsa_data->actual_rate, NULL);
	snd_pcm_hw_params_set_channels(alsa_data->out_handle, hw_params,
		alsa_data->num_channels);
	frames = alsa_data->actual_rate * 4;
	snd_pcm_hw_params_set_buffer_size_near(alsa_data->out_handle, hw_params,
		&frames);
	snd_pcm_hw_params(alsa_data->out_handle, hw_params);
	snd_pcm_hw_params_free(hw_params);

	error = snd_pcm_sw_params_malloc(&sw_params);
	if (error < 0)
	{
		LLOGLN(0, ("set_params: snd_pcm_sw_params_malloc"));
		return 1;
	}
	snd_pcm_sw_params_current(alsa_data->out_handle, sw_params);
	snd_pcm_sw_params_set_avail_min(alsa_data->out_handle, sw_params,
		4096);
	snd_pcm_sw_params_set_start_threshold(alsa_data->out_handle, sw_params,
		frames / 2);
	snd_pcm_sw_params(alsa_data->out_handle, sw_params);
	snd_pcm_sw_params_free(sw_params);

	snd_pcm_prepare(alsa_data->out_handle);

	LLOGLN(10, ("set_params: hardware buffer %d frames, playback buffer %.2g seconds",
		(int)frames, (double)frames / 2.0 / (double)alsa_data->actual_rate));
	if (alsa_data->actual_rate != alsa_data->source_rate)
	{
		LLOGLN(0, ("set_params: actual rate %d is different from source rate %d, resampling required.",
			alsa_data->actual_rate, alsa_data->source_rate));
	}
	return 0;
}

static int
rdpsnd_alsa_open(rdpsndDevicePlugin * devplugin)
{
	struct alsa_device_data * alsa_data;
	int error;

	alsa_data = (struct alsa_device_data *) devplugin->device_data;

	if (alsa_data->out_handle != 0)
	{
		return 0;
	}
	LLOGLN(10, ("rdpsnd_alsa_open:"));
	error = snd_pcm_open(&alsa_data->out_handle, alsa_data->device_name,
		SND_PCM_STREAM_PLAYBACK, 0);
	if (error < 0)
	{
		LLOGLN(0, ("rdpsnd_alsa_open: snd_pcm_open failed"));
		return 1;
	}
	set_params(alsa_data);
	return 0;
}

static int
rdpsnd_alsa_close(rdpsndDevicePlugin * devplugin)
{
	struct alsa_device_data * alsa_data;

	alsa_data = (struct alsa_device_data *) devplugin->device_data;
	if (alsa_data->out_handle != 0)
	{
		LLOGLN(10, ("rdpsnd_alsa_close:"));
		snd_pcm_drain(alsa_data->out_handle);
		snd_pcm_close(alsa_data->out_handle);
		alsa_data->out_handle = 0;
	}
	return 0;
}

static void
rdpsnd_alsa_free(rdpsndDevicePlugin * devplugin)
{
	rdpsnd_alsa_close(devplugin);
	free(devplugin->device_data);
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

/* returns boolean */
static int
rdpsnd_alsa_format_supported(rdpsndDevicePlugin * devplugin, char * snd_format, int size)
{
	struct alsa_device_data * alsa_data;
	int nChannels;
	int wBitsPerSample;
	int nSamplesPerSec;
	int cbSize;
	int wFormatTag;

	alsa_data = (struct alsa_device_data *) devplugin->device_data;

	LLOGLN(10, ("rdpsnd_alsa_format_supported: size %d", size));
	wFormatTag = GET_UINT16(snd_format, 0);
	nChannels = GET_UINT16(snd_format, 2);
	nSamplesPerSec = GET_UINT32(snd_format, 4);
	wBitsPerSample = GET_UINT16(snd_format, 14);
	cbSize = GET_UINT16(snd_format, 16);
	if (cbSize == 0 &&
		(nSamplesPerSec == 22050 || nSamplesPerSec == 44100) &&
		(wBitsPerSample == 8 || wBitsPerSample == 16) &&
		(nChannels == 1 || nChannels == 2) &&
		wFormatTag == 1) /* WAVE_FORMAT_PCM */
	{
		LLOGLN(0, ("rdpsnd_alsa_format_supported: ok"));
		return 1;
	}
	return 0;
}

static int
rdpsnd_alsa_set_format(rdpsndDevicePlugin * devplugin, char * snd_format, int size)
{
	struct alsa_device_data * alsa_data;
	int nChannels;
	int wBitsPerSample;
	int nSamplesPerSec;

	alsa_data = (struct alsa_device_data *) devplugin->device_data;

	nChannels = GET_UINT16(snd_format, 2);
	nSamplesPerSec = GET_UINT32(snd_format, 4);
	wBitsPerSample = GET_UINT16(snd_format, 14);
	LLOGLN(0, ("rdpsnd_alsa_set_format: nChannels %d "
		"nSamplesPerSec %d wBitsPerSample %d",
		nChannels, nSamplesPerSec, wBitsPerSample));
	alsa_data->source_rate = nSamplesPerSec;
	alsa_data->actual_rate = nSamplesPerSec;
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
	set_params(alsa_data);
	return 0;
}

static int
rdpsnd_alsa_set_volume(rdpsndDevicePlugin * devplugin, uint32 value)
{
	LLOGLN(0, ("rdpsnd_alsa_set_volume: %8.8x", value));
	return 0;
}

static int
rdpsnd_alsa_play(rdpsndDevicePlugin * devplugin, char * data, int size, int * delay_ms)
{
	struct alsa_device_data * alsa_data;
	char * resampled_data;
	int len;
	int error;
	int frames;
	int bytes_per_frame;
	char * pindex;
	char * end;

	alsa_data = (struct alsa_device_data *) devplugin->device_data;

	LLOGLN(10, ("rdpsnd_alsa_play: size %d", size));

	bytes_per_frame = alsa_data->num_channels * alsa_data->bytes_per_channel;
	if ((size % bytes_per_frame) != 0)
	{
		LLOGLN(0, ("rdpsnd_alsa_play: error len mod"));
		return 1;
	}

	if (alsa_data->source_rate == alsa_data->actual_rate)
	{
		resampled_data = data;
	}
	else
	{
		resampled_data = (char*)alsa_data->pResample((uint8*)data, bytes_per_frame,
			alsa_data->source_rate, size / bytes_per_frame,
			alsa_data->actual_rate, &frames);
		LLOGLN(10, ("rdpsnd_alsa_play: resampled %d frames at %d to %d frames at %d",
			size / bytes_per_frame, alsa_data->source_rate, frames, alsa_data->actual_rate));
		size = frames * bytes_per_frame;
	}

	pindex = resampled_data;
	end = pindex + size;
	while (pindex < end)
	{
		len = end - pindex;
		frames = len / bytes_per_frame;
		error = snd_pcm_writei(alsa_data->out_handle, pindex, frames);
		if (error == -EPIPE)
		{
			LLOGLN(0, ("rdpsnd_alsa_play: underrun occurred"));
			snd_pcm_recover(alsa_data->out_handle, error, 0);
			error = 0;
		}
		else if (error < 0)
		{
			LLOGLN(0, ("rdpsnd_alsa_play: error len %d", error));
			break;
		}
		pindex += error * bytes_per_frame;
	}

	if (resampled_data != data)
		free(resampled_data);

	*delay_ms = size * 1000 / (bytes_per_frame * alsa_data->actual_rate) - 10;

	return 0;
}

int
FreeRDPRdpsndDeviceEntry(PFREERDP_RDPSND_DEVICE_ENTRY_POINTS pEntryPoints)
{
	rdpsndDevicePlugin * devplugin;
	struct alsa_device_data * alsa_data;
	RD_PLUGIN_DATA * data;
	int i;

	devplugin = pEntryPoints->pRegisterRdpsndDevice(pEntryPoints->plugin);
	if (devplugin == NULL)
	{
		LLOGLN(0, ("rdpsnd_alsa: unable to register device."));
		return 1;
	}

	devplugin->open = rdpsnd_alsa_open;
	devplugin->format_supported = rdpsnd_alsa_format_supported;
	devplugin->set_format = rdpsnd_alsa_set_format;
	devplugin->set_volume = rdpsnd_alsa_set_volume;
	devplugin->play = rdpsnd_alsa_play;
	devplugin->close = rdpsnd_alsa_close;
	devplugin->free = rdpsnd_alsa_free;

	alsa_data = (struct alsa_device_data *) malloc(sizeof(struct alsa_device_data));
	memset(alsa_data, 0, sizeof(struct alsa_device_data));

	data = (RD_PLUGIN_DATA *) pEntryPoints->data;
	if (data && strcmp(data->data[0], "alsa") == 0)
	{
		for (i = 1; i < 4 && data->data[i]; i++)
		{
			if (i > 1)
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
	alsa_data->out_handle = 0;
	alsa_data->source_rate = 22050;
	alsa_data->actual_rate = 22050;
	alsa_data->format = SND_PCM_FORMAT_S16_LE;
	alsa_data->num_channels = 2;
	alsa_data->bytes_per_channel = 2;
	alsa_data->pResample = pEntryPoints->pResample;
	devplugin->device_data = alsa_data;

	LLOGLN(0, ("rdpsnd_alsa: alsa device '%s' registered.", alsa_data->device_name));

	return 0;
}

