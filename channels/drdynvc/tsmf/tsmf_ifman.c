/*
   FreeRDP: A Remote Desktop Protocol client.
   Video Redirection Virtual Channel - Interface Manipulation

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
#include "drdynvc_types.h"
#include "tsmf_constants.h"
#include "tsmf_media.h"
#include "tsmf_codec.h"
#include "tsmf_ifman.h"

int
tsmf_ifman_rim_exchange_capability_request(TSMF_IFMAN * ifman)
{
	uint32 CapabilityValue;

	CapabilityValue = GET_UINT32(ifman->input_buffer, 0);
	LLOGLN(0, ("tsmf_ifman_rim_exchange_capability_request: server CapabilityValue %d", CapabilityValue));

	ifman->output_buffer_size = 8;
	ifman->output_buffer = malloc(8);
	SET_UINT32(ifman->output_buffer, 0, 1); /* CapabilityValue */
	SET_UINT32(ifman->output_buffer, 4, 0); /* Result */

	return 0;
}

int
tsmf_ifman_exchange_capability_request(TSMF_IFMAN * ifman)
{
	uint8 * p;
	uint32 numHostCapabilities;
	uint32 i;
	uint32 CapabilityType;
	uint32 cbCapabilityLength;
	uint32 v;

	ifman->output_buffer_size = ifman->input_buffer_size + 4;
	ifman->output_buffer = malloc(ifman->output_buffer_size);
	memcpy(ifman->output_buffer, ifman->input_buffer, ifman->input_buffer_size);
	SET_UINT32(ifman->output_buffer, ifman->input_buffer_size, 0); /* Result */

	numHostCapabilities = GET_UINT32(ifman->output_buffer, 0);
	p = ifman->output_buffer + 4;
	for (i = 0; i < numHostCapabilities; i++)
	{
		CapabilityType = GET_UINT32(p, 0);
		cbCapabilityLength = GET_UINT32(p, 4);
		switch (CapabilityType)
		{
			case 1: /* Protocol version request */
				v = GET_UINT32(p, 8);
				LLOGLN(0, ("tsmf_ifman_exchange_capability_request: server protocol version %d", v));
				break;
			case 2: /* Supported platform */
				v = GET_UINT32(p, 8);
				LLOGLN(0, ("tsmf_ifman_exchange_capability_request: server supported platform %d", v));
				/* Claim that we support both MF and DShow platforms. */
				SET_UINT32(p, 8, MMREDIR_CAPABILITY_PLATFORM_MF | MMREDIR_CAPABILITY_PLATFORM_DSHOW);
				break;
			default:
				LLOGLN(0, ("tsmf_ifman_exchange_capability_request: unknown capability type %d", CapabilityType));
				break;
		}
		p += 8 + cbCapabilityLength;
	}
	ifman->output_interface_id = TSMF_INTERFACE_DEFAULT | STREAM_ID_STUB;
	return 0;
}

int
tsmf_ifman_check_format_support_request(TSMF_IFMAN * ifman)
{
	uint32 PlatformCookie;
	uint32 numMediaType;
	uint32 FormatSupported = 1;

	PlatformCookie = GET_UINT32(ifman->input_buffer, 0);
	/* NoRolloverFlags (4 bytes) ignored */
	numMediaType = GET_UINT32(ifman->input_buffer, 8);

	LLOGLN(0, ("tsmf_ifman_check_format_support_request: PlatformCookie %d numMediaType %d",
		PlatformCookie, numMediaType));

	if (tsmf_codec_check_media_type(ifman->input_buffer + 12))
		FormatSupported = 0;

	if (FormatSupported == 1)
		LLOGLN(0, ("tsmf_ifman_check_format_support_request: format ok."));

	ifman->output_buffer_size = 12;
	ifman->output_buffer = malloc(12);
	SET_UINT32(ifman->output_buffer, 0, FormatSupported);
	SET_UINT32(ifman->output_buffer, 4, PlatformCookie);
	SET_UINT32(ifman->output_buffer, 8, 0); /* Result */

	ifman->output_interface_id = TSMF_INTERFACE_DEFAULT | STREAM_ID_STUB;

	return 0;
}

int
tsmf_ifman_on_new_presentation(TSMF_IFMAN * ifman)
{
	TSMF_PRESENTATION * presentation;
	int error = 0;

	LLOGLN(0, ("tsmf_ifman_on_new_presentation:"));
	presentation = tsmf_presentation_new(ifman->input_buffer, ifman->channel_callback);
	if (presentation == NULL)
		error = 1;
	tsmf_presentation_set_audio_device(presentation, ifman->audio_name, ifman->audio_device);
	ifman->output_pending = 1;
	return error;
}

