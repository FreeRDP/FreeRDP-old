/*
   FreeRDP: A Remote Desktop Protocol client.
   Persistent Bitmap Cache Routines

   Copyright (C) Jeroen Meijer 2004-2008

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

#include "frdp.h"
#include "rdp.h"
#include "cache.h"
#include <freerdp/rdpset.h>

#include "pstcache.h"

#define MAX_CELL_SIZE		0x1000	/* pixels */

#define IS_PERSISTENT(id) (id < 8 && pcache->pstcache_fd[id] > 0)

/* Update mru stamp/index for a bitmap */
void
pstcache_touch_bitmap(rdpPcache * pcache, uint8 cache_id, uint16 cache_idx, uint32 stamp)
{
	int fd;

	if (!IS_PERSISTENT(cache_id) || cache_idx >= BMPCACHE2_NUM_PSTCELLS)
		return;

	fd = pcache->pstcache_fd[cache_id];
	rd_lseek_file(fd, 12 + cache_idx *
			(pcache->pstcache_Bpp * MAX_CELL_SIZE + sizeof(CELLHEADER)));
	rd_write_file(fd, &stamp, sizeof(stamp));
}

/* Load a bitmap from the persistent cache */
RD_BOOL
pstcache_load_bitmap(rdpPcache * pcache, uint8 cache_id, uint16 cache_idx)
{
	uint8 *celldata;
	int fd;
	CELLHEADER cellhdr;
	RD_HBITMAP bitmap;

	if (!(pcache->rdp->settings->bitmap_cache_persist_enable))
		return False;

	if (!IS_PERSISTENT(cache_id) || cache_idx >= BMPCACHE2_NUM_PSTCELLS)
		return False;

	fd = pcache->pstcache_fd[cache_id];
	rd_lseek_file(fd, cache_idx *
			(pcache->pstcache_Bpp * MAX_CELL_SIZE + sizeof(CELLHEADER)));
	rd_read_file(fd, &cellhdr, sizeof(CELLHEADER));
	celldata = (uint8 *) xmalloc(cellhdr.length);
	rd_read_file(fd, celldata, cellhdr.length);

	bitmap = ui_create_bitmap(pcache->rdp->inst, cellhdr.width, cellhdr.height, celldata);
	DEBUG_CACHE("Load bitmap from disk: id=%d, idx=%d, bmp=0x%x)\n", cache_id, cache_idx,
	       (unsigned int) bitmap);
	cache_put_bitmap(pcache->rdp->cache, cache_id, cache_idx, bitmap);

	xfree(celldata);
	return True;
}

/* Store a bitmap in the persistent cache */
RD_BOOL
pstcache_save_bitmap(rdpPcache * pcache, uint8 cache_id, uint16 cache_idx, uint8 * key,
		     uint8 width, uint8 height, uint16 length, uint8 * data)
{
	int fd;
	CELLHEADER cellhdr;

	if (!IS_PERSISTENT(cache_id) || cache_idx >= BMPCACHE2_NUM_PSTCELLS)
		return False;

	memcpy(cellhdr.key, key, sizeof(HASH_KEY));
	cellhdr.width = width;
	cellhdr.height = height;
	cellhdr.length = length;
	cellhdr.stamp = 0;

	fd = pcache->pstcache_fd[cache_id];
	rd_lseek_file(fd, cache_idx *
			(pcache->pstcache_Bpp * MAX_CELL_SIZE + sizeof(CELLHEADER)));
	rd_write_file(fd, &cellhdr, sizeof(CELLHEADER));
	rd_write_file(fd, data, length);

	return True;
}

/* List the bitmap keys from the persistent cache file */
int
pstcache_enumerate(rdpPcache * pcache, uint8 id, HASH_KEY * keylist)
{
	int fd, n;
	uint16 idx;
	sint16 mru_idx[0xa00];
	uint32 mru_stamp[0xa00];
	CELLHEADER cellhdr;

	if (!(pcache->rdp->settings->bitmap_cache &&
	      pcache->rdp->settings->bitmap_cache_persist_enable &&
	      IS_PERSISTENT(id)))
		return 0;

	/* The server disconnects if the bitmap cache content is sent more than once */
	if (pcache->pstcache_enumerated)
		return 0;

	DEBUG_CACHE("Persistent bitmap cache enumeration... ");
	for (idx = 0; idx < BMPCACHE2_NUM_PSTCELLS; idx++)
	{
		fd = pcache->pstcache_fd[id];
		rd_lseek_file(fd, idx *
			(pcache->pstcache_Bpp * MAX_CELL_SIZE + sizeof(CELLHEADER)));
		if (rd_read_file(fd, &cellhdr, sizeof(CELLHEADER)) <= 0)
			break;

		if (memcmp(cellhdr.key, pcache->zero_key, sizeof(HASH_KEY)) != 0)
		{
			memcpy(keylist[idx], cellhdr.key, sizeof(HASH_KEY));

			/* Pre-cache (not possible for 8 bit color depth cause it needs a colormap) */
			if (pcache->rdp->settings->bitmap_cache_precache && cellhdr.stamp &&
			    pcache->rdp->settings->server_depth > 8)
				pstcache_load_bitmap(pcache, id, idx);

			/* Sort by stamp */
			for (n = idx; n > 0 && cellhdr.stamp < mru_stamp[n - 1]; n--)
			{
				mru_idx[n] = mru_idx[n - 1];
				mru_stamp[n] = mru_stamp[n - 1];
			}

			mru_idx[n] = idx;
			mru_stamp[n] = cellhdr.stamp;
		}
		else
		{
			break;
		}
	}

	DEBUG_CACHE("%d cached bitmaps.\n", idx);

	cache_rebuild_bmpcache_linked_list(pcache->rdp->cache, id, mru_idx, idx);
	pcache->pstcache_enumerated = True;
	return idx;
}

/* initialise the persistent bitmap cache */
RD_BOOL
pstcache_init(rdpPcache * pcache, uint8 cache_id)
{
	int fd;
	char filename[256];

	if (pcache->pstcache_enumerated)
		return True;

	pcache->pstcache_fd[cache_id] = 0;

	if (!(pcache->rdp->settings->bitmap_cache &&
	      pcache->rdp->settings->bitmap_cache_persist_enable))
		return False;

	if (!rd_pstcache_mkdir())
	{
		DEBUG_CACHE("failed to get/make cache directory!\n");
		return False;
	}

	pcache->pstcache_Bpp = (pcache->rdp->settings->server_depth + 7) / 8;
	sprintf(filename, "cache/pstcache_%d_%d", cache_id, pcache->pstcache_Bpp);
	DEBUG_CACHE("persistent bitmap cache file: %s\n", filename);

	fd = rd_open_file(filename);
	if (fd == -1)
		return False;

	if (!rd_lock_file(fd, 0, 0))
	{
		ui_warning(pcache->rdp->inst, "Persistent bitmap caching is disabled. (The file is already in use)\n");
		rd_close_file(fd);
		return False;
	}

	pcache->pstcache_fd[cache_id] = fd;
	return True;
}

rdpPcache *
pcache_new(struct rdp_rdp * rdp)
{
	rdpPcache * self;

	self = (rdpPcache *) xmalloc(sizeof(rdpPcache));
	if (self != NULL)
	{
		memset(self, 0, sizeof(rdpPcache));
		self->rdp = rdp;
	}
	return self;
}

void
pcache_free(rdpPcache * pcache)
{
	if (pcache != NULL)
	{
		xfree(pcache);
	}
}
