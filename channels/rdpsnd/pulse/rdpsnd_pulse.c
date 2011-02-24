/*
   FreeRDP: A Remote Desktop Protocol client.

   Copyright 2011 Vic Lee

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
#include <pulse/pulseaudio.h>
#include <freerdp/types_ui.h>
#include "chan_stream.h"
#include "rdpsnd_types.h"

#define LOG_LEVEL 1
#define LLOG(_level, _args) \
  do { if (_level < LOG_LEVEL) { printf _args ; } } while (0)
#define LLOGLN(_level, _args) \
  do { if (_level < LOG_LEVEL) { printf _args ; printf("\n"); } } while (0)

struct pulse_device_data
{
	char device_name[32];
	pa_threaded_mainloop *mainloop;
	pa_context *context;
	pa_sample_spec sample_spec;
	pa_stream *stream;
	int format;
	int block_size;
	rdpsndDspAdpcm adpcm;

	PRDPSNDDSPDECODEIMAADPCM pDecodeImaAdpcm;
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
			pa_threaded_mainloop_signal (pulse_data->mainloop, 0);
			break;

		case PA_CONTEXT_FAILED:
		case PA_CONTEXT_TERMINATED:
			LLOGLN(10, ("rdpsnd_pulse_context_state_callback: state %d", (int)state));
			pa_threaded_mainloop_signal (pulse_data->mainloop, 0);
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
	pa_threaded_mainloop_lock(pulse_data->mainloop);
	if (pa_threaded_mainloop_start(pulse_data->mainloop) < 0)
	{
		pa_threaded_mainloop_unlock(pulse_data->mainloop);
		LLOGLN(0, ("rdpsnd_pulse_connect: pa_threaded_mainloop_start failed (%d)",
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
		pa_threaded_mainloop_wait(pulse_data->mainloop);
	}
	pa_threaded_mainloop_unlock(pulse_data->mainloop);
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

static void
rdpsnd_pulse_stream_success_callback(pa_stream * stream, int success, void * userdata)
{
	rdpsndDevicePlugin * devplugin;
	struct pulse_device_data * pulse_data;

	devplugin = (rdpsndDevicePlugin *) userdata;
	pulse_data = (struct pulse_device_data *) devplugin->device_data;
	pa_threaded_mainloop_signal(pulse_data->mainloop, 0);
}

static void
rdpsnd_pulse_wait_for_operation(rdpsndDevicePlugin * devplugin, pa_operation * operation)
{
	struct pulse_device_data * pulse_data;

	pulse_data = (struct pulse_device_data *) devplugin->device_data;
	while (pa_operation_get_state(operation) == PA_OPERATION_RUNNING)
	{
		pa_threaded_mainloop_wait(pulse_data->mainloop);
	}
	pa_operation_unref(operation);
}

static void
rdpsnd_pulse_stream_state_callback(pa_stream * stream, void * userdata)
{
	rdpsndDevicePlugin * devplugin;
	struct pulse_device_data * pulse_data;
	pa_stream_state_t state;

	devplugin = (rdpsndDevicePlugin *) userdata;
	pulse_data = (struct pulse_device_data *) devplugin->device_data;
	state = pa_stream_get_state(stream);
	switch (state)
	{
		case PA_STREAM_READY:
			LLOGLN(10, ("rdpsnd_pulse_stream_state_callback: PA_STREAM_READY"));
			pa_threaded_mainloop_signal (pulse_data->mainloop, 0);
			break;

		case PA_STREAM_FAILED:
		case PA_STREAM_TERMINATED:
			LLOGLN(10, ("rdpsnd_pulse_stream_state_callback: state %d", (int)state));
			pa_threaded_mainloop_signal (pulse_data->mainloop, 0);
			break;

		default:
			LLOGLN(10, ("rdpsnd_pulse_stream_state_callback: state %d", (int)state));
			break;
	}
}

static void
rdpsnd_pulse_stream_request_callback(pa_stream * stream, size_t length, void * userdata)
{
	rdpsndDevicePlugin * devplugin;
	struct pulse_device_data * pulse_data;

	devplugin = (rdpsndDevicePlugin *) userdata;
	pulse_data = (struct pulse_device_data *) devplugin->device_data;
	pa_threaded_mainloop_signal(pulse_data->mainloop, 0);
}

static int
rdpsnd_pulse_close(rdpsndDevicePlugin * devplugin)
{
	struct pulse_device_data * pulse_data;

	pulse_data = (struct pulse_device_data *) devplugin->device_data;
	if (!pulse_data->context || !pulse_data->stream)
		return 1;
	LLOGLN(0, ("rdpsnd_pulse_close:"));
	pa_threaded_mainloop_lock(pulse_data->mainloop);
	rdpsnd_pulse_wait_for_operation(devplugin,
		pa_stream_drain(pulse_data->stream, rdpsnd_pulse_stream_success_callback, devplugin));
	pa_stream_disconnect(pulse_data->stream);
	pa_stream_unref(pulse_data->stream);
	pulse_data->stream = NULL;
	pa_threaded_mainloop_unlock(pulse_data->mainloop);
	return 0;
}

static int
rdpsnd_pulse_open(rdpsndDevicePlugin * devplugin)
{
	struct pulse_device_data * pulse_data;
	pa_stream_state_t state;
	pa_buffer_attr buffer_attr = { 0 };

	pulse_data = (struct pulse_device_data *) devplugin->device_data;
	if (!pulse_data->context)
		return 1;
	/* Since rdpsnd_main calls set_format() after open(), but we need the
	   format spec to open the stream, we will defer the open request if
	   initial set_format request is not yet received */
	if (!pulse_data->sample_spec.rate || pulse_data->stream)
		return 0;
	LLOGLN(10, ("rdpsnd_pulse_open:"));
	pa_threaded_mainloop_lock(pulse_data->mainloop);
	pulse_data->stream = pa_stream_new(pulse_data->context, "freerdp",
		&pulse_data->sample_spec, NULL);
	if (!pulse_data->stream)
	{
		pa_threaded_mainloop_unlock(pulse_data->mainloop);
		LLOGLN(0, ("rdpsnd_pulse_open: pa_stream_new failed (%d)",
			pa_context_errno(pulse_data->context)));
		return 1;
	}
	pa_stream_set_state_callback(pulse_data->stream,
		rdpsnd_pulse_stream_state_callback, devplugin);
	pa_stream_set_write_callback(pulse_data->stream,
		rdpsnd_pulse_stream_request_callback, devplugin);
	buffer_attr.maxlength = (uint32_t) -1;
	buffer_attr.tlength = (uint32_t) -1;//pa_usec_to_bytes(2000000, &pulse_data->sample_spec);
	buffer_attr.prebuf = (uint32_t) -1;
	buffer_attr.minreq = (uint32_t) -1;
	buffer_attr.fragsize = (uint32_t) -1;
	if (pa_stream_connect_playback(pulse_data->stream,
		pulse_data->device_name[0] ? pulse_data->device_name : NULL,
		&buffer_attr, 0, NULL, NULL) < 0)
	{
		pa_threaded_mainloop_unlock(pulse_data->mainloop);
		LLOGLN(0, ("rdpsnd_pulse_open: pa_stream_connect_playback failed (%d)",
			pa_context_errno(pulse_data->context)));
		return 1;
	}

	for (;;)
	{
		state = pa_stream_get_state(pulse_data->stream);
		if (state == PA_STREAM_READY)
			break;
        if (!PA_STREAM_IS_GOOD(state))
		{
			LLOGLN(0, ("rdpsnd_pulse_open: bad stream state (%d)",
				pa_context_errno(pulse_data->context)));
			break;
		}
		pa_threaded_mainloop_wait(pulse_data->mainloop);
	}
	pa_threaded_mainloop_unlock(pulse_data->mainloop);
	if (state == PA_STREAM_READY)
	{
		memset(&pulse_data->adpcm, 0, sizeof(rdpsndDspAdpcm));
		LLOGLN(0, ("rdpsnd_pulse_open: connected"));
		return 0;
	}
	else
	{
		rdpsnd_pulse_close(devplugin);
		return 1;
	}
}