int
tsmf_ifman_add_stream(TSMF_IFMAN * ifman)
{
	TSMF_PRESENTATION * presentation;
	TSMF_STREAM * stream;
	uint32 StreamId;
	int error = 0;

	LLOGLN(0, ("tsmf_ifman_add_stream:"));
	presentation = tsmf_presentation_find_by_id(ifman->input_buffer);
	if (presentation == NULL)
		error = 1;
	else
	{
		StreamId = GET_UINT32(ifman->input_buffer, 16);
		stream = tsmf_stream_new(presentation, StreamId);
		if (stream)
			tsmf_stream_set_format(stream, ifman->decoder_name, ifman->input_buffer + 24);
	}
	ifman->output_pending = 1;
	return error;
}

int
tsmf_ifman_set_topology_request(TSMF_IFMAN * ifman)
{
	LLOGLN(0, ("tsmf_ifman_set_topology_request:"));
	ifman->output_buffer_size = 8;
	ifman->output_buffer = malloc(8);
	SET_UINT32(ifman->output_buffer, 0, 1); /* TopologyReady */
	SET_UINT32(ifman->output_buffer, 4, 0); /* Result */
	ifman->output_interface_id = TSMF_INTERFACE_DEFAULT | STREAM_ID_STUB;
	return 0;
}

int
tsmf_ifman_remove_stream(TSMF_IFMAN * ifman)
{
	TSMF_PRESENTATION * presentation;
	TSMF_STREAM * stream;
	uint32 StreamId;
	int error = 0;

	LLOGLN(0, ("tsmf_ifman_remove_stream:"));
	presentation = tsmf_presentation_find_by_id(ifman->input_buffer);
	if (presentation == NULL)
		error = 1;
	else
	{
		StreamId = GET_UINT32(ifman->input_buffer, 16);
		stream = tsmf_stream_find_by_id(presentation, StreamId);
		if (stream)
			tsmf_stream_free(stream);
		else
			error = 1;
	}
	ifman->output_pending = 1;
	return error;
}

int
tsmf_ifman_shutdown_presentation(TSMF_IFMAN * ifman)
{
	TSMF_PRESENTATION * presentation;

	LLOGLN(0, ("tsmf_ifman_shutdown_presentation:"));
	presentation = tsmf_presentation_find_by_id(ifman->input_buffer);
	if (presentation)
		tsmf_presentation_free(presentation);
	ifman->output_buffer_size = 4;
	ifman->output_buffer = malloc(4);
	SET_UINT32(ifman->output_buffer, 0, 0); /* Result */
	ifman->output_interface_id = TSMF_INTERFACE_DEFAULT | STREAM_ID_STUB;
	return 0;
}

int
tsmf_ifman_on_stream_volume(TSMF_IFMAN * ifman)
{
	LLOGLN(0, ("tsmf_ifman_on_stream_volume:"));
	ifman->output_pending = 1;
	return 0;
}

int
tsmf_ifman_on_channel_volume(TSMF_IFMAN * ifman)
{
	LLOGLN(0, ("tsmf_ifman_on_channel_volume:"));
	ifman->output_pending = 1;
	return 0;
}

int
tsmf_ifman_set_video_window(TSMF_IFMAN * ifman)
{
	LLOGLN(10, ("tsmf_ifman_set_video_window:"));
	ifman->output_pending = 1;
	return 0;
}

