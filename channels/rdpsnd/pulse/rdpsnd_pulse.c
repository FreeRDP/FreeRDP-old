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
};

static int
rdpsnd_pulse_open(rdpsndDevicePlugin * devplugin)
{
	LLOGLN(10, ("rdpsnd_pulse_open:"));
	return 0;
}

static int
rdpsnd_pulse_close(rdpsndDevicePlugin * devplugin)
{
	LLOGLN(10, ("rdpsnd_pulse_close:"));
	return 0;
}

static void
rdpsnd_pulse_free(rdpsndDevicePlugin * devplugin)
{
	LLOGLN(10, ("rdpsnd_pulse_free:"));
}

static int
rdpsnd_pulse_format_supported(rdpsndDevicePlugin * devplugin, char * snd_format, int size)
{
	LLOGLN(10, ("rdpsnd_pulse_format_supported:"));
	return 0;
}

static int
rdpsnd_pulse_set_format(rdpsndDevicePlugin * devplugin, char * snd_format, int size)
{
	LLOGLN(10, ("rdpsnd_pulse_set_format:"));
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

	LLOGLN(0, ("rdpsnd_pulse: pulse device '%s' registered.", pulse_data->device_name));

	return 0;
}

