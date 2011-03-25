/*
   FreeRDP: A Remote Desktop Protocol client.
   Video Redirection Virtual Channel - Copy Decoder and Consumer

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
#include "tsmf_decoder.h"

typedef struct _TSMFCopyDecoder
{
	ITSMFDecoder iface;
} TSMFCopyDecoder;

static int
tsmf_copy_set_format(ITSMFDecoder * decoder, const uint8 * pMediaType)
{
	return 0;
}

static int
tsmf_copy_decode(ITSMFDecoder * decoder, const uint8 * data, uint32 data_size, uint8 ** decoded_data, uint32 * decoded_size)
{
	*decoded_data = malloc(data_size);
	memcpy(*decoded_data, data, data_size);
	*decoded_size = data_size;
	return 0;
}

static void
tsmf_copy_free(ITSMFDecoder * decoder)
{
	free(decoder);
}

ITSMFDecoder *
TSMFDecoderEntry(void)
{
	TSMFCopyDecoder * decoder;

	decoder = malloc(sizeof(TSMFCopyDecoder));
	memset(decoder, 0, sizeof(TSMFCopyDecoder));
	decoder->iface.SetFormat = tsmf_copy_set_format;
	decoder->iface.Decode = tsmf_copy_decode;
	decoder->iface.Free = tsmf_copy_free;

	return (ITSMFDecoder *) decoder;
}

