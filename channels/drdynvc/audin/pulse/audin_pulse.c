/*
   FreeRDP: A Remote Desktop Protocol client.
   Audio Input Redirection Virtual Channel - PulseAudio implementation

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
#include <pulse/pulseaudio.h>
#include "drdynvc_types.h"
#include "audin_types.h"

struct pulse_device_data
{
	char device_name[32];
	uint32 frames_per_packet;
	pa_threaded_mainloop *mainloop;
	pa_context *context;
	pa_sample_spec sample_spec;
	pa_stream *stream;
	int format;
	int block_size;
	audinDspAdpcm adpcm;

	int bytes_per_frame;
	char * buffer;
	int buffer_frames;

	audin_receive_func receive_func;
	void * user_data;

	PAUDINDSPENCODEIMAADPCM pEncodeImaAdpcm;
};

static void
audin_pulse_context_state_callback(pa_context * context, void * userdata)
{
	audinDevicePlugin * devplugin;
	struct pulse_device_data * pulse_data;
	pa_context_state_t state;

	devplugin = (audinDevicePlugin *) userdata;
	pulse_data = (struct pulse_device_data *) devplugin->device_data;
	state = pa_context_get_state(context);
	switch (state)
	{
		case PA_CONTEXT_READY:
			LLOGLN(10, ("audin_pulse_context_state_callback: PA_CONTEXT_READY"));
			pa_threaded_mainloop_signal (pulse_data->mainloop, 0);
			break;

		case PA_CONTEXT_FAILED:
		case PA_CONTEXT_TERMINATED:
			LLOGLN(10, ("audin_pulse_context_state_callback: state %d", (int)state));
			pa_threaded_mainloop_signal (pulse_data->mainloop, 0);
			break;

		default:
			LLOGLN(10, ("audin_pulse_context_state_callback: state %d", (int)state));
			break;
	}
}

static int
audin_pulse_connect(audinDevicePlugin * devplugin)
{
	struct pulse_device_data * pulse_data;
	pa_context_state_t state;

	pulse_data = (struct pulse_device_data *) devplugin->device_data;
	if (!pulse_data->context)
		return 1;

	if (pa_context_connect(pulse_data->context, NULL, 0, NULL))
	{
		LLOGLN(0, ("audin_pulse_connect: pa_context_connect failed (%d)",
			pa_context_errno(pulse_data->context)));
		return 1;
	}
	pa_threaded_mainloop_lock(pulse_data->mainloop);
	if (pa_threaded_mainloop_start(pulse_data->mainloop) < 0)
	{
		pa_threaded_mainloop_unlock(pulse_data->mainloop);
		LLOGLN(0, ("audin_pulse_connect: pa_threaded_mainloop_start failed (%d)",
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
			LLOGLN(0, ("audin_pulse_connect: bad context state (%d)",
				pa_context_errno(pulse_data->context)));
			break;
		}
		pa_threaded_mainloop_wait(pulse_data->mainloop);
	}
	pa_threaded_mainloop_unlock(pulse_data->mainloop);
	if (state == PA_CONTEXT_READY)
	{
		LLOGLN(0, ("audin_pulse_connect: connected"));
		return 0;
	}
	else
	{
		pa_context_disconnect(pulse_data->context);
		return 1;
	}
}

void
audin_pulse_free(audinDevicePlugin * devplugin)
{
	struct pulse_device_data * pulse_data = (struct pulse_device_data *) devplugin->device_data;

	LLOGLN(10, ("audin_pulse_free:"));
	if (!pulse_data)
		return;
	if (pulse_data->mainloop)
	{
		pa_threaded_mainloop_stop(pulse_data->mainloop);
	}
	if (pulse_data->context)
	{
		pa_context_disconnect(pulse_data->context);
		pa_context_unref(pulse_data->context);
		pulse_data->context = NULL;
	}
	if (pulse_data->mainloop)
	{
		pa_threaded_mainloop_free(pulse_data->mainloop);
		pulse_data->mainloop = NULL;
	}
	free(pulse_data);
}

int
audin_pulse_format_supported(audinDevicePlugin * devplugin, char * snd_format, int size)
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
	LLOGLN(10, ("audin_pulse_format_supported: wFormatTag=%d "
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
				LLOGLN(0, ("audin_pulse_format_supported: ok"));
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
				LLOGLN(0, ("audin_pulse_format_supported: ok"));
				return 1;
			}
			break;

		case 0x11: /* IMA ADPCM */
			if ((nSamplesPerSec <= PA_RATE_MAX) &&
				(wBitsPerSample == 4) &&
				(nChannels == 1 || nChannels == 2))
			{
				LLOGLN(0, ("audin_pulse_format_supported: ok"));
				return 1;
			}
			break;
	}
	return 0;
}

