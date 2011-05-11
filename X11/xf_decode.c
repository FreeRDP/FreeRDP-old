/*
   FreeRDP: A Remote Desktop Protocol client.
   UI decode

   Copyright (C) Vic Lee 2011

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
#include <librfx/rfx.h>
#include "xf_types.h"
#include "xf_decode.h"

void
xf_decode_data(xfInfo * xfi, uint8 * data, int data_size)
{
	printf("xf_decode_data: %d\n", data_size);
}

