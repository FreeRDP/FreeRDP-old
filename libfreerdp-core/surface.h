/*
   FreeRDP: A Remote Desktop Protocol client.
   surface routines

   Copyright (C) Jay Sorg 2011

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

#ifndef __SURFACE_H
#define __SURFACE_H

STREAM
surface_codec_cap(rdpRdp * rdp, uint8 * codec_guid, int codec_id,
	uint8 * codec_property, int codec_properties_size);
int
surface_cmd(rdpRdp * rdp, STREAM s);

#endif
