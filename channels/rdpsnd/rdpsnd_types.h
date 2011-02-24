/*
   FreeRDP: A Remote Desktop Protocol client.
   Audio Output Virtual Channel

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
	int (*play) (rdpsndDevicePlugin * devplugin, char * data, int size);
	int (*close) (rdpsndDevicePlugin * devplugin);
	void (*free) (rdpsndDevicePlugin * devplugin);
	void * device_data;
};

struct rdpsnd_dsp_adpcm
{
	sint16 last_sample[2];
	sint16 last_step[2];
};
typedef struct rdpsnd_dsp_adpcm rdpsndDspAdpcm;

#define RDPSND_DEVICE_EXPORT_FUNC_NAME "FreeRDPRdpsndDeviceEntry"

typedef rdpsndDevicePlugin * (* PREGISTERRDPSNDDEVICE)(rdpsndPlugin * plugin);

typedef uint8 * (* PRDPSNDDSPRESAMPLE)(uint8 * src, int bytes_per_sample, \
	uint32 schan, uint32 srate, int sframes, \
	uint32 rchan, uint32 rrate, int * prframes);

typedef uint8 * (* PRDPSNDDSPDECODEIMAADPCM)(rdpsndDspAdpcm * adpcm, \
	uint8 * src, int size, int channels, int block_size, int * out_size);

struct _FREERDP_RDPSND_DEVICE_ENTRY_POINTS
{
	rdpsndPlugin * plugin;
	PREGISTERRDPSNDDEVICE pRegisterRdpsndDevice;
	PRDPSNDDSPRESAMPLE pResample;
	PRDPSNDDSPDECODEIMAADPCM pDecodeImaAdpcm;
	void * data;
};
typedef struct _FREERDP_RDPSND_DEVICE_ENTRY_POINTS FREERDP_RDPSND_DEVICE_ENTRY_POINTS;
typedef FREERDP_RDPSND_DEVICE_ENTRY_POINTS * PFREERDP_RDPSND_DEVICE_ENTRY_POINTS;

typedef int (* PFREERDP_RDPSND_DEVICE_ENTRY)(PFREERDP_RDPSND_DEVICE_ENTRY_POINTS pEntryPoints);

#endif