int
audin_pulse_set_format(audinDevicePlugin * devplugin, uint32 FramesPerPacket, char * snd_format, int size)
{
	struct pulse_device_data * pulse_data;
	pa_sample_spec sample_spec = { 0 };
	int nChannels;
	int wBitsPerSample;
	int nSamplesPerSec;
	int wFormatTag;
	int nBlockAlign;
	int bs;

	pulse_data = (struct pulse_device_data *) devplugin->device_data;
	if (!pulse_data->context)
		return 1;
	wFormatTag = GET_UINT16(snd_format, 0);
	nChannels = GET_UINT16(snd_format, 2);
	nSamplesPerSec = GET_UINT32(snd_format, 4);
	nBlockAlign = GET_UINT16(snd_format, 12);
	wBitsPerSample = GET_UINT16(snd_format, 14);
	if (FramesPerPacket > 0)
	{
		pulse_data->frames_per_packet = FramesPerPacket;
	}
	LLOGLN(0, ("audin_pulse_set_format: wFormatTag=%d nChannels=%d "
		"nSamplesPerSec=%d wBitsPerSample=%d nBlockAlign=%d FramesPerPacket=%d",
		wFormatTag, nChannels, nSamplesPerSec, wBitsPerSample, nBlockAlign,
		pulse_data->frames_per_packet));

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
			bs = (nBlockAlign - 4 * nChannels) * 4;
			pulse_data->frames_per_packet = (pulse_data->frames_per_packet * nChannels * 2 /
				bs + 1) * bs / (nChannels * 2);
			LLOGLN(0, ("audin_pulse_set_format: aligned FramesPerPacket=%d",
				pulse_data->frames_per_packet));
			break;
	}

	pulse_data->sample_spec = sample_spec;
	pulse_data->format = wFormatTag;
	pulse_data->block_size = nBlockAlign;

	return 0;
}

static void
audin_pulse_stream_state_callback(pa_stream * stream, void * userdata)
{
	audinDevicePlugin * devplugin;
	struct pulse_device_data * pulse_data;
	pa_stream_state_t state;

	devplugin = (audinDevicePlugin *) userdata;
	pulse_data = (struct pulse_device_data *) devplugin->device_data;
	state = pa_stream_get_state(stream);
	switch (state)
	{
		case PA_STREAM_READY:
			LLOGLN(10, ("audin_pulse_stream_state_callback: PA_STREAM_READY"));
			pa_threaded_mainloop_signal (pulse_data->mainloop, 0);
			break;

		case PA_STREAM_FAILED:
		case PA_STREAM_TERMINATED:
			LLOGLN(10, ("audin_pulse_stream_state_callback: state %d", (int)state));
			pa_threaded_mainloop_signal (pulse_data->mainloop, 0);
			break;

		default:
			LLOGLN(10, ("audin_pulse_stream_state_callback: state %d", (int)state));
			break;
	}
}

