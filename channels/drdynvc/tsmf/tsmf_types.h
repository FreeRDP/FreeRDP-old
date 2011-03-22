/*
   FreeRDP: A Remote Desktop Protocol client.
   Video Redirection Virtual Channel

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

#ifndef __TSMF_TYPES_H
#define __TSMF_TYPES_H

#include "drdynvc_types.h"

typedef struct _TSMF_LISTENER_CALLBACK TSMF_LISTENER_CALLBACK;
struct _TSMF_LISTENER_CALLBACK
{
	IWTSListenerCallback iface;

	IWTSPlugin * plugin;
	IWTSVirtualChannelManager * channel_mgr;
};

typedef struct _TSMF_CHANNEL_CALLBACK TSMF_CHANNEL_CALLBACK;
struct _TSMF_CHANNEL_CALLBACK
{
	IWTSVirtualChannelCallback iface;

	IWTSPlugin * plugin;
	IWTSVirtualChannelManager * channel_mgr;
	IWTSVirtualChannel * channel;

	char * input_buffer;
	uint32 input_buffer_size;
	char * output_buffer;
	uint32 output_buffer_size;
	int output_pending;
	uint32 output_interface_id;
};

typedef struct _TSMF_PLUGIN TSMF_PLUGIN;
struct _TSMF_PLUGIN
{
	IWTSPlugin iface;

	TSMF_LISTENER_CALLBACK * listener_callback;
};

#endif

