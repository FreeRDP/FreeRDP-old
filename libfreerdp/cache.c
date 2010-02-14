/* -*- c-basic-offset: 8 -*-
   rdesktop: A Remote Desktop Protocol client.
   Cache routines
   Copyright (C) Matthew Chapman 1999-2008
   Copyright (C) Jeroen Meijer 2005

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

#include "frdp.h"
#include "cache.h"
#include "rdp.h"
#include "pstcache.h"
#include "mem.h"
#include "debug.h"

#define NUM_ELEMENTS(array) (sizeof(array) / sizeof(array[0]))
#define IS_PERSISTENT(id) (cache->rdp->pcache->pstcache_fd[id] > 0)
#define TO_TOP -1
#define NOT_SET -1
#define IS_SET(idx) (idx >= 0)

/*
 * TODO: Test for optimal value of BUMP_COUNT. TO_TOP gives lowest cpu utilisation but using
 * a positive value will hopefully result in less frequently used bitmaps having a greater chance
 * of being evicted from the cache, and therby reducing the need to load bitmaps from disk.
 * (Jeroen)
 */
#define BUMP_COUNT 40

/* Setup the bitmap cache lru/mru linked list */
void
cache_rebuild_bmpcache_linked_list(rdpCache * cache, uint8 id, sint16 * idx, int count)
{
	int n = count, c = 0;
	sint16 n_idx;

	/* find top, skip evicted bitmaps */
	while (--n >= 0 && cache->bmpcache[id][idx[n]].bitmap == NULL);
	if (n < 0)
	{
		cache->bmpcache_mru[id] = cache->bmpcache_lru[id] = NOT_SET;
		return;
	}

	cache->bmpcache_mru[id] = idx[n];
	cache->bmpcache[id][idx[n]].next = NOT_SET;
	n_idx = idx[n];
	c++;

	/* link list */
	while (n >= 0)
	{
		/* skip evicted bitmaps */
		while (--n >= 0 && cache->bmpcache[id][idx[n]].bitmap == NULL);

		if (n < 0)
			break;

		cache->bmpcache[id][n_idx].previous = idx[n];
		cache->bmpcache[id][idx[n]].next = n_idx;
		n_idx = idx[n];
		c++;
	}

	cache->bmpcache[id][n_idx].previous = NOT_SET;
	cache->bmpcache_lru[id] = n_idx;

	if (c != cache->bmpcache_count[id])
	{
		ui_error(cache->rdp->inst, "Oops. %d in bitmap cache linked list, %d in ui "
			 "cache...\n", c, cache->bmpcache_count[id]);
		exit(1);
	}
}

/* Move a bitmap to a new position in the linked list. */
void
cache_bump_bitmap(rdpCache * cache, uint8 id, uint16 idx, int bump)
{
	int p_idx, n_idx, n;

	if (!IS_PERSISTENT(id))
		return;

	if (cache->bmpcache_mru[id] == idx)
		return;

	DEBUG_RDP5("bump bitmap: id=%d, idx=%d, bump=%d\n", id, idx, bump);

	n_idx = cache->bmpcache[id][idx].next;
	p_idx = cache->bmpcache[id][idx].previous;

	if (IS_SET(n_idx))
	{
		/* remove */
		--(cache->bmpcache_count[id]);
		if (IS_SET(p_idx))
			cache->bmpcache[id][p_idx].next = n_idx;
		else
			cache->bmpcache_lru[id] = n_idx;
		if (IS_SET(n_idx))
			cache->bmpcache[id][n_idx].previous = p_idx;
		else
			cache->bmpcache_mru[id] = p_idx;
	}
	else
	{
		p_idx = NOT_SET;
		n_idx = cache->bmpcache_lru[id];
	}

	if (bump >= 0)
	{
		for (n = 0; n < bump && IS_SET(n_idx); n++)
		{
			p_idx = n_idx;
			n_idx = cache->bmpcache[id][p_idx].next;
		}
	}
	else
	{
		p_idx = cache->bmpcache_mru[id];
		n_idx = NOT_SET;
	}

	/* insert */
	++(cache->bmpcache_count[id]);
	cache->bmpcache[id][idx].previous = p_idx;
	cache->bmpcache[id][idx].next = n_idx;

	if (p_idx >= 0)
		cache->bmpcache[id][p_idx].next = idx;
	else
		cache->bmpcache_lru[id] = idx;

	if (n_idx >= 0)
		cache->bmpcache[id][n_idx].previous = idx;
	else
		cache->bmpcache_mru[id] = idx;
}

