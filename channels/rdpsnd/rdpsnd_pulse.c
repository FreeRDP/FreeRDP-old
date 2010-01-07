
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pulse/simple.h>
#include "types_ui.h"

#define LOG_LEVEL 11
#define LLOG(_level, _args) \
  do { if (_level < LOG_LEVEL) { printf _args ; } } while (0)
#define LLOGLN(_level, _args) \
  do { if (_level < LOG_LEVEL) { printf _args ; printf("\n"); } } while (0)

static pa_sample_spec * g_ss = 0;
static pa_simple * g_s = 0;

int
wave_out_open(void)
{
	int error;

	LLOGLN(10, ("wave_out_open:"));
	if (g_ss != 0)
	{
		return 0;
	}
	g_ss = (pa_sample_spec *) malloc(sizeof(pa_sample_spec));
	g_ss->format = PA_SAMPLE_S16LE;
	g_ss->rate = 22050;
	g_ss->channels = 2;
	g_s = pa_simple_new(NULL, "freerdp", PA_STREAM_PLAYBACK,
		NULL, "playback", g_ss, NULL, NULL, &error);
	if (g_s == 0)
	{
		LLOGLN(0, ("wave_out_open: pa_simple_new failed"));
	}
	return 0;
}

int
wave_out_close(void)
{
	LLOGLN(10, ("wave_out_close:"));
	if (g_ss == 0)
	{
		return 0;
	}
	pa_simple_free(g_s);
	g_s = 0;
	free(g_ss);
	g_ss = 0;
	return 0;
}

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
	LLOGLN(10, ("wave_out_play:"));
	return 0;
}
