/*
   FreeRDP: A Remote Desktop Protocol client.
   Cache routines

   Copyright (C) Jay Sorg 2009

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

#ifndef __CACHE_H
#define __CACHE_H

#include "types.h"
#include <freerdp/utils/debug.h>
#include <freerdp/utils/memory.h>
#include <freerdp/utils/datablob.h>

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

#ifdef WITH_DEBUG_CACHE
#define DEBUG_CACHE(fmt, ...) DEBUG_CLASS(CACHE, fmt, ...)
#else
#define DEBUG_CACHE(fmt, ...) DEBUG_NULL(fmt, ...)
#endif

#endif
