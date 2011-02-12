/*
   Copyright (c) 2011 Vic Lee

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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pulse/pulseaudio.h>
#include <freerdp/types_ui.h>
#include "chan_stream.h"
#include "rdpsnd_types.h"

#define LOG_LEVEL 11
#define LLOG(_level, _args) \
  do { if (_level < LOG_LEVEL) { printf _args ; } } while (0)
#define LLOGLN(_level, _args) \
  do { if (_level < LOG_LEVEL) { printf _args ; printf("\n"); } } while (0)

struct pulse_device_data
{
	char device_name[32];
	pa_mainloop *mainloop;
	pa_context *context;
	pa_sample_spec sample_spec;
	pa_stream *stream;
};

static void
rdpsnd_pulse_context_state_callback(pa_context * context, void * userdata)
{
	rdpsndDevicePlugin * devplugin;
	struct pulse_device_data * pulse_data;
	pa_context_state_t state;

	devplugin = (rdpsndDevicePlugin *) userdata;
	pulse_data = (struct pulse_device_data *) devplugin->device_data;
	state = pa_context_get_state(context);
	switch (state)
	{
		case PA_CONTEXT_READY:
			LLOGLN(10, ("rdpsnd_pulse_context_state_callback: PA_CONTEXT_READY"));
			pa_mainloop_wakeup (pulse_data->mainloop);
			break;

		default:
			LLOGLN(10, ("rdpsnd_pulse_context_state_callback: state %d", (int)state));
			break;
	}
}

static int
rdpsnd_pulse_connect(rdpsndDevicePlugin * devplugin)
{
	struct pulse_device_data * pulse_data;
	pa_context_state_t state;

	pulse_data = (struct pulse_device_data *) devplugin->device_data;
	if (!pulse_data->context)
		return 1;

	if (pa_context_connect(pulse_data->context, NULL, 0, NULL))
	{
		LLOGLN(0, ("rdpsnd_pulse_connect: pa_context_connect failed (%d)",
			pa_context_errno(pulse_data->context)));
		return 1;
	}
	for (;;)
	{
		state = pa_context_get_state(pulse_data->context);
		if (state == PA_CONTEXT_READY)
			break;
        if (!PA_CONTEXT_IS_GOOD(state))
		{
			LLOGLN(0, ("rdpsnd_pulse_connect: bad context state (%d)",
				pa_context_errno(pulse_data->context)));
			break;
		}
		if (pa_mainloop_iterate(pulse_data->mainloop, 1, NULL) < 0)
		{
			LLOGLN(0, ("rdpsnd_pulse_connect: pa_mainloop_iterate failed"));
			break;
		}
	}
	if (state == PA_CONTEXT_READY)
	{
		LLOGLN(0, ("rdpsnd_pulse_connect: connected"));
		return 0;
	}
	else
	{
		pa_context_disconnect(pulse_data->context);
		return 1;
	}
}

static int
rdpsnd_pulse_open(rdpsndDevicePlugin * devplugin)
{
	struct pulse_device_data * pulse_data;

	pulse_data = (struct pulse_device_data *) devplugin->device_data;
	if (!pulse_data->context)
		return 1;
	/* Since RDP server sends set_format request after open request, but we
	   need the format spec to open the stream, we will defer the open request
	   if initial set_format request is not yet received */
	if (!pulse_data->sample_spec.rate || pulse_data->stream)
		return 0;
	LLOGLN(10, ("rdpsnd_pulse_open:"));

	return 0;
}

static int
rdpsnd_pulse_close(rdpsndDevicePlugin * devplugin)
{
	struct pulse_device_data * pulse_data;

	pulse_data = (struct pulse_device_data *) devplugin->device_data;
	if (!pulse_data->context || !pulse_data->stream)
		return 1;
	LLOGLN(10, ("rdpsnd_pulse_close:"));

	return 0;
}

static void
rdpsnd_pulse_free(rdpsndDevicePlugin * devplugin)
{
	struct pulse_device_data * pulse_data;

	pulse_data = (struct pulse_device_data *) devplugin->device_data;
	LLOGLN(10, ("rdpsnd_pulse_free:"));
	rdpsnd_pulse_close(devplugin);
	if (pulse_data->context)
	{
		pa_context_disconnect(pulse_data->context);
		pa_context_unref(pulse_data->context);
	}
	if (pulse_data->mainloop)
		pa_mainloop_free(pulse_data->mainloop);
	free(pulse_data);
}

static int
rdpsnd_pulse_format_supported(rdpsndDevicePlugin * devplugin, char * snd_format, int size)
{
	struct pulse_device_data * pulse_data;
	int nChannels;
	int wBitsPerSample;
	int nSamplesPerSec;
	int cbSize;
	int wFormatTag;

	pulse_data = (struct pulse_device_data *) devplugin->device_data;
	if (!pulse_data->context)
		return 0;
	LLOGLN(10, ("rdpsnd_pulse_format_supported: size %d", size));
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
		LLOGLN(0, ("rdpsnd_pulse_format_supported: ok"));
		return 1;
	}

	return 0;
}

