/*
   FreeRDP: A Remote Desktop Protocol client.
   Rdpdr stuff for redirected Smart Card Device Service

   Copyright 2011 O.S. Systems Software Ltda.
   Copyright 2011 Eduardo Fiss Beloni <beloni@ossystems.com.br>

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

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

#include "irp.h"
#include "rdpdr_constants.h"
#include "rdpdr_types.h"
#include "devman.h"
#include <freerdp/utils/stream.h>
#include <freerdp/utils/chan_plugin.h>
#include <freerdp/utils/wait_obj.h>

#include "rdpdr_scard.h"

pthread_t scard_thread;

static void
rdpdr_scard_send_completion(rdpdrPlugin * plugin, SERVICE * srv)
{
	IRP * pending = NULL;
	char * out;
	int out_size, error;

	pending = (IRP *) srv->message((void *) 0);
	while (pending)
	{
		LLOGLN(10, ("%s sending completion %d\n", __PRETTY_FUNCTION__, plugin->open_handle));
		pending->ioStatus = RD_STATUS_SUCCESS;
		pending->outputResult = pending->outputBufferLength; /* smart card requires that */
		out = irp_output_device_io_completion(pending, &out_size);
		error = plugin->ep.pVirtualChannelWrite(plugin->open_handle, out, out_size, out);
		if (error != CHANNEL_RC_OK)
			LLOGLN(0, ("rdpdr_scard_send_completion: VirtualChannelWrite failed %d", error));

		if (pending->outputBuffer)
			free(pending->outputBuffer);
		free(pending);

		pending = (IRP *) srv->message((void *) 0);
	}
}

void *
rdpdr_scard_finished_scanner(void *arg)
{
	SERVICE * srv;
	rdpdrPlugin * plugin = (rdpdrPlugin *)arg;

	pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);

	srv = devman_get_service_by_type(plugin->devman, RDPDR_DTYP_SMARTCARD);
	if (!srv)
	{
		LLOGLN(0, ("%s: smart card service is not registered", __PRETTY_FUNCTION__));
		return NULL;
	}

	while (1)
	{
		srv->message((void *) 1); /* wait for a pending irp */
		rdpdr_scard_send_completion(plugin, srv);
	}

	return NULL;
}
