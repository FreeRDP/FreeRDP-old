/*
   FreeRDP: A Remote Desktop Protocol client.
   Video Redirection Virtual Channel - ALSA Audio Device

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
#include "tsmf_audio.h"

typedef struct _TSMFALSAAudioDevice
{
	ITSMFAudioDevice iface;

	char device[32];
	snd_pcm_t * out_handle;
	uint32 source_rate;
	uint32 actual_rate;
	uint32 source_channels;
	uint32 actual_channels;
	uint32 bytes_per_sample;
} TSMFALSAAudioDevice;

static uint8 *
tsmf_alsa_resample(uint8 * src, int bytes_per_sample,
	uint32 schan, uint32 srate, int sframes,
	uint32 rchan, uint32 rrate, int * prframes)
{
	uint8 * dst;
	uint8 * p;
	int rframes;
	int rsize;
	int i, j;
	int n1, n2;
	int sbytes, rbytes;

	sbytes = bytes_per_sample * schan;
	rbytes = bytes_per_sample * rchan;
	rframes = sframes * rrate / srate;
	*prframes = rframes;
	rsize = rbytes * rframes;
	dst = (uint8 *) malloc(rsize);
	memset(dst, 0, rsize);

	p = dst;
	for (i = 0; i < rframes; i++)
	{
		n1 = i * srate / rrate;
		if (n1 >= sframes)
			n1 = sframes - 1;
		n2 = (n1 * rrate == i * srate || n1 == sframes - 1 ? n1 : n1 + 1);
		for (j = 0; j < rbytes; j++)
		{
			/* Nearest Interpolation, probably the easiest, but works */
			*p++ = (i * srate - n1 * rrate > n2 * rrate - i * srate ?
				src[n2 * sbytes + (j % sbytes)] :
				src[n1 * sbytes + (j % sbytes)]);
		}
	}

	return dst;
}

static int
tsmf_alsa_open_device(TSMFALSAAudioDevice * alsa)
{
	int error;

	error = snd_pcm_open(&alsa->out_handle, alsa->device, SND_PCM_STREAM_PLAYBACK, 0);
	if (error < 0)
	{
		LLOGLN(0, ("tsmf_alsa_open_device: failed to open device %s", alsa->device));
		return 1;
	}

	LLOGLN(0, ("tsmf_alsa_open: open device %s", alsa->device));
	return 0;
}

static int
tsmf_alsa_open(ITSMFAudioDevice * audio, const char * device)
{
	TSMFALSAAudioDevice * alsa = (TSMFALSAAudioDevice *) audio;

	if (!device)
	{
		if (!alsa->device[0])
			strcpy(alsa->device, "default");
	}
	else
	{
		strcpy(alsa->device, device);
	}

	return tsmf_alsa_open_device(alsa);
}

static int
tsmf_alsa_set_format(ITSMFAudioDevice * audio, uint32 sample_rate, uint32 channels, uint32 bits_per_sample)
{
	TSMFALSAAudioDevice * alsa = (TSMFALSAAudioDevice *) audio;
	snd_pcm_hw_params_t * hw_params;
	snd_pcm_sw_params_t * sw_params;
	int error;
	snd_pcm_uframes_t frames;

	if (!alsa->out_handle)
		return 1;

	snd_pcm_drop(alsa->out_handle);

	alsa->actual_rate = alsa->source_rate = sample_rate;
	alsa->actual_channels = alsa->source_channels = channels;
	alsa->bytes_per_sample = bits_per_sample / 8;

	error = snd_pcm_hw_params_malloc(&hw_params);
	if (error < 0)
	{
		LLOGLN(0, ("tsmf_alsa_set_format: snd_pcm_hw_params_malloc failed"));
		return 1;
	}
	snd_pcm_hw_params_any(alsa->out_handle, hw_params);
	snd_pcm_hw_params_set_access(alsa->out_handle, hw_params,
		SND_PCM_ACCESS_RW_INTERLEAVED);
	snd_pcm_hw_params_set_format(alsa->out_handle, hw_params,
		SND_PCM_FORMAT_S16_LE);
	snd_pcm_hw_params_set_rate_near(alsa->out_handle, hw_params,
		&alsa->actual_rate, NULL);
	snd_pcm_hw_params_set_channels_near(alsa->out_handle, hw_params,
		&alsa->actual_channels);
	frames = sample_rate;
	snd_pcm_hw_params_set_buffer_size_near(alsa->out_handle, hw_params,
		&frames);
	snd_pcm_hw_params(alsa->out_handle, hw_params);
	snd_pcm_hw_params_free(hw_params);

	error = snd_pcm_sw_params_malloc(&sw_params);
	if (error < 0)
	{
		LLOGLN(0, ("tsmf_alsa_set_format: snd_pcm_sw_params_malloc"));
		return 1;
	}
	snd_pcm_sw_params_current(alsa->out_handle, sw_params);
	snd_pcm_sw_params_set_start_threshold(alsa->out_handle, sw_params,
		frames / 2);
	snd_pcm_sw_params(alsa->out_handle, sw_params);
	snd_pcm_sw_params_free(sw_params);

	snd_pcm_prepare(alsa->out_handle);

	LLOGLN(0, ("tsmf_alsa_set_format: sample_rate %d channels %d bits_per_sample %d",
		sample_rate, channels, bits_per_sample));
	LLOGLN(0, ("tsmf_alsa_set_format: hardware buffer %d frames", (int)frames));
	if ((alsa->actual_rate != alsa->source_rate) ||
		(alsa->actual_channels != alsa->source_channels))
	{
		LLOGLN(0, ("tsmf_alsa_set_format: actual rate %d / channel %d is different "
			"from source rate %d / channel %d, resampling required.",
			alsa->actual_rate, alsa->actual_channels,
			alsa->source_rate, alsa->source_channels));
	}
	return 0;
}