static void
rdpsnd_pulse_free(rdpsndDevicePlugin * devplugin)
{
	struct pulse_device_data * pulse_data;

	pulse_data = (struct pulse_device_data *) devplugin->device_data;
	LLOGLN(10, ("rdpsnd_pulse_free:"));
	rdpsnd_pulse_close(devplugin);
	if (pulse_data->mainloop)
	{
		pa_threaded_mainloop_stop(pulse_data->mainloop);
	}
	if (pulse_data->context)
	{
		pa_context_disconnect(pulse_data->context);
		pa_context_unref(pulse_data->context);
	}
	if (pulse_data->mainloop)
	{
		pa_threaded_mainloop_free(pulse_data->mainloop);
	}
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
	wFormatTag = GET_UINT16(snd_format, 0);
	nChannels = GET_UINT16(snd_format, 2);
	nSamplesPerSec = GET_UINT32(snd_format, 4);
	wBitsPerSample = GET_UINT16(snd_format, 14);
	cbSize = GET_UINT16(snd_format, 16);
	LLOGLN(10, ("rdpsnd_pulse_format_supported: wFormatTag=%d "
		"nChannels=%d nSamplesPerSec=%d wBitsPerSample=%d cbSize=%d",
		wFormatTag, nChannels, nSamplesPerSec, wBitsPerSample, cbSize));
	switch (wFormatTag)
	{
		case 1: /* PCM */
			if (cbSize == 0 &&
				(nSamplesPerSec <= PA_RATE_MAX) &&
				(wBitsPerSample == 8 || wBitsPerSample == 16) &&
				(nChannels >= 1 && nChannels <= PA_CHANNELS_MAX))
			{
				LLOGLN(0, ("rdpsnd_pulse_format_supported: ok"));
				return 1;
			}
			break;

		case 6: /* A-LAW */
		case 7: /* U-LAW */
			if (cbSize == 0 &&
				(nSamplesPerSec <= PA_RATE_MAX) &&
				(wBitsPerSample == 8) &&
				(nChannels >= 1 && nChannels <= PA_CHANNELS_MAX))
			{
				LLOGLN(0, ("rdpsnd_pulse_format_supported: ok"));
				return 1;
			}
			break;

		case 0x11: /* IMA ADPCM */
			if ((nSamplesPerSec <= PA_RATE_MAX) &&
				(wBitsPerSample == 4) &&
				(nChannels == 1 || nChannels == 2))
			{
				LLOGLN(0, ("rdpsnd_pulse_format_supported: ok"));
				return 1;
			}
			break;
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
	int wFormatTag;
	int nBlockAlign;

	pulse_data = (struct pulse_device_data *) devplugin->device_data;
	if (!pulse_data->context)
		return 1;
	wFormatTag = GET_UINT16(snd_format, 0);
	nChannels = GET_UINT16(snd_format, 2);
	nSamplesPerSec = GET_UINT32(snd_format, 4);
	nBlockAlign = GET_UINT16(snd_format, 12);
	wBitsPerSample = GET_UINT16(snd_format, 14);
	LLOGLN(0, ("rdpsnd_pulse_set_format: wFormatTag=%d nChannels=%d "
		"nSamplesPerSec=%d wBitsPerSample=%d nBlockAlign=%d",
		wFormatTag, nChannels, nSamplesPerSec, wBitsPerSample, nBlockAlign));

	sample_spec.rate = nSamplesPerSec;
	sample_spec.channels = nChannels;
	switch (wFormatTag)
	{
		case 1: /* PCM */
			switch (wBitsPerSample)
			{
				case 8:
					sample_spec.format = PA_SAMPLE_U8;
					break;
				case 16:
					sample_spec.format = PA_SAMPLE_S16LE;
					break;
			}
			break;

		case 6: /* A-LAW */
			sample_spec.format = PA_SAMPLE_ALAW;
			break;

		case 7: /* U-LAW */
			sample_spec.format = PA_SAMPLE_ULAW;
			break;

		case 0x11: /* IMA ADPCM */
			sample_spec.format = PA_SAMPLE_S16LE;
			break;
	}

	pulse_data->sample_spec = sample_spec;
	pulse_data->format = wFormatTag;
	pulse_data->block_size = nBlockAlign;

	if (pulse_data->stream)
	{
		pa_threaded_mainloop_lock(pulse_data->mainloop);
		pa_stream_disconnect(pulse_data->stream);
		pa_stream_unref(pulse_data->stream);
		pulse_data->stream = NULL;
		pa_threaded_mainloop_unlock(pulse_data->mainloop);
	}
	rdpsnd_pulse_open(devplugin);

	return 0;
}

static int
rdpsnd_pulse_set_volume(rdpsndDevicePlugin * devplugin, uint32 value)
{
	LLOGLN(0, ("rdpsnd_pulse_set_volume: %8.8x", value));
	return 0;
}

static int
rdpsnd_pulse_play(rdpsndDevicePlugin * devplugin, char * data, int size)
{
	struct pulse_device_data * pulse_data;
	int len;
	int ret;
	uint8 * decoded_data;
	char * src;
	int decoded_size;

	pulse_data = (struct pulse_device_data *) devplugin->device_data;
	if (!pulse_data->stream)
		return 1;

	if (pulse_data->format == 0x11)
	{
		decoded_data = pulse_data->pDecodeImaAdpcm(&pulse_data->adpcm,
			(uint8 *) data, size, pulse_data->sample_spec.channels, pulse_data->block_size, &decoded_size);
		size = decoded_size;
		src = (char *) decoded_data;
	}
	else
	{
		decoded_data = NULL;
		src = data;
	}

	LLOGLN(10, ("rdpsnd_pulse_play: size %d", size));

	pa_threaded_mainloop_lock(pulse_data->mainloop);
	while (size > 0)
	{
		while ((len = pa_stream_writable_size(pulse_data->stream)) == 0)
		{
			LLOGLN(10, ("rdpsnd_pulse_play: waiting"));
			pa_threaded_mainloop_wait(pulse_data->mainloop);
		}
		if (len < 0)
			break;
		if (len > size)
			len = size;
		ret = pa_stream_write(pulse_data->stream, src, len, NULL, 0LL, PA_SEEK_RELATIVE);
		if (ret < 0)
		{
			LLOGLN(0, ("rdpsnd_pulse_play: pa_stream_write failed (%d)",
				pa_context_errno(pulse_data->context)));
			break;
		}
		src += len;
		size -= len;
	}
	pa_threaded_mainloop_unlock(pulse_data->mainloop);

	if (decoded_data)
		free(decoded_data);

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
	pulse_data->pDecodeImaAdpcm = pEntryPoints->pDecodeImaAdpcm;
	devplugin->device_data = pulse_data;

	pulse_data->mainloop = pa_threaded_mainloop_new();
	if (!pulse_data->mainloop)
	{
		LLOGLN(0, ("rdpsnd_pulse: pa_threaded_mainloop_new failed"));
		return 1;
	}
	pulse_data->context = pa_context_new(pa_threaded_mainloop_get_api(pulse_data->mainloop), "freerdp");
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

