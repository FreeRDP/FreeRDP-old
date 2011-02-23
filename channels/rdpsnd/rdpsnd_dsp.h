/*
   FreeRDP: A Remote Desktop Protocol client.
   Digital Sound Processing

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

#ifndef __RDPSND_DSP_H
#define __RDPSND_DSP_H

uint8 *
rdpsnd_dsp_resample(uint8 * src, int bytes_per_frame,
	uint32 srate, int sframes,
	uint32 rrate, int * prframes);

#endif

