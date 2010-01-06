
// libasound2-dev

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <alsa/asoundlib.h>
#include "types_ui.h"
#include "chan_stream.h"

#define LOG_LEVEL 1
#define LLOG(_level, _args) \
  do { if (_level < LOG_LEVEL) { printf _args ; } } while (0)
#define LLOGLN(_level, _args) \
  do { if (_level < LOG_LEVEL) { printf _args ; printf("\n"); } } while (0)

static snd_pcm_t * g_out_handle = 0;

int
wave_out_open(void)
{
	int error;
	unsigned int rrate = 22050;
	snd_pcm_hw_params_t * hw_params;
	snd_pcm_uframes_t buffer_size = 1024 * 32;
	snd_pcm_uframes_t period_size = 64;
	snd_pcm_sw_params_t * sw_params;

	if (g_out_handle != 0)
	{
		return 0;
	}
	LLOGLN(0, ("wave_out_open:"));
	error = snd_pcm_open(&g_out_handle, "default",
		SND_PCM_STREAM_PLAYBACK, 0);
	if (error < 0)
	{
		LLOGLN(0, ("wave_out_open: snd_pcm_open failed"));
		return 1;
	}
	error = snd_pcm_hw_params_malloc(&hw_params);
	if (error < 0)
	{
		LLOGLN(0, ("wave_out_open: snd_pcm_hw_params_malloc failed"));
		return 1;
	}
	snd_pcm_hw_params_any(g_out_handle, hw_params);
	snd_pcm_hw_params_set_access(g_out_handle, hw_params, SND_PCM_ACCESS_RW_INTERLEAVED);
	snd_pcm_hw_params_set_format(g_out_handle, hw_params, SND_PCM_FORMAT_S16_LE);
	snd_pcm_hw_params_set_rate_near(g_out_handle, hw_params, &rrate, NULL);
	if (rrate != 22050)
	{
		LLOGLN(0, ("wave_out_open: rrate error"));
	}
	snd_pcm_hw_params_set_channels(g_out_handle, hw_params, 2);
	snd_pcm_hw_params_set_buffer_size_near(g_out_handle, hw_params, &buffer_size);
	snd_pcm_hw_params_set_period_size_near(g_out_handle, hw_params, &period_size, NULL);
	snd_pcm_hw_params(g_out_handle, hw_params);
	snd_pcm_hw_params_free(hw_params);
	snd_pcm_sw_params_malloc(&sw_params);
	snd_pcm_sw_params_current(g_out_handle, sw_params);
	snd_pcm_sw_params_set_start_threshold(g_out_handle, sw_params, buffer_size - period_size);
	snd_pcm_sw_params_set_avail_min(g_out_handle, sw_params, period_size);
	snd_pcm_sw_params(g_out_handle, sw_params);
	snd_pcm_sw_params_free(sw_params);
	snd_pcm_prepare(g_out_handle);
	return 0;
}

int
wave_out_close(void)
{
	if (g_out_handle != 0)
	{
		LLOGLN(0, ("wave_out_close:"));
		snd_pcm_close(g_out_handle);
		g_out_handle = 0;
	}
	return 0;
}

/* returns boolean */
int
wave_out_format_supported(char * snd_format, int size)
{
	int nChannels;
	int wBitsPerSample;
	int nSamplesPerSec;
	int cbSize;

	LLOGLN(10, ("wave_out_format_supported: size %d", size));
	nChannels = GET_UINT16(snd_format, 2);
	nSamplesPerSec = GET_UINT32(snd_format, 4);
	wBitsPerSample = GET_UINT16(snd_format, 14);
	cbSize = GET_UINT16(snd_format, 16);
	if (cbSize == 0 && nSamplesPerSec == 22050 &&
		wBitsPerSample == 16 && nChannels == 2)
	{
		return 1;
	}
	return 0;
}

int
wave_out_set_format(char * snd_format, int size)
{
	LLOGLN(10, ("wave_out_set_format:"));
	return 0;
}

int
wave_out_set_volume(uint32 value)
{
	LLOGLN(10, ("wave_out_set_volume:"));
	return 0;
}

int
wave_out_play(char * data, int size)
{
	int len;
	int error;
	char * p;
	char * e;

	LLOGLN(10, ("wave_out_play: size %d", size));
	p = data;
	e = p + size;
	while (p < e)
	{
		len = e - p;
		if (len > 1024 * 32)
		{
			len = 1024 * 32;
		}
		if ((len % 4) != 0)
		{
			LLOGLN(0, ("wave_out_play: error len mod 4"));
			break;
		}
		error = snd_pcm_writei(g_out_handle, p, len / 4);
		if (error < 0)
		{
			LLOGLN(0, ("wave_out_play: error len %d", error));
			break;
		}
		if (error != len / 4)
		{
			LLOGLN(0, ("wave_out_play: error %d %d", len, error));
			break;
		}
		p += len;
	}
	error = snd_pcm_wait(g_out_handle, -1);
	if (error < 0)
	{
		LLOGLN(0, ("wave_out_play: error"));
	}
	LLOGLN(10, ("-EBADFD %d -EPIPE %d -ESTRPIPE %d",
		-EBADFD, -EPIPE, -ESTRPIPE));
	return 0;
}