static int
tsmf_alsa_play(ITSMFAudioDevice * audio, uint8 * data, uint32 data_size)
{
	TSMFALSAAudioDevice * alsa = (TSMFALSAAudioDevice *) audio;
	uint8 * resampled_data;
	uint8 * src;
	int len;
	int error;
	int frames;
	int rbytes_per_frame;
	int sbytes_per_frame;
	uint8 * pindex;
	uint8 * end;

	LLOGLN(10, ("tsmf_alsa_play: data_size %d", data_size));
	if (alsa->out_handle)
	{
		sbytes_per_frame = alsa->source_channels * alsa->bytes_per_sample;
		rbytes_per_frame = alsa->actual_channels * alsa->bytes_per_sample;

		if ((alsa->source_rate == alsa->actual_rate) &&
			(alsa->source_channels == alsa->actual_channels))
		{
			resampled_data = NULL;
			src = data;
			data_size = data_size;
		}
		else
		{
			resampled_data = tsmf_alsa_resample(data, alsa->bytes_per_sample,
				alsa->source_channels, alsa->source_rate, data_size / sbytes_per_frame,
				alsa->actual_channels, alsa->actual_rate, &frames);
			LLOGLN(10, ("tsmf_alsa_play: resampled %d frames at %d to %d frames at %d",
				data_size / sbytes_per_frame, alsa->source_rate, frames, alsa->actual_rate));
			data_size = frames * rbytes_per_frame;
			src = resampled_data;
		}

		pindex = src;
		end = pindex + data_size;
		while (pindex < end)
		{
			len = end - pindex;
			frames = len / rbytes_per_frame;
			error = snd_pcm_writei(alsa->out_handle, pindex, frames);
			if (error == -EPIPE)
			{
				LLOGLN(0, ("tsmf_alsa_play: underrun occurred"));
				snd_pcm_recover(alsa->out_handle, error, 0);
				error = 0;
			}
			else if (error < 0)
			{
				LLOGLN(0, ("tsmf_alsa_play: error len %d", error));
				snd_pcm_close(alsa->out_handle);
				alsa->out_handle = 0;
				tsmf_alsa_open_device(alsa);
				break;
			}
			LLOGLN(10, ("tsmf_alsa_play: %d frames played.", error));
			if (error == 0)
				break;
			pindex += error * rbytes_per_frame;
		}

		if (resampled_data)
			free(resampled_data);
	}
	free(data);

	return 0;
}

static uint64
tsmf_alsa_get_latency(ITSMFAudioDevice * audio)
{
	TSMFALSAAudioDevice * alsa = (TSMFALSAAudioDevice *) audio;

	uint64 latency = 0;
	snd_pcm_sframes_t frames = 0;

	if (alsa->out_handle && alsa->actual_rate > 0 &&
		snd_pcm_delay(alsa->out_handle, &frames) == 0 &&
		frames > 0)
	{
		latency = ((uint64)frames) * 10000000LL / (uint64)alsa->actual_rate;
	}
	return latency;
}

static void
tsmf_alsa_flush(ITSMFAudioDevice * audio)
{
}

static void
tsmf_alsa_free(ITSMFAudioDevice * audio)
{
	TSMFALSAAudioDevice * alsa = (TSMFALSAAudioDevice *) audio;

	LLOGLN(10, ("tsmf_alsa_free:"));

	if (alsa->out_handle)
	{
		snd_pcm_drain(alsa->out_handle);
		snd_pcm_close(alsa->out_handle);
	}
	free(alsa);
}

ITSMFAudioDevice *
TSMFAudioDeviceEntry(void)
{
	TSMFALSAAudioDevice * alsa;

	alsa = malloc(sizeof(TSMFALSAAudioDevice));
	memset(alsa, 0, sizeof(TSMFALSAAudioDevice));

	alsa->iface.Open = tsmf_alsa_open;
	alsa->iface.SetFormat = tsmf_alsa_set_format;
	alsa->iface.Play = tsmf_alsa_play;
	alsa->iface.GetLatency = tsmf_alsa_get_latency;
	alsa->iface.Flush = tsmf_alsa_flush;
	alsa->iface.Free = tsmf_alsa_free;

	return (ITSMFAudioDevice *) alsa;
}

