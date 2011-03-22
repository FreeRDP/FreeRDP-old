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

#ifndef __TSMF_IFMAN_H
#define __TSMF_IFMAN_H

typedef struct _TSMF_IFMAN TSMF_IFMAN;
struct _TSMF_IFMAN
{
	char * input_buffer;
	uint32 input_buffer_size;
	char * output_buffer;
	uint32 output_buffer_size;
	int output_pending;
	uint32 output_interface_id;
};

int
tsmf_ifman_process_interface_capability_request(TSMF_IFMAN * ifman);
int
tsmf_ifman_process_channel_params(TSMF_IFMAN * ifman);
int
tsmf_ifman_process_capability_request(TSMF_IFMAN * ifman);
int
tsmf_ifman_process_format_support_request(TSMF_IFMAN * ifman);

#endif

