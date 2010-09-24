/* -*- c-basic-offset: 8 -*-
   freerdp: A Remote Desktop Protocol client.
   Cache routines
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

#ifndef __CACHE_H
#define __CACHE_H

struct bmpcache_entry
{
	RD_HBITMAP bitmap;
	sint16 previous;
	sint16 next;
};

struct rdp_cache
{
	struct rdp_rdp * rdp;
	struct bmpcache_entry bmpcache[3][0xa00];
	RD_HBITMAP volatile_bc[3];
	RD_HBITMAP drawing_surface[100];
	int bmpcache_lru[3];
	int bmpcache_mru[3];
	int bmpcache_count[3];
	FONTGLYPH fontcache[12][256];
	DATABLOB textcache[256];
	RD_HCURSOR cursorcache[0x20];
	RD_BRUSHDATA brushcache[2][64];
};
typedef struct rdp_cache rdpCache;

void
cache_rebuild_bmpcache_linked_list(rdpCache * cache, uint8 id, sint16 * idx, int count);
void
cache_bump_bitmap(rdpCache * cache, uint8 id, uint16 idx, int bump);
void
cache_evict_bitmap(rdpCache * cache, uint8 id);
RD_HBITMAP
cache_get_bitmap(rdpCache * cache, uint8 id, uint16 idx);
void
cache_put_bitmap(rdpCache * cache, uint8 id, uint16 idx, RD_HBITMAP bitmap);
void
cache_save_state(rdpCache * cache);
FONTGLYPH *
cache_get_font(rdpCache * cache, uint8 font, uint16 character);
void
cache_put_font(rdpCache * cache, uint8 font, uint16 character, uint16 offset, uint16 baseline,
	       uint16 width, uint16 height, RD_HGLYPH pixmap);
DATABLOB *
cache_get_text(rdpCache * cache, uint8 cache_id);
void
cache_put_text(rdpCache * cache, uint8 cache_id, void *data, int length);
RD_HCURSOR
cache_get_cursor(rdpCache * cache, uint16 cache_idx);
void
cache_put_cursor(rdpCache * cache, uint16 cache_idx, RD_HCURSOR cursor);
RD_BRUSHDATA *
cache_get_brush_data(rdpCache * cache, uint8 color_code, uint8 idx);
void
cache_put_brush_data(rdpCache * cache, uint8 color_code, uint8 idx, RD_BRUSHDATA * brush_data);
rdpCache *
cache_new(struct rdp_rdp * rdp);
void
cache_free(rdpCache * cache);

#endif
