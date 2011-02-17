/*
   FreeRDP: A Remote Desktop Protocol client.
   Device Redirection - Interrupt Request Packet (IRP) Processing

   Copyright 2009 Marc-Andre Moreau <marcandre.moreau@gmail.com>

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

#ifndef __IRP_H
#define __IRP_H

#include "rdpdr_types.h"

char *
irp_output_device_io_completion(IRP* irp, int * data_size);
void
irp_process_create_request(IRP* irp, char* data, int data_size);
void
irp_process_close_request(IRP* irp, char* data, int data_size);
void
irp_process_read_request(IRP* irp, char* data, int data_size);
void
irp_process_write_request(IRP* irp, char* data, int data_size);
void
irp_process_query_volume_information_request(IRP* irp, char* data, int data_size);
void
irp_process_set_volume_information_request(IRP* irp, char* data, int data_size);
void
irp_process_query_information_request(IRP* irp, char* data, int data_size);
void
irp_process_set_information_request(IRP* irp, char* data, int data_size);
void
irp_process_directory_control_request(IRP* irp, char* data, int data_size);
void
irp_process_device_control_request(IRP* irp, char* data, int data_size);
void
irp_process_file_lock_control_request(IRP* irp, char* data, int data_size);
void
irp_process_query_directory_request(IRP* irp, char* data, int data_size);
void
irp_process_notify_change_directory_request(IRP* irp, char* data, int data_size);
int
irp_get_event(IRP * irp, uint32 * result);
int
irp_file_descriptor(IRP * irp);
void
irp_get_timeouts(IRP * irp, uint32 * timeout, uint32 * interval_timeout);

#endif // __IRP_H
