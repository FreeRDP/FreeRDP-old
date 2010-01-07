
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
static uint32 g_rrate = 22050;
static snd_pcm_format_t g_format = SND_PCM_FORMAT_S16_LE;
static int g_num_channels = 2;
static int g_bytes_per_channel = 2;

static int
set_params(void)
{
	snd_pcm_hw_params_t * hw_params;
	int error;

	error = snd_pcm_hw_params_malloc(&hw_params);
	if (error < 0)
	{
		LLOGLN(0, ("set_params: snd_pcm_hw_params_malloc failed"));
		return 1;
	}
	snd_pcm_hw_params_any(g_out_handle, hw_params);
	snd_pcm_hw_params_set_access(g_out_handle, hw_params,
		SND_PCM_ACCESS_RW_INTERLEAVED);
	snd_pcm_hw_params_set_format(g_out_handle, hw_params,
		g_format);
	snd_pcm_hw_params_set_rate_near(g_out_handle, hw_params,
		&g_rrate, NULL);
	snd_pcm_hw_params_set_channels(g_out_handle, hw_params, g_num_channels);
	snd_pcm_hw_params(g_out_handle, hw_params);
	snd_pcm_hw_params_free(hw_params);
	snd_pcm_prepare(g_out_handle);
	return 0;
}

int
wave_out_open(void)
{
	int error;

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
	set_params();
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
wave_out_format_supported(char * snd_format, int size)
{
	int nChannels;
	int wBitsPerSample;
	int nSamplesPerSec;
	int cbSize;
	int wFormatTag;

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
wave_out_set_format(char * snd_format, int size)
{
	int nChannels;
	int wBitsPerSample;
	int nSamplesPerSec;

	nChannels = GET_UINT16(snd_format, 2);
	nSamplesPerSec = GET_UINT32(snd_format, 4);
	wBitsPerSample = GET_UINT16(snd_format, 14);
	LLOGLN(0, ("wave_out_set_format: nChannels %d "
		"nSamplesPerSec %d wBitsPerSample %d",
		nChannels, nSamplesPerSec, wBitsPerSample));
	g_rrate = nSamplesPerSec;
	g_num_channels = nChannels;
	switch (wBitsPerSample)
	{
		case 8:
			g_format = SND_PCM_FORMAT_S8;
			g_bytes_per_channel = 1;
			break;
		case 16:
			g_format = SND_PCM_FORMAT_S16_LE;
			g_bytes_per_channel = 2;
			break;
	}
	set_params();
	return 0;
}

int
wave_out_set_volume(uint32 value)
{
	LLOGLN(0, ("wave_out_set_volume: %8.8x", value));
	return 0;
}

int
wave_out_play(char * data, int size)
{
	int len;
	int error;
	int frames;
	int bytes_per_frame;
	char * pindex;
	char * end;

	LLOGLN(10, ("wave_out_play: size %d", size));
	pindex = data;
	end = pindex + size;
	while (pindex < end)
	{
		len = end - pindex;
		bytes_per_frame = g_num_channels * g_bytes_per_channel;
		if ((len % bytes_per_frame) != 0)
		{
			LLOGLN(0, ("wave_out_play: error len mod"));
			break;
		}
		frames = len / bytes_per_frame;
		error = snd_pcm_writei(g_out_handle, pindex, frames);
		if (error < 0)
		{
			LLOGLN(0, ("wave_out_play: error len %d", error));
			break;
		}
		pindex += error * bytes_per_frame;
	}
	return 0;
}
