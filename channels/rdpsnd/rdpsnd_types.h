/*
   FreeRDP: A Remote Desktop Protocol client.
   Redirected Device Manager

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

#ifndef __RDPSND_TYPES_H
#define __RDPSND_TYPES_H

typedef struct rdpsnd_plugin rdpsndPlugin;

typedef struct rdpsnd_device_plugin rdpsndDevicePlugin;

struct rdpsnd_device_plugin
{
	int (*open) (rdpsndDevicePlugin * devplugin);
	int (*format_supported) (rdpsndDevicePlugin * devplugin, char * snd_format, int size);
	int (*set_format) (rdpsndDevicePlugin * devplugin, char * snd_format, int size);
	int (*set_volume) (rdpsndDevicePlugin * devplugin, uint32 value);
	int (*play) (rdpsndDevicePlugin * devplugin, char * data, int size, int * delay_ms);
	int (*close) (rdpsndDevicePlugin * devplugin);
	void (*free) (rdpsndDevicePlugin * devplugin);
	void * device_data;
};

#define RDPSND_DEVICE_EXPORT_FUNC_NAME "FreeRDPRdpsndDeviceEntry"

typedef rdpsndDevicePlugin * (* PREGISTERRDPSNDDEVICE)(rdpsndPlugin * plugin);

typedef uint8 * (* PRDPSNDDSPRESAMPLE)(uint8 * src, int bytes_per_frame, \
	uint32 srate, int sframes, \
	uint32 rrate, int * prframes);

struct _FREERDP_RDPSND_DEVICE_ENTRY_POINTS
{
	rdpsndPlugin * plugin;
	PREGISTERRDPSNDDEVICE pRegisterRdpsndDevice;
	PRDPSNDDSPRESAMPLE pResample;
	void * data;
};
typedef struct _FREERDP_RDPSND_DEVICE_ENTRY_POINTS FREERDP_RDPSND_DEVICE_ENTRY_POINTS;
typedef FREERDP_RDPSND_DEVICE_ENTRY_POINTS * PFREERDP_RDPSND_DEVICE_ENTRY_POINTS;

typedef int (* PFREERDP_RDPSND_DEVICE_ENTRY)(PFREERDP_RDPSND_DEVICE_ENTRY_POINTS pEntryPoints);

#endif