/* Evict the least-recently used bitmap from the cache */
void
cache_evict_bitmap(rdpCache * cache, uint8 id)
{
	uint16 idx;
	int n_idx;

	if (!IS_PERSISTENT(id))
		return;

	idx = cache->bmpcache_lru[id];
	n_idx = cache->bmpcache[id][idx].next;
/*	DEBUG_RDP5("evict bitmap: id=%d idx=%d n_idx=%d bmp=0x%x\n", id, idx, n_idx,
		    cache->bmpcache[id][idx].bitmap); */

	ui_destroy_bitmap(cache->rdp->inst, cache->bmpcache[id][idx].bitmap);
	--(cache->bmpcache_count[id]);
	cache->bmpcache[id][idx].bitmap = 0;

	cache->bmpcache_lru[id] = n_idx;
	cache->bmpcache[id][n_idx].previous = NOT_SET;

	pstcache_touch_bitmap(cache->rdp->pcache, id, idx, 0);
}

/* Retrieve a bitmap from the cache */
RD_HBITMAP
cache_get_bitmap(rdpCache * cache, uint8 id, uint16 idx)
{
	if ((id < NUM_ELEMENTS(cache->bmpcache)) && (idx < NUM_ELEMENTS(cache->bmpcache[0])))
	{
		if (cache->bmpcache[id][idx].bitmap ||
		    pstcache_load_bitmap(cache->rdp->pcache, id, idx))
		{
			if (IS_PERSISTENT(id))
				cache_bump_bitmap(cache, id, idx, BUMP_COUNT);

			return cache->bmpcache[id][idx].bitmap;
		}
	}
	else if ((id < NUM_ELEMENTS(cache->volatile_bc)) && (idx == 0x7fff))
	{
		return cache->volatile_bc[id];
	}
	else if ((id == 255) && (idx < NUM_ELEMENTS(cache->drawing_surface)))
	{
		return cache->drawing_surface[idx];
	}
	ui_error(cache->rdp->inst, "get bitmap %d:%d\n", id, idx);
	return NULL;
}

/* Store a bitmap in the cache */
void
cache_put_bitmap(rdpCache * cache, uint8 id, uint16 idx, RD_HBITMAP bitmap)
{
	RD_HBITMAP old;

	if ((id < NUM_ELEMENTS(cache->bmpcache)) && (idx < NUM_ELEMENTS(cache->bmpcache[0])))
	{
		old = cache->bmpcache[id][idx].bitmap;
		if (old != NULL)
			ui_destroy_bitmap(cache->rdp->inst, old);
		cache->bmpcache[id][idx].bitmap = bitmap;

		if (IS_PERSISTENT(id))
		{
			if (old == NULL)
				cache->bmpcache[id][idx].previous =
				cache->bmpcache[id][idx].next = NOT_SET;

			cache_bump_bitmap(cache, id, idx, TO_TOP);
			if (cache->bmpcache_count[id] > BMPCACHE2_C2_CELLS)
				cache_evict_bitmap(cache, id);
		}
	}
	else if ((id < NUM_ELEMENTS(cache->volatile_bc)) && (idx == 0x7fff))
	{
		old = cache->volatile_bc[id];
		if (old != NULL)
			ui_destroy_bitmap(cache->rdp->inst, old);
		cache->volatile_bc[id] = bitmap;
	}
	else if ((id == 255) && (idx < NUM_ELEMENTS(cache->drawing_surface)))
	{
		cache->drawing_surface[idx] = bitmap;
	}
	else
	{
		ui_error(cache->rdp->inst, "put bitmap %d:%d\n", id, idx);
	}
}

/* Updates the persistent bitmap cache MRU information on exit */
void
cache_save_state(rdpCache * cache)
{
	uint32 id = 0, t = 0;
	int idx;

	for (id = 0; id < NUM_ELEMENTS(cache->bmpcache); id++)
		if (IS_PERSISTENT(id))
		{
			DEBUG_RDP5("Saving cache state for bitmap cache %d...", id);
			idx = cache->bmpcache_lru[id];
			while (idx >= 0)
			{
				pstcache_touch_bitmap(cache->rdp->pcache, id, idx, ++t);
				idx = cache->bmpcache[id][idx].next;
			}
			DEBUG_RDP5(" %d stamps written.\n", t);
		}
}

/* Retrieve a glyph from the font cache */
FONTGLYPH *
cache_get_font(rdpCache * cache, uint8 font, uint16 character)
{
	FONTGLYPH *glyph;

	if ((font < NUM_ELEMENTS(cache->fontcache)) && (character < NUM_ELEMENTS(cache->fontcache[0])))
	{
		glyph = &(cache->fontcache[font][character]);
		if (glyph->pixmap != NULL)
			return glyph;
	}

	ui_error(cache->rdp->inst, "get font %d:%d\n", font, character);
	return NULL;
}

