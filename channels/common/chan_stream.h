/* -*- c-basic-offset: 8 -*-
   FreeRDP: A Remote Desktop Protocol client.

   Copyright (C) Marc-Andre Moreau <marcandre.moreau@gmail.com> 2010

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#ifndef __CHAN_STREAM_H
#define __CHAN_STREAM_H

#define GET_UINT8(_p1, _offset) *(((uint8 *) _p1) + _offset)
#define GET_UINT16(_p1, _offset) *((uint16 *) (((uint8 *) _p1) + _offset))
#define GET_UINT32(_p1, _offset) *((uint32 *) (((uint8 *) _p1) + _offset))

#define SET_UINT8(_p1, _offset, _value) *(((uint8 *) _p1) + _offset) = _value
#define SET_UINT16(_p1, _offset, _value) *((uint16 *) (((uint8 *) _p1) + _offset)) = _value
#define SET_UINT32(_p1, _offset, _value) *((uint32 *) (((uint8 *) _p1) + _offset)) = _value

int
set_wstr(char* dst, int dstlen, char* src, int srclen);

#endif