static void
audin_pulse_stream_request_callback(pa_stream * stream, size_t length, void * userdata)
{
	audinDevicePlugin * devplugin;
	struct pulse_device_data * pulse_data;
	const void * data;
	const char * src;
	int frames;
	int cframes;
	char * encoded_data;
	int encoded_size;
	int ret;

	devplugin = (audinDevicePlugin *) userdata;
	pulse_data = (struct pulse_device_data *) devplugin->device_data;

	pa_stream_peek(stream, &data, &length);
	frames = length / pulse_data->bytes_per_frame;
	LLOGLN(10, ("audin_pulse_stream_request_callback: length %d frames %d", length, frames));

	src = (const char *) data;
	while (frames > 0)
	{
		cframes = pulse_data->frames_per_packet - pulse_data->buffer_frames;
		if (cframes > frames)
			cframes = frames;
		memcpy(pulse_data->buffer + pulse_data->buffer_frames * pulse_data->bytes_per_frame,
			src, cframes * pulse_data->bytes_per_frame);
		pulse_data->buffer_frames += cframes;
		if (pulse_data->buffer_frames >= pulse_data->frames_per_packet)
		{
			if (pulse_data->format == 0x11)
			{
				encoded_data = (char *) pulse_data->pEncodeImaAdpcm(&pulse_data->adpcm,
					(uint8 *) pulse_data->buffer, pulse_data->buffer_frames * pulse_data->bytes_per_frame,
					pulse_data->sample_spec.channels, pulse_data->block_size, &encoded_size);
				LLOGLN(10, ("audin_pulse_stream_request_callback: encoded %d to %d",
					pulse_data->buffer_frames * pulse_data->bytes_per_frame, encoded_size));
			}
			else
			{
				encoded_data = pulse_data->buffer;
				encoded_size = pulse_data->buffer_frames * pulse_data->bytes_per_frame;
			}

			ret = pulse_data->receive_func(encoded_data, encoded_size, pulse_data->user_data);
			pulse_data->buffer_frames = 0;
			if (encoded_data != pulse_data->buffer)
				free(encoded_data);
			if (ret)
				break;
		}
		src += cframes * pulse_data->bytes_per_frame;
		frames -= cframes;
	}

	pa_stream_drop(stream);
}


int
audin_pulse_close(audinDevicePlugin * devplugin)
{
	struct pulse_device_data * pulse_data;

	pulse_data = (struct pulse_device_data *) devplugin->device_data;
	if (!pulse_data->context || !pulse_data->stream)
		return 1;
	LLOGLN(0, ("audin_pulse_close:"));
	pa_threaded_mainloop_lock(pulse_data->mainloop);
	pa_stream_disconnect(pulse_data->stream);
	pa_stream_unref(pulse_data->stream);
	pulse_data->stream = NULL;
	pa_threaded_mainloop_unlock(pulse_data->mainloop);

	pulse_data->receive_func = NULL;
	pulse_data->user_data = NULL;
	if (pulse_data->buffer)
	{
		free(pulse_data->buffer);
		pulse_data->buffer = NULL;
		pulse_data->buffer_frames = 0;
	}
	return 0;
}