/* Store a glyph in the font cache */
void
cache_put_font(rdpCache * cache, uint8 font, uint16 character, uint16 offset,
	       uint16 baseline, uint16 width, uint16 height, RD_HGLYPH pixmap)
{
	FONTGLYPH *glyph;

	if ((font < NUM_ELEMENTS(cache->fontcache)) && (character < NUM_ELEMENTS(cache->fontcache[0])))
	{
		glyph = &(cache->fontcache[font][character]);
		if (glyph->pixmap != NULL)
			ui_destroy_glyph(cache->rdp->inst, glyph->pixmap);

		glyph->offset = offset;
		glyph->baseline = baseline;
		glyph->width = width;
		glyph->height = height;
		glyph->pixmap = pixmap;
	}
	else
	{
		ui_error(cache->rdp->inst, "put font %d:%d\n", font, character);
	}
}

/* Retrieve a text item from the cache */
DATABLOB *
cache_get_text(rdpCache * cache, uint8 cache_id)
{
	DATABLOB *text;

	text = &(cache->textcache[cache_id]);
	return text;
}

/* Store a text item in the cache */
void
cache_put_text(rdpCache * cache, uint8 cache_id, void *data, int length)
{
	DATABLOB *text;

	text = &(cache->textcache[cache_id]);
	if (text->data != NULL)
		xfree(text->data);
	text->data = xmalloc(length);
	text->size = length;
	memcpy(text->data, data, length);
}

/* Retrieve cursor from cache */
RD_HCURSOR
cache_get_cursor(rdpCache * cache, uint16 cache_idx)
{
	RD_HCURSOR cursor;

	if (cache_idx < NUM_ELEMENTS(cache->cursorcache))
	{
		cursor = cache->cursorcache[cache_idx];
		if (cursor != NULL)
			return cursor;
	}

	ui_error(cache->rdp->inst, "get cursor %d\n", cache_idx);
	return NULL;
}

/* Store cursor in cache */
void
cache_put_cursor(rdpCache * cache, uint16 cache_idx, RD_HCURSOR cursor)
{
	RD_HCURSOR old;

	if (cache_idx < NUM_ELEMENTS(cache->cursorcache))
	{
		old = cache->cursorcache[cache_idx];
		if (old != NULL)
			ui_destroy_cursor(cache->rdp->inst, old);

		cache->cursorcache[cache_idx] = cursor;
	}
	else
	{
		ui_error(cache->rdp->inst, "put cursor %d\n", cache_idx);
	}
}

/* Retrieve brush from cache */
RD_BRUSHDATA *
cache_get_brush_data(rdpCache * cache, uint8 colour_code, uint8 idx)
{
	colour_code = colour_code == 1 ? 0 : 1;
	if (idx < NUM_ELEMENTS(cache->brushcache[0]))
	{
		return &(cache->brushcache[colour_code][idx]);
	}
	ui_error(cache->rdp->inst, "get brush %d %d\n", colour_code, idx);
	return NULL;
}

/* Store brush in cache */
/* this function takes over the data pointer in struct, eg, caller gives it up */
void
cache_put_brush_data(rdpCache * cache, uint8 colour_code, uint8 idx, RD_BRUSHDATA * brush_data)
{
	RD_BRUSHDATA *bd;

	colour_code = colour_code == 1 ? 0 : 1;
	if (idx < NUM_ELEMENTS(cache->brushcache[0]))
	{
		bd = &(cache->brushcache[colour_code][idx]);
		if (bd->data != 0)
		{
			xfree(bd->data);
		}
		memcpy(bd, brush_data, sizeof(RD_BRUSHDATA));
	}
	else
	{
		ui_error(cache->rdp->inst, "put brush %d %d\n", colour_code, idx);
	}
}

rdpCache *
cache_new(struct rdp_rdp * rdp)
{
	rdpCache * self;

	self = (rdpCache *) xmalloc(sizeof(rdpCache));
	if (self != NULL)
	{
		memset(self, 0, sizeof(rdpCache));
		self->rdp = rdp;
		self->bmpcache_lru[0] = NOT_SET;
		self->bmpcache_lru[1] = NOT_SET;
		self->bmpcache_lru[2] = NOT_SET;
		self->bmpcache_mru[0] = NOT_SET;
		self->bmpcache_mru[1] = NOT_SET;
		self->bmpcache_mru[2] = NOT_SET;
	}
	return self;
}

void
cache_free(rdpCache * cache)
{
	if (cache != NULL)
	{
		xfree(cache);
	}
}
