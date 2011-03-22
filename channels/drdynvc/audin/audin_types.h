/*
   FreeRDP: A Remote Desktop Protocol client.
   Audio Input Reirection Virtual Channel

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

#ifndef __AUDIN_TYPES_H
#define __AUDIN_TYPES_H

typedef int (*audin_receive_func) (char * wave_data, int size, void * user_data);

typedef struct audin_device_plugin audinDevicePlugin;

struct audin_device_plugin
{
	int (*open) (audinDevicePlugin * devplugin, audin_receive_func receive_func, void * user_data);
	int (*format_supported) (audinDevicePlugin * devplugin, char * snd_format, int size);
	int (*set_format) (audinDevicePlugin * devplugin, uint32 FramesPerPacket, char * snd_format, int size);
	int (*close) (audinDevicePlugin * devplugin);
	void (*free) (audinDevicePlugin * devplugin);
	void * device_data;
};

struct audin_dsp_adpcm
{
	sint16 last_sample[2];
	sint16 last_step[2];
};
typedef struct audin_dsp_adpcm audinDspAdpcm;

#define AUDIN_DEVICE_EXPORT_FUNC_NAME "FreeRDPAudinDeviceEntry"

typedef audinDevicePlugin * (* PREGISTERAUDINDEVICE)(IWTSPlugin * plugin);

typedef uint8 * (* PAUDINDSPRESAMPLE)(uint8 * src, int bytes_per_sample, \
	uint32 schan, uint32 srate, int sframes, \
	uint32 rchan, uint32 rrate, int * prframes);

typedef uint8 * (* PAUDINDSPENCODEIMAADPCM)(audinDspAdpcm * adpcm, \
	uint8 * src, int size, int channels, int block_size, int * out_size);

struct _FREERDP_AUDIN_DEVICE_ENTRY_POINTS
{
	IWTSPlugin * plugin;
	PREGISTERAUDINDEVICE pRegisterAudinDevice;
	PAUDINDSPRESAMPLE pResample;
	PAUDINDSPENCODEIMAADPCM pEncodeImaAdpcm;
	void * data;
};
typedef struct _FREERDP_AUDIN_DEVICE_ENTRY_POINTS FREERDP_AUDIN_DEVICE_ENTRY_POINTS;
typedef FREERDP_AUDIN_DEVICE_ENTRY_POINTS * PFREERDP_AUDIN_DEVICE_ENTRY_POINTS;

typedef int (* PFREERDP_AUDIN_DEVICE_ENTRY)(PFREERDP_AUDIN_DEVICE_ENTRY_POINTS pEntryPoints);

#endif