int
audin_pulse_open(audinDevicePlugin * devplugin, audin_receive_func receive_func, void * user_data)
{
	struct pulse_device_data * pulse_data;
	pa_stream_state_t state;
	pa_buffer_attr buffer_attr = { 0 };

	pulse_data = (struct pulse_device_data *) devplugin->device_data;
	if (!pulse_data->context)
		return 1;
	if (!pulse_data->sample_spec.rate || pulse_data->stream)
		return 0;
	LLOGLN(10, ("audin_pulse_open:"));

	pulse_data->receive_func = receive_func;
	pulse_data->user_data = user_data;

	pa_threaded_mainloop_lock(pulse_data->mainloop);
	pulse_data->stream = pa_stream_new(pulse_data->context, "freerdp_audin",
		&pulse_data->sample_spec, NULL);
	if (!pulse_data->stream)
	{
		pa_threaded_mainloop_unlock(pulse_data->mainloop);
		LLOGLN(0, ("audin_pulse_open: pa_stream_new failed (%d)",
			pa_context_errno(pulse_data->context)));
		return 1;
	}
	pulse_data->bytes_per_frame = pa_frame_size(&pulse_data->sample_spec);
	pa_stream_set_state_callback(pulse_data->stream,
		audin_pulse_stream_state_callback, devplugin);
	pa_stream_set_read_callback(pulse_data->stream,
		audin_pulse_stream_request_callback, devplugin);
	buffer_attr.maxlength = (uint32_t) -1;
	buffer_attr.tlength = (uint32_t) -1;
	buffer_attr.prebuf = (uint32_t) -1;
	buffer_attr.minreq = (uint32_t) -1;
	/* 500ms latency */
	buffer_attr.fragsize = pa_usec_to_bytes(500000, &pulse_data->sample_spec);
	if (pa_stream_connect_record(pulse_data->stream,
		pulse_data->device_name[0] ? pulse_data->device_name : NULL,
		&buffer_attr, PA_STREAM_ADJUST_LATENCY) < 0)
	{
		pa_threaded_mainloop_unlock(pulse_data->mainloop);
		LLOGLN(0, ("audin_pulse_open: pa_stream_connect_playback failed (%d)",
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
			LLOGLN(0, ("audin_pulse_open: bad stream state (%d)",
				pa_context_errno(pulse_data->context)));
			break;
		}
		pa_threaded_mainloop_wait(pulse_data->mainloop);
	}
	pa_threaded_mainloop_unlock(pulse_data->mainloop);
	if (state == PA_STREAM_READY)
	{
		memset(&pulse_data->adpcm, 0, sizeof(audinDspAdpcm));
		pulse_data->buffer = malloc(pulse_data->bytes_per_frame * pulse_data->frames_per_packet);
		pulse_data->buffer_frames = 0;
		LLOGLN(0, ("audin_pulse_open: connected"));
		return 0;
	}
	else
	{
		audin_pulse_close(devplugin);
		return 1;
	}
}

int
FreeRDPAudinDeviceEntry(PFREERDP_AUDIN_DEVICE_ENTRY_POINTS pEntryPoints)
{
	audinDevicePlugin * devplugin;
	struct pulse_device_data * pulse_data;
	RD_PLUGIN_DATA * data;
	int i;

	devplugin = pEntryPoints->pRegisterAudinDevice(pEntryPoints->plugin);
	if (devplugin == NULL)
	{
		LLOGLN(0, ("audin_pulse: unable to register device."));
		return 1;
	}

	devplugin->open = audin_pulse_open;
	devplugin->format_supported = audin_pulse_format_supported;
	devplugin->set_format = audin_pulse_set_format;
	devplugin->close = audin_pulse_close;
	devplugin->free = audin_pulse_free;

	pulse_data = (struct pulse_device_data *) malloc(sizeof(struct pulse_device_data));
	memset(pulse_data, 0, sizeof(struct pulse_device_data));

	data = (RD_PLUGIN_DATA *) pEntryPoints->data;
	if (data && strcmp(data->data[0], "audin") == 0 && strcmp(data->data[1], "pulse") == 0)
	{
		for (i = 2; i < 4 && data->data[i]; i++)
		{
			if (i > 2)
			{
				strncat(pulse_data->device_name, ":",
					sizeof(pulse_data->device_name) - strlen(pulse_data->device_name));
			}
			strncat(pulse_data->device_name, (char*)data->data[i],
				sizeof(pulse_data->device_name) - strlen(pulse_data->device_name));
		}
	}
	pulse_data->pEncodeImaAdpcm = pEntryPoints->pEncodeImaAdpcm;
	devplugin->device_data = pulse_data;

	pulse_data->mainloop = pa_threaded_mainloop_new();
	if (!pulse_data->mainloop)
	{
		LLOGLN(0, ("audin_pulse: pa_threaded_mainloop_new failed"));
		audin_pulse_free(devplugin);
		return 1;
	}
	pulse_data->context = pa_context_new(pa_threaded_mainloop_get_api(pulse_data->mainloop), "freerdp");
	if (!pulse_data->context)
	{
		LLOGLN(0, ("audin_pulse: pa_context_new failed"));
		audin_pulse_free(devplugin);
		return 1;
	}
	pa_context_set_state_callback(pulse_data->context, audin_pulse_context_state_callback, devplugin);
	if (audin_pulse_connect(devplugin))
	{
		LLOGLN(0, ("audin_pulse: audin_pulse_connect failed"));
		audin_pulse_free(devplugin);
		return 1;
	}

	LLOGLN(0, ("audin_pulse: pulse device '%s' registered.", pulse_data->device_name));

	return 0;
}

