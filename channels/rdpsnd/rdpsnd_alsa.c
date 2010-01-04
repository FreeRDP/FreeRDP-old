
// libasound2-dev

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <alsa/asoundlib.h>
#include "types_ui.h"
#include "chan_stream.h"

static snd_pcm_t * g_out_handle = 0;

int
wave_out_open(void)
{
	int error;

	if (g_out_handle == 0)
	{
		//printf("wave_out_open:\n");
		error = snd_pcm_open(&g_out_handle, "default",
				SND_PCM_STREAM_PLAYBACK, 0);
		if (error < 0)
		{
			printf("wave_out_open: snd_pcm_open failed\n");
		}
	}
	return 0;
}

int
wave_out_close(void)
{
	if (g_out_handle != 0)
	{
		//printf("wave_out_close:\n");
		snd_pcm_close(g_out_handle);
		g_out_handle = 0;
	}
	return 0;
}

int
wave_out_format_supported(char * snd_format, int size)
{
	int nChannels;
	int wBitsPerSample;
	int nSamplesPerSec;
	int cbSize;

	//printf("wave_out_format_supported:\n");
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
	return 0;
}

int
wave_out_set_volume(uint32 value)
{
	return 0;
}

int
wave_out_play(char * data, int size)
{
	int len;

	len = snd_pcm_writei(g_out_handle, data, 1);
	printf("wave_out_play: len %d\n", len);
	printf("-EBADFD %d -EPIPE %d -ESTRPIPE %d\n",
		-EBADFD, -EPIPE, -ESTRPIPE);
	return 0;
}
