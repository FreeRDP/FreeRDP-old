/*
   FreeRDP: A Remote Desktop Protocol client.
   ASN.1 Encoding and Decoding

   Copyright 2009 Marc-Andre Moreau <marcandre.moreau@gmail.com>

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

#ifndef __ASN1_H
#define __ASN1_H

#include "mcs.h"

RD_BOOL
ber_parse_header(rdpMcs * mcs, STREAM s, int tagval, int *length);

void
ber_out_header(STREAM s, int tagval, int length);

void
ber_out_integer(STREAM s, int value);

#endif // __ASN1_H
