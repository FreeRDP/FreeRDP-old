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

#define LOG_LEVEL 1
#define LLOG(_level, _args) \
  do { if (_level < LOG_LEVEL) { printf _args ; } } while (0)
#define LLOGLN(_level, _args) \
  do { if (_level < LOG_LEVEL) { printf _args ; printf("\n"); } } while (0)

struct alsa_device_data
{
	snd_pcm_t * out_handle;
	uint32 rrate;
	snd_pcm_format_t format;
	int num_channels;
	int bytes_per_channel;
};

static int
set_params(struct alsa_device_data * alsa_data)
{
	snd_pcm_hw_params_t * hw_params;
	int error;

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
		&alsa_data->rrate, NULL);
	snd_pcm_hw_params_set_channels(alsa_data->out_handle, hw_params, alsa_data->num_channels);
	snd_pcm_hw_params(alsa_data->out_handle, hw_params);
	snd_pcm_hw_params_free(hw_params);
	snd_pcm_prepare(alsa_data->out_handle);
	return 0;
}

void *
wave_out_new(void)
{
	struct alsa_device_data * alsa_data;

	alsa_data = (struct alsa_device_data *) malloc(sizeof(struct alsa_device_data));
	memset(alsa_data, 0, sizeof(struct alsa_device_data));

	alsa_data->out_handle = 0;
	alsa_data->rrate = 22050;
	alsa_data->format = SND_PCM_FORMAT_S16_LE;
	alsa_data->num_channels = 2;
	alsa_data->bytes_per_channel = 2;

	return (void *) alsa_data;
}

void
wave_out_free(void * device_data)
{
	free(device_data);
}

int
wave_out_open(void * device_data)
{
	struct alsa_device_data * alsa_data;
	int error;

	alsa_data = (struct alsa_device_data *) device_data;

	if (alsa_data->out_handle != 0)
	{
		return 0;
	}
	LLOGLN(10, ("wave_out_open:"));
	error = snd_pcm_open(&alsa_data->out_handle, "default",
		SND_PCM_STREAM_PLAYBACK, 0);
	if (error < 0)
	{
		LLOGLN(0, ("wave_out_open: snd_pcm_open failed"));
		return 1;
	}
	set_params(alsa_data);
	return 0;
}

int
wave_out_close(void * device_data)
{
	struct alsa_device_data * alsa_data;

	alsa_data = (struct alsa_device_data *) device_data;
	if (alsa_data->out_handle != 0)
	{
		LLOGLN(10, ("wave_out_close:"));
		snd_pcm_close(alsa_data->out_handle);
		alsa_data->out_handle = 0;
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

/* returns boolean */
int
wave_out_format_supported(void * device_data, char * snd_format, int size)
{
	struct alsa_device_data * alsa_data;
	int nChannels;
	int wBitsPerSample;
	int nSamplesPerSec;
	int cbSize;
	int wFormatTag;

	alsa_data = (struct alsa_device_data *) device_data;

	LLOGLN(10, ("wave_out_format_supported: size %d", size));
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
		LLOGLN(0, ("wave_out_format_supported: ok"));
		return 1;
	}
	return 0;
}

int
wave_out_set_format(void * device_data, char * snd_format, int size)
{
	struct alsa_device_data * alsa_data;
	int nChannels;
	int wBitsPerSample;
	int nSamplesPerSec;

	alsa_data = (struct alsa_device_data *) device_data;

	nChannels = GET_UINT16(snd_format, 2);
	nSamplesPerSec = GET_UINT32(snd_format, 4);
	wBitsPerSample = GET_UINT16(snd_format, 14);
	LLOGLN(0, ("wave_out_set_format: nChannels %d "
		"nSamplesPerSec %d wBitsPerSample %d",
		nChannels, nSamplesPerSec, wBitsPerSample));
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
	set_params(alsa_data);
	return 0;
}

int
wave_out_set_volume(void * device_data, uint32 value)
{
	LLOGLN(0, ("wave_out_set_volume: %8.8x", value));
	return 0;
}

int
wave_out_play(void * device_data, char * data, int size, int * delay_ms)
{
	struct alsa_device_data * alsa_data;
	int len;
	int error;
	int frames;
	int bytes_per_frame;
	char * pindex;
	char * end;
	snd_pcm_sframes_t delay_frames = 0;

	alsa_data = (struct alsa_device_data *) device_data;

	LLOGLN(10, ("wave_out_play: size %d", size));

	bytes_per_frame = alsa_data->num_channels * alsa_data->bytes_per_channel;
	if ((size % bytes_per_frame) != 0)
	{
		LLOGLN(0, ("wave_out_play: error len mod"));
		return 1;
	}

	pindex = data;
	end = pindex + size;
	while (pindex < end)
	{
		len = end - pindex;
		frames = len / bytes_per_frame;
		error = snd_pcm_writei(alsa_data->out_handle, pindex, frames);
		if (error == -EPIPE)
		{
			LLOGLN(0, ("wave_out_play: underrun occurred"));
			snd_pcm_recover(alsa_data->out_handle, error, 0);
			error = 0;
		}
		else if (error < 0)
		{
			LLOGLN(0, ("wave_out_play: error len %d", error));
			break;
		}
		pindex += error * bytes_per_frame;
	}

	if (snd_pcm_delay(alsa_data->out_handle, &delay_frames) < 0)
	{
		delay_frames = size / bytes_per_frame;
	}
	if (delay_frames < 0)
	{
		delay_frames = 0;
	}
	*delay_ms = delay_frames * (1000000 / alsa_data->rrate) / 1000;

	return 0;
}
