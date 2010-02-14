/* -*- c-basic-offset: 8 -*-
   freerdp: A Remote Desktop Protocol client.
   Persistent Bitmap Cache routines
   Copyright (C) Jay Sorg 2009

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

#ifndef __PSTCACHE_H
#define __PSTCACHE_H

struct rdp_pcache
{
	struct rdp_rdp * rdp;
	int pstcache_Bpp;
	int pstcache_fd[8];
	RD_BOOL pstcache_enumerated;
	uint8 zero_key[8];
};
typedef struct rdp_pcache rdpPcache;

void
pstcache_touch_bitmap(rdpPcache * pcache, uint8 cache_id, uint16 cache_idx, uint32 stamp);
RD_BOOL
pstcache_load_bitmap(rdpPcache * pcache, uint8 cache_id, uint16 cache_idx);
RD_BOOL
pstcache_save_bitmap(rdpPcache * pcache, uint8 cache_id, uint16 cache_idx, uint8 * key,
		     uint8 width, uint8 height, uint16 length, uint8 * data);
int
pstcache_enumerate(rdpPcache * pcache, uint8 id, HASH_KEY * keylist);
RD_BOOL
pstcache_init(rdpPcache * pcache, uint8 cache_id);
rdpPcache *
pcache_new(struct rdp_rdp * rdp);
void
pcache_free(rdpPcache * pcache);

#endif