static int
rdpsnd_pulse_set_format(rdpsndDevicePlugin * devplugin, char * snd_format, int size)
{
	struct pulse_device_data * pulse_data;
	pa_sample_spec sample_spec = { 0 };
	int nChannels;
	int wBitsPerSample;
	int nSamplesPerSec;

	pulse_data = (struct pulse_device_data *) devplugin->device_data;
	if (!pulse_data->context)
		return 1;
	nChannels = GET_UINT16(snd_format, 2);
	nSamplesPerSec = GET_UINT32(snd_format, 4);
	wBitsPerSample = GET_UINT16(snd_format, 14);

	sample_spec.rate = nSamplesPerSec;
	sample_spec.channels = nChannels;
	switch (wBitsPerSample)
	{
		case 8:
			sample_spec.format = PA_SAMPLE_U8;
			break;
		case 16:
			sample_spec.format = PA_SAMPLE_S16LE;
			break;
	}
	LLOGLN(0, ("rdpsnd_pulse_set_format: nChannels %d "
		"nSamplesPerSec %d wBitsPerSample %d",
		nChannels, nSamplesPerSec, wBitsPerSample));
	if (memcmp(&sample_spec, &pulse_data->sample_spec, sizeof(pa_sample_spec)) != 0)
	{
		pulse_data->sample_spec = sample_spec;
		rdpsnd_pulse_close(devplugin);
		rdpsnd_pulse_open(devplugin);
	}

	return 0;
}

static int
rdpsnd_pulse_set_volume(rdpsndDevicePlugin * devplugin, uint32 value)
{
	LLOGLN(0, ("rdpsnd_pulse_set_volume: %8.8x", value));
	return 0;
}

static int
rdpsnd_pulse_play(rdpsndDevicePlugin * devplugin, char * data, int size, int * delay_ms)
{
	struct pulse_device_data * pulse_data;

	pulse_data = (struct pulse_device_data *) devplugin->device_data;
	if (!pulse_data->stream)
		return 1;
	LLOGLN(10, ("rdpsnd_pulse_play: size %d", size));

	return 0;
}

int
FreeRDPRdpsndDeviceEntry(PFREERDP_RDPSND_DEVICE_ENTRY_POINTS pEntryPoints)
{
	rdpsndDevicePlugin * devplugin;
	struct pulse_device_data * pulse_data;
	RD_PLUGIN_DATA * data;
	int i;

	devplugin = pEntryPoints->pRegisterRdpsndDevice(pEntryPoints->plugin);
	if (devplugin == NULL)
	{
		LLOGLN(0, ("rdpsnd_pulse: unable to register device."));
		return 1;
	}

	devplugin->open = rdpsnd_pulse_open;
	devplugin->format_supported = rdpsnd_pulse_format_supported;
	devplugin->set_format = rdpsnd_pulse_set_format;
	devplugin->set_volume = rdpsnd_pulse_set_volume;
	devplugin->play = rdpsnd_pulse_play;
	devplugin->close = rdpsnd_pulse_close;
	devplugin->free = rdpsnd_pulse_free;

	pulse_data = (struct pulse_device_data *) malloc(sizeof(struct pulse_device_data));
	memset(pulse_data, 0, sizeof(struct pulse_device_data));

	data = (RD_PLUGIN_DATA *) pEntryPoints->data;
	if (data && strcmp(data->data[0], "pulse") == 0)
	{
		for (i = 1; i < 4 && data->data[i]; i++)
		{
			if (i > 1)
			{
				strncat(pulse_data->device_name, ":",
					sizeof(pulse_data->device_name) - strlen(pulse_data->device_name));
			}
			strncat(pulse_data->device_name, (char*)data->data[i],
				sizeof(pulse_data->device_name) - strlen(pulse_data->device_name));
		}
	}
	devplugin->device_data = pulse_data;

	pulse_data->mainloop = pa_mainloop_new();
	if (!pulse_data->mainloop)
	{
		LLOGLN(0, ("rdpsnd_pulse: pa_mainloop_new failed"));
		return 1;
	}
	pulse_data->context = pa_context_new(pa_mainloop_get_api(pulse_data->mainloop), "freerdp");
	if (!pulse_data->context)
	{
		LLOGLN(0, ("rdpsnd_pulse: pa_context_new failed"));
		return 1;
	}
	pa_context_set_state_callback(pulse_data->context, rdpsnd_pulse_context_state_callback, devplugin);
	if (rdpsnd_pulse_connect(devplugin))
	{
		LLOGLN(0, ("rdpsnd_pulse: rdpsnd_pulse_connect failed"));
		return 1;
	}

	LLOGLN(0, ("rdpsnd_pulse: pulse device '%s' registered.", pulse_data->device_name));

	return 0;
}

