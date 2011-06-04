/*
   FreeRDP: A Remote Desktop Protocol client.
   Persistent Bitmap Cache Routines

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

#ifndef __PSTCACHE_H
#define __PSTCACHE_H

typedef uint8 HASH_KEY[8];

/* Header for an entry in the persistent bitmap cache file */
typedef struct _PSTCACHE_CELLHEADER
{
	HASH_KEY key;
	uint8 width, height;
	uint16 length;
	uint32 stamp;
} CELLHEADER;

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