int
tsmf_ifman_update_geometry_info(TSMF_IFMAN * ifman)
{
	TSMF_PRESENTATION * presentation;
	uint32 numGeometryInfo;
	uint32 Left;
	uint32 Top;
	uint32 Width;
	uint32 Height;
	uint32 cbVisibleRect;
	RD_RECT * rects = NULL;
	int num_rects = 0;
	int i;
	int error = 0;

	numGeometryInfo = GET_UINT32(ifman->input_buffer, 16);
	Width = GET_UINT32(ifman->input_buffer, 20 + 12);
	Height = GET_UINT32(ifman->input_buffer, 20 + 16);
	Left = GET_UINT32(ifman->input_buffer, 20 + 20);
	Top = GET_UINT32(ifman->input_buffer, 20 + 24);

	cbVisibleRect = GET_UINT32(ifman->input_buffer, 20 + numGeometryInfo);
	num_rects = cbVisibleRect / 16;

	LLOGLN(10, ("tsmf_ifman_update_geometry_info: numGeometryInfo %d "
		"Width %d Height %d Left %d Top %d cbVisibleRect %d num_rects %d",
		numGeometryInfo, Width, Height, Left, Top, cbVisibleRect, num_rects));

	presentation = tsmf_presentation_find_by_id(ifman->input_buffer);
	if (presentation == NULL)
		error = 1;
	else
	{
		if (num_rects > 0)
		{
			rects = (RD_RECT *) malloc(sizeof(RD_RECT) * num_rects);
			for (i = 0; i < num_rects; i++)
			{
				rects[i].x = (uint16) GET_UINT32(ifman->input_buffer, 24 + numGeometryInfo + i * 16 + 4);
				rects[i].y = (uint16) GET_UINT32(ifman->input_buffer, 24 + numGeometryInfo + i * 16);
				rects[i].width = (uint16) GET_UINT32(ifman->input_buffer, 24 + numGeometryInfo + i * 16 + 12) - rects[i].x;
				rects[i].height = (uint16) GET_UINT32(ifman->input_buffer, 24 + numGeometryInfo + i * 16 + 8) - rects[i].y;
				LLOGLN(10, ("rect %d: %d %d %d %d", i, rects[i].x, rects[i].y, rects[i].width, rects[i].height));
			}
		}
		tsmf_presentation_set_geometry_info(presentation, Left, Top, Width, Height, num_rects, rects);
	}
	ifman->output_pending = 1;
	return 0;
}

int
tsmf_ifman_set_allocator(TSMF_IFMAN * ifman)
{
	LLOGLN(0, ("tsmf_ifman_set_allocator:"));
	ifman->output_pending = 1;
	return 0;
}

int
tsmf_ifman_notify_preroll(TSMF_IFMAN * ifman)
{
	LLOGLN(0, ("tsmf_ifman_notify_preroll:"));
	ifman->output_pending = 1;
	return 0;
}

int
tsmf_ifman_on_sample(TSMF_IFMAN * ifman)
{
	TSMF_PRESENTATION * presentation;
	TSMF_STREAM * stream;
	uint32 StreamId;
	uint64 SampleStartTime;
	uint64 SampleEndTime;
	uint64 ThrottleDuration;
	uint32 SampleExtensions;
	uint32 cbData;

	StreamId = GET_UINT32(ifman->input_buffer, 16);
	SampleStartTime = GET_UINT64(ifman->input_buffer, 24);
	SampleEndTime = GET_UINT64(ifman->input_buffer, 32);
	ThrottleDuration = GET_UINT64(ifman->input_buffer, 40);
	SampleExtensions = GET_UINT32(ifman->input_buffer, 52);
	cbData = GET_UINT32(ifman->input_buffer, 56);
	
	LLOGLN(10, ("tsmf_ifman_on_sample: MessageId %d StreamId %d SampleStartTime %d SampleEndTime %d "
		"ThrottleDuration %d SampleExtensions %d cbData %d",
		ifman->message_id, StreamId, (int)SampleStartTime, (int)SampleEndTime,
		(int)ThrottleDuration, SampleExtensions, cbData));

	presentation = tsmf_presentation_find_by_id(ifman->presentation_id);
	if (presentation == NULL)
	{
		LLOGLN(0, ("tsmf_ifman_on_sample: unknown presentation id"));
		return 1;
	}
	stream = tsmf_stream_find_by_id(presentation, StreamId);
	if (stream == NULL)
	{
		LLOGLN(0, ("tsmf_ifman_on_sample: unknown stream id"));
		return 1;
	}
	tsmf_stream_push_sample(stream, ifman->channel_callback,
		ifman->message_id, SampleStartTime, SampleEndTime, ThrottleDuration, SampleExtensions,
		cbData, ifman->input_buffer + 60);

	ifman->output_pending = 1;
	return 0;
}

int
tsmf_ifman_on_flush(TSMF_IFMAN * ifman)
{
	TSMF_PRESENTATION * presentation;
	uint32 StreamId;

	StreamId = GET_UINT32(ifman->input_buffer, 16);
	LLOGLN(0, ("tsmf_ifman_on_flush: StreamId %d", StreamId));

	presentation = tsmf_presentation_find_by_id(ifman->presentation_id);
	if (presentation == NULL)
	{
		LLOGLN(0, ("tsmf_ifman_on_sample: unknown presentation id"));
		return 1;
	}

	tsmf_presentation_flush(presentation);

	ifman->output_pending = 1;
	return 0;
}

