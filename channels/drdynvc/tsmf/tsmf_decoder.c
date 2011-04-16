/*
   FreeRDP: A Remote Desktop Protocol client.
   Video Redirection Virtual Channel - Decoder

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
#include "tsmf_types.h"
#include "tsmf_constants.h"
#include "tsmf_decoder.h"

#ifdef _WIN32
#define DLOPEN(f) LoadLibraryA(f)
#define DLSYM(f, n) GetProcAddress(f, n)
#define DLCLOSE(f) FreeLibrary(f)
#define PATH_SEPARATOR '\\'
#define PLUGIN_EXT "dll"
#else
#include <dlfcn.h>
#define DLOPEN(f) dlopen(f, RTLD_LOCAL | RTLD_LAZY)
#define DLSYM(f, n) dlsym(f, n)
#define DLCLOSE(f) dlclose(f)
#define PATH_SEPARATOR '/'
#define PLUGIN_EXT "so"
#endif

static ITSMFDecoder *
tsmf_load_decoder_by_name(const char * name, TS_AM_MEDIA_TYPE * media_type)
{
	ITSMFDecoder * decoder;
	char path[256];
	void * han;
	TSMF_DECODER_ENTRY entry;

	if (strchr(name, PATH_SEPARATOR) == NULL)
	{
		snprintf(path, sizeof(path), PLUGIN_PATH "/tsmf_%s." PLUGIN_EXT, name);
	}
	else
	{
		snprintf(path, sizeof(path), "%s", name);
	}
	han = DLOPEN(path);
	LLOGLN(0, ("tsmf_load_decoder_by_name: %s", path));
	if (han == NULL)
	{
		LLOGLN(0, ("tsmf_load_decoder_by_name: failed to load %s", path));
		return NULL;
	}
	entry = (TSMF_DECODER_ENTRY) DLSYM(han, TSMF_DECODER_EXPORT_FUNC_NAME);
	if (entry == NULL)
	{
		DLCLOSE(han);
		LLOGLN(0, ("tsmf_load_decoder_by_name: failed to find export function in %s", path));
		return NULL;
	}
	decoder = entry();
	if (decoder == NULL)
	{
		DLCLOSE(han);
		LLOGLN(0, ("tsmf_load_decoder_by_name: failed to call export function in %s", path));
		return NULL;
	}
	if (decoder->SetFormat(decoder, media_type) != 0)
	{
		decoder->Free(decoder);
		decoder = NULL;
	}
	return decoder;
}

ITSMFDecoder *
tsmf_load_decoder(const char * name, TS_AM_MEDIA_TYPE * media_type)
{
	ITSMFDecoder * decoder;

	if (name)
	{
		decoder = tsmf_load_decoder_by_name(name, media_type);
	}
	else
	{
		decoder = tsmf_load_decoder_by_name("ffmpeg", media_type);
	}

	return decoder;
}

