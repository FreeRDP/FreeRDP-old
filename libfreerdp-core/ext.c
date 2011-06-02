/*
   FreeRDP: A Remote Desktop Protocol client.
   Extension

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
#include <freerdp/freerdp.h>
#include "frdp.h"
#include "rdp.h"
#include "ext.h"

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

static uint32 RDPEXT_CC
ext_register_extension(rdpExtPlugin * plugin)
{
	rdpExt * ext = (rdpExt *) plugin->ext;

	if (ext->num_plugins >= RDPEXT_MAX_COUNT)
	{
		printf("ext_register_extension: maximum plugin reached.\n");
		return 1;
	}
	ext->plugins[ext->num_plugins++] = plugin;
	return 0;
}

static uint32 RDPEXT_CC
ext_register_pre_connect_hook(rdpExtPlugin * plugin, PFREERDP_EXTENSION_HOOK hook)
{
	rdpExt * ext = (rdpExt *) plugin->ext;

	if (ext->num_pre_connect_hooks >= RDPEXT_MAX_COUNT)
	{
		printf("ext_register_pre_connect_hook: maximum plugin reached.\n");
		return 1;
	}
	ext->pre_connect_hooks[ext->num_pre_connect_hooks] = hook;
	ext->pre_connect_hooks_instances[ext->num_pre_connect_hooks] = plugin;
	ext->num_pre_connect_hooks++;
	return 0;
}

static uint32 RDPEXT_CC
ext_register_post_connect_hook(rdpExtPlugin * plugin, PFREERDP_EXTENSION_HOOK hook)
{
	rdpExt * ext = (rdpExt *) plugin->ext;

	if (ext->num_post_connect_hooks >= RDPEXT_MAX_COUNT)
	{
		printf("ext_register_post_connect_hook: maximum plugin reached.\n");
		return 1;
	}
	ext->post_connect_hooks[ext->num_post_connect_hooks] = hook;
	ext->post_connect_hooks_instances[ext->num_post_connect_hooks] = plugin;
	ext->num_post_connect_hooks++;
	return 0;
}

static int
ext_load_plugins(rdpExt * ext)
{
	FREERDP_EXTENSION_ENTRY_POINTS entryPoints;
	int i;
	char path[256];
	void * han;
	PFREERDP_EXTENSION_ENTRY entry;

	entryPoints.ext = ext;
	entryPoints.pRegisterExtension = ext_register_extension;
	entryPoints.pRegisterPreConnectHook = ext_register_pre_connect_hook;
	entryPoints.pRegisterPostConnectHook = ext_register_post_connect_hook;

	for (i = 0; ext->inst->settings->extensions[i].name[0]; i++)
	{
		if (strchr(ext->inst->settings->extensions[i].name, PATH_SEPARATOR) == NULL)
		{
			snprintf(path, sizeof(path), EXT_PATH "/%s." PLUGIN_EXT, ext->inst->settings->extensions[i].name);
		}
		else
		{
			snprintf(path, sizeof(path), "%s", ext->inst->settings->extensions[i].name);
		}
		han = DLOPEN(path);
		printf("ext_load_plugins: %s\n", path);
		if (han == NULL)
		{
			printf("ext_load_plugins: failed to load %s\n", path);
			continue;
		}

		entry = (PFREERDP_EXTENSION_ENTRY) DLSYM(han, RDPEXT_EXPORT_FUNC_NAME);
		if (entry == NULL)
		{
			DLCLOSE(han);
			printf("ext_load_plugins: failed to find export function in %s\n", path);
			continue;
		}

		entryPoints.data = ext->inst->settings->extensions[i].data;
		if (entry(&entryPoints) != 0)
		{
			DLCLOSE(han);
			printf("ext_load_plugins: %s entry returns error.\n", path);
			continue;
		}
	}
	return 0;
}

static int
ext_call_init(rdpExt * ext)
{
	int i;

	for (i = 0; i < ext->num_plugins; i++)
	{
		ext->plugins[i]->init(ext->plugins[i], ext->inst);
	}
	return 0;
}

static int
ext_call_uninit(rdpExt * ext)
{
	int i;

	for (i = 0; i < ext->num_plugins; i++)
	{
		ext->plugins[i]->uninit(ext->plugins[i], ext->inst);
	}
	return 0;
}

rdpExt *
ext_new(rdpRdp * rdp)
{
	rdpExt * ext;

	ext = (rdpExt *) malloc(sizeof(rdpExt));
	memset(ext, 0, sizeof(rdpExt));

	ext->inst = rdp->inst;

	ext_load_plugins(ext);
	ext_call_init(ext);

	return ext;
}

void
ext_free(rdpExt * ext)
{
	ext_call_uninit(ext);
	free(ext);
}

int
ext_pre_connect(rdpExt * ext)
{
	int i;

	for (i = 0; i < ext->num_pre_connect_hooks; i++)
	{
		ext->pre_connect_hooks[i](ext->pre_connect_hooks_instances[i], ext->inst);
	}
	return 0;
}

int
ext_post_connect(rdpExt * ext)
{
	int i;

	for (i = 0; i < ext->num_post_connect_hooks; i++)
	{
		ext->post_connect_hooks[i](ext->post_connect_hooks_instances[i], ext->inst);
	}
	return 0;
}

