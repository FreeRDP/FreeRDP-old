/*
   Copyright (c) 2010 Vic Lee

   Permission is hereby granted, free of charge, to any person obtaining a
   copy of this software and associated documentation files (the "Software"),
   to deal in the Software without restriction, including without limitation
   the rights to use, copy, modify, merge, publish, distribute, sublicense,
   and/or sell copies of the Software, and to permit persons to whom the
   Software is furnished to do so, subject to the following conditions:

   The above copyright notice and this permission notice shall be included
   in all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
   OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
   AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
   FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
   DEALINGS IN THE SOFTWARE.

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

typedef uint8 * (* PRDPSNDDSPRESAMPLE)(uint8 * src, int bytes_per_frame, \
	uint32 srate, int sframes, \
	uint32 rrate, int * prframes);

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

