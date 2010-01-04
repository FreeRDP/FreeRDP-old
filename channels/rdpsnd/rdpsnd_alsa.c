
// libasound2-dev

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <alsa/asoundlib.h>
#include "types_ui.h"
#include "chan_stream.h"

#define LOG_LEVEL 11
#define LLOG(_level, _args) \
  do { if (_level < LOG_LEVEL) { printf _args ; } } while (0)
#define LLOGLN(_level, _args) \
  do { if (_level < LOG_LEVEL) { printf _args ; printf("\n"); } } while (0)

static snd_pcm_t * g_out_handle = 0;

int
wave_out_open(void)
{
	int error;

	if (g_out_handle == 0)
	{
		LLOGLN(10, ("wave_out_open:"));
		error = snd_pcm_open(&g_out_handle, "default",
				SND_PCM_STREAM_PLAYBACK, 0);
		if (error < 0)
		{
			LLOGLN(0, ("wave_out_open: snd_pcm_open failed"));
		}
	}
	return 0;
}

int
wave_out_close(void)
{
	if (g_out_handle != 0)
	{
		LLOGLN(10, ("wave_out_close:"));
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

	len = snd_pcm_writei(g_out_handle, data, 1);
	LLOGLN(10, ("wave_out_play: len %d", len));
	LLOGLN(10, ("-EBADFD %d -EPIPE %d -ESTRPIPE %d",
		-EBADFD, -EPIPE, -ESTRPIPE));
	return 0;
}
