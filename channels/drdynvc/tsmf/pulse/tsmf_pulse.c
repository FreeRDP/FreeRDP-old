/*
   FreeRDP: A Remote Desktop Protocol client.
   Video Redirection Virtual Channel - PulseAudio Device

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
#include "tsmf_audio.h"

typedef struct _TSMFAudioData TSMFAudioData;
struct _TSMFAudioData
{
	uint8 * data;
	uint32 data_size;
	uint32 data_played;
	TSMFAudioData * next;
};

typedef struct _TSMFPulseAudioDevice
{
	ITSMFAudioDevice iface;

	char device[32];
	pa_threaded_mainloop *mainloop;
	pa_context *context;
	pa_sample_spec sample_spec;
	pa_stream *stream;

	TSMFAudioData * audio_data_head;
	TSMFAudioData * audio_data_tail;
	int audio_data_length;
} TSMFPulseAudioDevice;

static void
tsmf_pulse_context_state_callback(pa_context * context, void * userdata)
{
	TSMFPulseAudioDevice * pulse = (TSMFPulseAudioDevice *) userdata;
	pa_context_state_t state;

	state = pa_context_get_state(context);
	switch (state)
	{
		case PA_CONTEXT_READY:
			LLOGLN(10, ("tsmf_pulse_context_state_callback: PA_CONTEXT_READY"));
			pa_threaded_mainloop_signal(pulse->mainloop, 0);
			break;

		case PA_CONTEXT_FAILED:
		case PA_CONTEXT_TERMINATED:
			LLOGLN(10, ("tsmf_pulse_context_state_callback: state %d", (int)state));
			pa_threaded_mainloop_signal(pulse->mainloop, 0);
			break;

		default:
			LLOGLN(10, ("tsmf_pulse_context_state_callback: state %d", (int)state));
			break;
	}
}

static int
tsmf_pulse_connect(TSMFPulseAudioDevice * pulse)
{
	pa_context_state_t state;

	if (!pulse->context)
		return 1;

	if (pa_context_connect(pulse->context, NULL, 0, NULL))
	{
		LLOGLN(0, ("tsmf_pulse_connect: pa_context_connect failed (%d)",
			pa_context_errno(pulse->context)));
		return 1;
	}
	pa_threaded_mainloop_lock(pulse->mainloop);
	if (pa_threaded_mainloop_start(pulse->mainloop) < 0)
	{
		pa_threaded_mainloop_unlock(pulse->mainloop);
		LLOGLN(0, ("tsmf_pulse_connect: pa_threaded_mainloop_start failed (%d)",
			pa_context_errno(pulse->context)));
		return 1;
	}
	for (;;)
	{
		state = pa_context_get_state(pulse->context);
		if (state == PA_CONTEXT_READY)
			break;
        if (!PA_CONTEXT_IS_GOOD(state))
		{
			LLOGLN(0, ("tsmf_pulse_connect: bad context state (%d)",
				pa_context_errno(pulse->context)));
			break;
		}
		pa_threaded_mainloop_wait(pulse->mainloop);
	}
	pa_threaded_mainloop_unlock(pulse->mainloop);
	if (state == PA_CONTEXT_READY)
	{
		LLOGLN(0, ("tsmf_pulse_connect: connected"));
		return 0;
	}
	else
	{
		pa_context_disconnect(pulse->context);
		return 1;
	}
}

static int
tsmf_pulse_open(ITSMFAudioDevice * audio, const char * device)
{
	TSMFPulseAudioDevice * pulse = (TSMFPulseAudioDevice *) audio;

	if (device)
	{
		strcpy(pulse->device, device);
	}

	pulse->mainloop = pa_threaded_mainloop_new();
	if (!pulse->mainloop)
	{
		LLOGLN(0, ("tsmf_pulse_open: pa_threaded_mainloop_new failed"));
		return 1;
	}
	pulse->context = pa_context_new(pa_threaded_mainloop_get_api(pulse->mainloop), "freerdp");
	if (!pulse->context)
	{
		LLOGLN(0, ("tsmf_pulse_open: pa_context_new failed"));
		return 1;
	}
	pa_context_set_state_callback(pulse->context, tsmf_pulse_context_state_callback, pulse);
	if (tsmf_pulse_connect(pulse))
	{
		LLOGLN(0, ("tsmf_pulse_open: tsmf_pulse_connect failed"));
		return 1;
	}

	LLOGLN(0, ("tsmf_pulse_open: open device %s", pulse->device));
	return 0;
}

static void
tsmf_pulse_stream_success_callback(pa_stream * stream, int success, void * userdata)
{
	TSMFPulseAudioDevice * pulse = (TSMFPulseAudioDevice *) userdata;

	pa_threaded_mainloop_signal(pulse->mainloop, 0);
}

static void
tsmf_pulse_wait_for_operation(TSMFPulseAudioDevice * pulse, pa_operation * operation)
{
	while (pa_operation_get_state(operation) == PA_OPERATION_RUNNING)
	{
		pa_threaded_mainloop_wait(pulse->mainloop);
	}
	pa_operation_unref(operation);
}

static void
tsmf_pulse_stream_state_callback(pa_stream * stream, void * userdata)
{
	TSMFPulseAudioDevice * pulse = (TSMFPulseAudioDevice *) userdata;
	pa_stream_state_t state;

	state = pa_stream_get_state(stream);
	switch (state)
	{
		case PA_STREAM_READY:
			LLOGLN(10, ("tsmf_pulse_stream_state_callback: PA_STREAM_READY"));
			pa_threaded_mainloop_signal (pulse->mainloop, 0);
			break;

		case PA_STREAM_FAILED:
		case PA_STREAM_TERMINATED:
			LLOGLN(10, ("tsmf_pulse_stream_state_callback: state %d", (int)state));
			pa_threaded_mainloop_signal (pulse->mainloop, 0);
			break;

		default:
			LLOGLN(10, ("tsmf_pulse_stream_state_callback: state %d", (int)state));
			break;
	}
}

static void
tsmf_pulse_stream_playback(TSMFPulseAudioDevice * pulse)
{
	TSMFAudioData * audio_data;
	int len;
	int played;
	int ret;

	if (!pulse->stream)
		return;

	len = pa_stream_writable_size(pulse->stream);
	if (len <= 0)
		return;

	while (pulse->audio_data_head && len > 0)
	{
		audio_data = pulse->audio_data_head;
		played = audio_data->data_size - audio_data->data_played;
		if (played > len)
			played = len;
		ret = pa_stream_write(pulse->stream,
			audio_data->data + audio_data->data_played,
			played, NULL, 0LL, PA_SEEK_RELATIVE);
		if (ret < 0)
		{
			LLOGLN(0, ("tsmf_pulse_stream_playback: pa_stream_write failed (%d)",
				pa_context_errno(pulse->context)));
			audio_data->data_played = audio_data->data_size;
		}
		else
		{
			audio_data->data_played += played;
			len -= played;
		}

		if (audio_data->data_played >= audio_data->data_size)
		{
			pulse->audio_data_head = audio_data->next;
			if (pulse->audio_data_head == NULL)
				pulse->audio_data_tail = NULL;
			free(audio_data->data);
			free(audio_data);
			pulse->audio_data_length--;
		}
	}
}

static void
tsmf_pulse_stream_request_callback(pa_stream * stream, size_t length, void * userdata)
{
	TSMFPulseAudioDevice * pulse = (TSMFPulseAudioDevice *) userdata;

	LLOGLN(10, ("tsmf_pulse_stream_request_callback: %d", length));

	tsmf_pulse_stream_playback(pulse);
}

static int
tsmf_pulse_close_stream(TSMFPulseAudioDevice * pulse)
{
	if (!pulse->context || !pulse->stream)
		return 1;
	LLOGLN(0, ("tsmf_pulse_close_stream:"));
	pa_threaded_mainloop_lock(pulse->mainloop);
	tsmf_pulse_wait_for_operation(pulse,
		pa_stream_drain(pulse->stream, tsmf_pulse_stream_success_callback, pulse));
	pa_stream_disconnect(pulse->stream);
	pa_stream_unref(pulse->stream);
	pulse->stream = NULL;
	pa_threaded_mainloop_unlock(pulse->mainloop);
	return 0;
}

static int
tsmf_pulse_open_stream(TSMFPulseAudioDevice * pulse)
{
	pa_stream_state_t state;
	pa_buffer_attr buffer_attr = { 0 };

	if (!pulse->context)
		return 1;

	LLOGLN(0, ("tsmf_pulse_open_stream:"));
	pa_threaded_mainloop_lock(pulse->mainloop);
	pulse->stream = pa_stream_new(pulse->context, "freerdp",
		&pulse->sample_spec, NULL);
	if (!pulse->stream)
	{
		pa_threaded_mainloop_unlock(pulse->mainloop);
		LLOGLN(0, ("tsmf_pulse_open_stream: pa_stream_new failed (%d)",
			pa_context_errno(pulse->context)));
		return 1;
	}
	pa_stream_set_state_callback(pulse->stream,
		tsmf_pulse_stream_state_callback, pulse);
	pa_stream_set_write_callback(pulse->stream,
		tsmf_pulse_stream_request_callback, pulse);
	buffer_attr.maxlength = (uint32_t) -1;
	buffer_attr.tlength = (uint32_t) -1;//pa_usec_to_bytes(2000000, &pulse->sample_spec);
	buffer_attr.prebuf = (uint32_t) -1;
	buffer_attr.minreq = (uint32_t) -1;
	buffer_attr.fragsize = (uint32_t) -1;
	if (pa_stream_connect_playback(pulse->stream,
		pulse->device[0] ? pulse->device : NULL,
		&buffer_attr, 0, NULL, NULL) < 0)
	{
		pa_threaded_mainloop_unlock(pulse->mainloop);
		LLOGLN(0, ("tsmf_pulse_open_stream: pa_stream_connect_playback failed (%d)",
			pa_context_errno(pulse->context)));
		return 1;
	}

	for (;;)
	{
		state = pa_stream_get_state(pulse->stream);
		if (state == PA_STREAM_READY)
			break;
        if (!PA_STREAM_IS_GOOD(state))
		{
			LLOGLN(0, ("tsmf_pulse_open_stream: bad stream state (%d)",
				pa_context_errno(pulse->context)));
			break;
		}
		pa_threaded_mainloop_wait(pulse->mainloop);
	}
	pa_threaded_mainloop_unlock(pulse->mainloop);
	if (state == PA_STREAM_READY)
	{
		LLOGLN(0, ("tsmf_pulse_open_stream: connected"));
		return 0;
	}
	else
	{
		tsmf_pulse_close_stream(pulse);
		return 1;
	}
}

static int
tsmf_pulse_set_format(ITSMFAudioDevice * audio, uint32 sample_rate, uint32 channels, uint32 bits_per_sample)
{
	TSMFPulseAudioDevice * pulse = (TSMFPulseAudioDevice *) audio;

	LLOGLN(0, ("tsmf_pulse_set_format: sample_rate %d channels %d bits_per_sample %d",
		sample_rate, channels, bits_per_sample));

	pulse->sample_spec.rate = sample_rate;
	pulse->sample_spec.channels = channels;
	pulse->sample_spec.format = PA_SAMPLE_S16LE;

	return tsmf_pulse_open_stream(pulse);
}

static int
tsmf_pulse_is_busy(ITSMFAudioDevice * audio)
{
	TSMFPulseAudioDevice * pulse = (TSMFPulseAudioDevice *) audio;

	return pulse->audio_data_length > 10 ? 1 : 0;
}


static int
tsmf_pulse_play(ITSMFAudioDevice * audio, uint8 * data, uint32 data_size)
{
	TSMFPulseAudioDevice * pulse = (TSMFPulseAudioDevice *) audio;
	TSMFAudioData * audio_data;

	LLOGLN(10, ("tsmf_pulse_play: data_size %d", data_size));

	audio_data = (TSMFAudioData *) malloc(sizeof(TSMFAudioData));
	audio_data->data = data;
	audio_data->data_size = data_size;
	audio_data->data_played = 0;
	audio_data->next = NULL;

	pa_threaded_mainloop_lock(pulse->mainloop);
	if (pulse->audio_data_head == NULL)
	{
		pulse->audio_data_head = audio_data;
		pulse->audio_data_tail = audio_data;
	}
	else
	{
		pulse->audio_data_tail->next = audio_data;
		pulse->audio_data_tail = audio_data;
	}
	pulse->audio_data_length++;
	tsmf_pulse_stream_playback(pulse);
	pa_threaded_mainloop_unlock(pulse->mainloop);

	return 0;
}

static void
tsmf_pulse_free(ITSMFAudioDevice * audio)
{
	TSMFPulseAudioDevice * pulse = (TSMFPulseAudioDevice *) audio;

	LLOGLN(0, ("tsmf_pulse_free:"));

	tsmf_pulse_close_stream(pulse);
	if (pulse->mainloop)
	{
		pa_threaded_mainloop_stop(pulse->mainloop);
	}
	if (pulse->context)
	{
		pa_context_disconnect(pulse->context);
		pa_context_unref(pulse->context);
		pulse->context = NULL;
	}
	if (pulse->mainloop)
	{
		pa_threaded_mainloop_free(pulse->mainloop);
		pulse->mainloop = NULL;
	}
	while (pulse->audio_data_head)
	{
		free(pulse->audio_data_head->data);
		free(pulse->audio_data_head);
		pulse->audio_data_head = pulse->audio_data_head->next;
	}

	free(pulse);
}

ITSMFAudioDevice *
TSMFAudioDeviceEntry(void)
{
	TSMFPulseAudioDevice * pulse;

	pulse = malloc(sizeof(TSMFPulseAudioDevice));
	memset(pulse, 0, sizeof(TSMFPulseAudioDevice));

	pulse->iface.Open = tsmf_pulse_open;
	pulse->iface.SetFormat = tsmf_pulse_set_format;
	pulse->iface.IsBusy = tsmf_pulse_is_busy;
	pulse->iface.Play = tsmf_pulse_play;
	pulse->iface.Free = tsmf_pulse_free;

	return (ITSMFAudioDevice *) pulse;
}