int
tsmf_ifman_on_end_of_stream(TSMF_IFMAN * ifman)
{
	TSMF_PRESENTATION * presentation;
	TSMF_STREAM * stream;
	uint32 StreamId;

	presentation = tsmf_presentation_find_by_id(ifman->input_buffer);
	StreamId = GET_UINT32(ifman->input_buffer, 16);
	stream = tsmf_stream_find_by_id(presentation, StreamId);
	tsmf_stream_end(stream);

	LLOGLN(0, ("tsmf_ifman_on_end_of_stream: StreamId %d", StreamId));

	ifman->output_buffer_size = 16;
	ifman->output_buffer = malloc(16);
	SET_UINT32(ifman->output_buffer, 0, CLIENT_EVENT_NOTIFICATION); /* FunctionId */
	SET_UINT32(ifman->output_buffer, 4, StreamId); /* StreamId */
	SET_UINT32(ifman->output_buffer, 8, TSMM_CLIENT_EVENT_ENDOFSTREAM); /* EventId */
	SET_UINT32(ifman->output_buffer, 12, 0); /* cbData */
	ifman->output_interface_id = TSMF_INTERFACE_CLIENT_NOTIFICATIONS | STREAM_ID_PROXY;

	return 0;
}

int
tsmf_ifman_on_playback_started(TSMF_IFMAN * ifman)
{
	TSMF_PRESENTATION * presentation;

	LLOGLN(0, ("tsmf_ifman_on_playback_started:"));

	presentation = tsmf_presentation_find_by_id(ifman->input_buffer);
	if (presentation)
		tsmf_presentation_start(presentation);
	else
		LLOGLN(0, ("tsmf_ifman_on_playback_started: unknown presentation id"));

	ifman->output_buffer_size = 16;
	ifman->output_buffer = malloc(16);
	SET_UINT32(ifman->output_buffer, 0, CLIENT_EVENT_NOTIFICATION); /* FunctionId */
	SET_UINT32(ifman->output_buffer, 4, 0); /* StreamId */
	SET_UINT32(ifman->output_buffer, 8, TSMM_CLIENT_EVENT_START_COMPLETED); /* EventId */
	SET_UINT32(ifman->output_buffer, 12, 0); /* cbData */
	ifman->output_interface_id = TSMF_INTERFACE_CLIENT_NOTIFICATIONS | STREAM_ID_PROXY;

	return 0;
}

int
tsmf_ifman_on_playback_paused(TSMF_IFMAN * ifman)
{
	LLOGLN(0, ("tsmf_ifman_on_playback_paused:"));
	ifman->output_pending = 1;
	return 0;
}

int
tsmf_ifman_on_playback_restarted(TSMF_IFMAN * ifman)
{
	LLOGLN(0, ("tsmf_ifman_on_playback_restarted:"));
	ifman->output_pending = 1;
	return 0;
}

int
tsmf_ifman_on_playback_stopped(TSMF_IFMAN * ifman)
{
	TSMF_PRESENTATION * presentation;

	LLOGLN(0, ("tsmf_ifman_on_playback_stopped:"));

	presentation = tsmf_presentation_find_by_id(ifman->input_buffer);
	if (presentation)
		tsmf_presentation_stop(presentation);
	else
		LLOGLN(0, ("tsmf_ifman_on_playback_stopped: unknown presentation id"));

	ifman->output_buffer_size = 16;
	ifman->output_buffer = malloc(16);
	SET_UINT32(ifman->output_buffer, 0, CLIENT_EVENT_NOTIFICATION); /* FunctionId */
	SET_UINT32(ifman->output_buffer, 4, 0); /* StreamId */
	SET_UINT32(ifman->output_buffer, 8, TSMM_CLIENT_EVENT_STOP_COMPLETED); /* EventId */
	SET_UINT32(ifman->output_buffer, 12, 0); /* cbData */
	ifman->output_interface_id = TSMF_INTERFACE_CLIENT_NOTIFICATIONS | STREAM_ID_PROXY;

	return 0;
}

int
tsmf_ifman_on_playback_rate_changed(TSMF_IFMAN * ifman)
{
	LLOGLN(0, ("tsmf_ifman_on_playback_rate_changed:"));

	ifman->output_buffer_size = 16;
	ifman->output_buffer = malloc(16);
	SET_UINT32(ifman->output_buffer, 0, CLIENT_EVENT_NOTIFICATION); /* FunctionId */
	SET_UINT32(ifman->output_buffer, 4, 0); /* StreamId */
	SET_UINT32(ifman->output_buffer, 8, TSMM_CLIENT_EVENT_MONITORCHANGED); /* EventId */
	SET_UINT32(ifman->output_buffer, 12, 0); /* cbData */
	ifman->output_interface_id = TSMF_INTERFACE_CLIENT_NOTIFICATIONS | STREAM_ID_PROXY;

	return 0;
}

