/*
   FreeRDP: A Remote Desktop Protocol client.
   RDP Capabilities

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

#include <freerdp/rdpset.h>
#include "frdp.h"
#include "rdp.h"
#include "pstcache.h"
#include "capabilities.h"
#include "stream.h"
#include "mem.h"

typedef uint8 * capsetHeaderRef;

/* Leave room in s for capability set header and return address for back patching */
static capsetHeaderRef
rdp_skip_capset_header(STREAM s)
{
	capsetHeaderRef rv;
	rv = s->p;
	s->p += 4;
	return rv;
}

/* Backpatch capability set header at the reference */
static void
rdp_out_capset_header(STREAM s, capsetHeaderRef header, uint16 capabilitySetType)
{
	struct stream tmp_s;

	ASSERT(header >= s->data);
	ASSERT(header < s->p);

	tmp_s.p = tmp_s.data = header;
	tmp_s.size = 4;
	tmp_s.end = tmp_s.data + tmp_s.size;
	out_uint16_le(&tmp_s, capabilitySetType); /* capabilitySetType */
	out_uint16_le(&tmp_s, s->p - header); /* lengthCapability */
	ASSERT(s_check_end(&tmp_s));
}

/* Output general capability set */
void
rdp_out_general_capset(rdpRdp * rdp, STREAM s)
{
	int flags;
	capsetHeaderRef header;

	header = rdp_skip_capset_header(s);
	out_uint16_le(s, OS_MAJOR_TYPE_WINDOWS); // osMajorType, should we lie?
	out_uint16_le(s, OS_MINOR_TYPE_WINDOWS_NT); // osMinorType
	out_uint16_le(s, CAPS_PROTOCOL_VERSION); // protocolVersion
	out_uint16_le(s, 0); // pad
	out_uint16_le(s, 0); // generalCompressionTypes, must be set to 0
	flags = 0;
	if (rdp->settings->rdp_version >= 5)
	{
		flags = FASTPATH_OUTPUT_SUPPORTED | NO_BITMAP_COMPRESSION_HDR |
			LONG_CREDENTIALS_SUPPORTED | AUTORECONNECT_SUPPORTED;
	}
	out_uint16_le(s, flags); // extraFlags
	out_uint16_le(s, 0); // updateCapabilityFlag, must be set to 0
	out_uint16_le(s, 0); // remoteUnshareFlag, must be set to 0
	out_uint16_le(s, 0); // generalCompressionLevel, must be set to 0
	out_uint8(s, 0); // refreshRectSupport, either TRUE (0x01) or FALSE (0x00)
	out_uint8(s, 0); // suppressOutputSupport, either TRUE (0x01) or FALSE (0x00)
	rdp_out_capset_header(s, header, CAPSET_TYPE_GENERAL);
}

/* Output bitmap capability set */
void
rdp_out_bitmap_capset(rdpRdp * rdp, STREAM s)
{
	capsetHeaderRef header;

	header = rdp_skip_capset_header(s);

	/*
	 * preferredBitsPerPixel (2 bytes):
	 * A 16-bit, unsigned integer. Color depth of the remote session. In RDP 4.0 and 5.0,
	 * this field MUST be set to 8 (even for a 16-color session)
	 */

	if(rdp->settings->rdp_version <= 5)
	{
		out_uint16_le(s, 8); // preferredBitsPerPixel
	}
	else
	{
		out_uint16_le(s, rdp->settings->server_depth); // preferredBitsPerPixel
	}

	out_uint16_le(s, 1); // receive1BitPerPixel
	out_uint16_le(s, 1); // receive4BitsPerPixel
	out_uint16_le(s, 1); // receive8BitsPerPixel
	out_uint16_le(s, rdp->settings->width); // desktopWidth
	out_uint16_le(s, rdp->settings->height); // desktopHeight
	out_uint16_le(s, 0); // pad
	out_uint16_le(s, 1); // desktopResizeFlag
	out_uint16_le(s, rdp->settings->bitmap_compression ? 1 : 0); // bitmapCompressionFlag
	out_uint8(s, 0); // highColorFlags, ignored and should be set to zero
	out_uint8(s, 1); // drawingFlags, indicating support for 32 bpp bitmaps
	out_uint16_le(s, 1); // multipleRectangleSupport, must be set to true
	out_uint16_le(s, 0); // pad
	rdp_out_capset_header(s, header, CAPSET_TYPE_BITMAP);
}

/* Process bitmap capability set */
void
rdp_process_bitmap_capset(rdpRdp * rdp, STREAM s)
{
	uint16 preferredBitsPerPixel;
	uint16 desktopWidth;
	uint16 desktopHeight;
	uint16 desktopResizeFlag;
	uint16 bitmapCompressionFlag;
	uint8 drawingFlags;

	/*
	 * preferredBitsPerPixel (2 bytes):
	 * A 16-bit, unsigned integer. Color depth of the remote session. In RDP 4.0 and 5.0,
	 * this field MUST be set to 8 (even for a 16-color session)
	 */

	in_uint16_le(s, preferredBitsPerPixel); // preferredBitsPerPixel
	in_uint8s(s, 6); // Ignore receive1BitPerPixel, receive4BitPerPixel, receive8BitPerPixel
	in_uint16_le(s, desktopWidth); // desktopWidth
	in_uint16_le(s, desktopHeight); // desktopHeight
	in_uint8s(s, 2); // Ignore pad
	in_uint16_le(s, desktopResizeFlag); // desktopResizeFlag
	in_uint16_le(s, bitmapCompressionFlag); // bitmapCompressionFlag
	in_uint8s(s, 1); // Ignore highColorFlags
	in_uint8(s, drawingFlags); // drawingFlags

	/*
	 * The server may limit depth and change the size of the desktop (for
	 * example when shadowing another session).
	 */
	if (rdp->settings->server_depth != preferredBitsPerPixel)
	{
		ui_warning(rdp->inst, "Remote desktop does not support color depth %d; falling back to %d\n",
			rdp->settings->server_depth, preferredBitsPerPixel);
		rdp->settings->server_depth = preferredBitsPerPixel;
	}
	if (rdp->settings->width != desktopWidth || rdp->settings->height != desktopHeight)
	{
		ui_warning(rdp->inst, "Remote desktop changed from %dx%d to %dx%d.\n", rdp->settings->width,
			rdp->settings->height, desktopWidth, desktopHeight);
		rdp->settings->width = desktopWidth;
		rdp->settings->height = desktopHeight;
		ui_resize_window(rdp->inst);
	}
}

/* Output order capability set */
void
rdp_out_order_capset(rdpRdp * rdp, STREAM s)
{
	uint8 orderSupport[32];
	capsetHeaderRef header;

	header = rdp_skip_capset_header(s);

	memset(orderSupport, 0, 32);
	orderSupport[NEG_DSTBLT_INDEX] = 1;
	orderSupport[NEG_PATBLT_INDEX] = 1;
	orderSupport[NEG_SCRBLT_INDEX] = 1;
	orderSupport[NEG_MEMBLT_INDEX] = (rdp->settings->bitmap_cache ? 1 : 0);
	orderSupport[NEG_MEM3BLT_INDEX] = (rdp->settings->triblt ? 1 : 0);
	// orderSupport[NEG_DRAWNINEGRID_INDEX] = 1;
	orderSupport[NEG_LINETO_INDEX] = 1;
	orderSupport[NEG_MULTI_DRAWNINEGRID_INDEX] = 1;
	orderSupport[NEG_SAVEBITMAP_INDEX] = (rdp->settings->desktop_save ? 1 : 0);
	// orderSupport[NEG_MULTIDSTBLT_INDEX] = 1;
	orderSupport[NEG_MULTIPATBLT_INDEX] = 1;
	// orderSupport[NEG_MULTISCRBLT_INDEX] = 1;
	// orderSupport[NEG_MULTIOPAQUERECT_INDEX] = 1;
	orderSupport[NEG_FAST_INDEX_INDEX] = 1;
	orderSupport[NEG_POLYGON_SC_INDEX] = (rdp->settings->polygon_ellipse_orders ? 1 : 0);
	orderSupport[NEG_POLYGON_CB_INDEX] = (rdp->settings->polygon_ellipse_orders ? 1 : 0);
	orderSupport[NEG_POLYLINE_INDEX] = 1;
	orderSupport[NEG_FAST_GLYPH_INDEX] = 1;
	orderSupport[NEG_ELLIPSE_SC_INDEX] = (rdp->settings->polygon_ellipse_orders ? 1 : 0);
	orderSupport[NEG_ELLIPSE_CB_INDEX] = (rdp->settings->polygon_ellipse_orders ? 1 : 0);
	orderSupport[NEG_INDEX_INDEX] = 1;

	out_uint8s(s, 16); // terminalDescriptor, ignored and should all be set to zeros
	out_uint32_le(s, 0); // pad
	out_uint16_le(s, 1); // desktopSaveXGranularity
	out_uint16_le(s, 20); // desktopSaveYGranularity
	out_uint16_le(s, 0); // pad
	out_uint16_le(s, 1); // maximumOrderLevel
	out_uint16_le(s, 0); // numberFonts, ignored and should be set to zero

	out_uint16_le(s,
		NEGOTIATEORDERSUPPORT |
		ZEROBOUNDSDELTASSUPPORT |
		COLORINDEXSUPPORT ); // orderFlags

	out_uint8p(s, orderSupport, 32); // orderSupport
	out_uint16_le(s, 0); // textFlags, must be ignored
	out_uint16_le(s, 0); // orderSupportExFlags
	out_uint32_le(s, 0); // pad
	out_uint32_le(s, rdp->settings->desktop_save == False ? 0 : 0x38400); // desktopSaveSize
	out_uint16_le(s, 0); // pad
	out_uint16_le(s, 0); // pad

	/* See [MSDN-CP]: http://msdn.microsoft.com/en-us/library/dd317756(VS.85).aspx */
	out_uint16_le(s, 0x04E4); // textANSICodePage, 0x04E4 is "ANSI Latin 1 Western European (Windows)"
	out_uint16_le(s, 0); // pad

	rdp_out_capset_header(s, header, CAPSET_TYPE_ORDER);
}

/* Process order capability set */
void
rdp_process_order_capset(rdpRdp * rdp, STREAM s)
{
	uint8 orderSupport[32];
	uint16 desktopSaveXGranularity;
	uint16 desktopSaveYGranularity;
	uint16 maximumOrderLevel;
	uint16 orderFlags;
	uint16 orderSupportExFlags;
	uint32 desktopSaveSize;
	uint16 textANSICodePage;
	memset(orderSupport, 0, 32);

	in_uint8s(s, 20); // Ignore terminalDescriptor and pad
	in_uint16_le(s, desktopSaveXGranularity); // desktopSaveXGranularity
	in_uint16_le(s, desktopSaveYGranularity); // desktopSaveYGranularity
	in_uint8s(s, 2); // Ignore pad
	in_uint16_le(s, maximumOrderLevel); // maximumOrderLevel
	in_uint8s(s, 2); // Ignore numberFonts
	in_uint16_le(s, orderFlags); // orderFlags
	in_uint8a(s, orderSupport, 32); // orderSupport
	in_uint8s(s, 2); // Ignore textFlags
	in_uint16_le(s, orderSupportExFlags); // orderSupportExFlags
	in_uint8s(s, 4); // Ignore pad
	in_uint32_le(s, desktopSaveSize); // desktopSaveSize
	in_uint8s(s, 4); // Ignore pad

	/* See [MSDN-CP]: http://msdn.microsoft.com/en-us/library/dd317756(VS.85).aspx */
	in_uint16_le(s, textANSICodePage); // textANSICodePage
	// Ignore pad
}

/* Output bitmap cache capability set */
void
rdp_out_bitmapcache_capset(rdpRdp * rdp, STREAM s)
{
	int Bpp;
	int size;
	capsetHeaderRef header;

	header = rdp_skip_capset_header(s);
	Bpp = (rdp->settings->server_depth + 7) / 8;	/* bytes per pixel */
	out_uint8s(s, 24); // pad
	out_uint16_le(s, 0x258); // Cache1Entries
	size = 0x100 * Bpp;
	out_uint16_le(s, size); // Cache1MaximumCellSize
	out_uint16_le(s, 0x12c); // Cache2Entries
	size = 0x400 * Bpp;
	out_uint16_le(s, size); // Cache2MaximumCellSize
	out_uint16_le(s, 0x106); // Cache3Entries
	size = 0x1000 * Bpp;
	out_uint16_le(s, size); //Cache3MaximumCellSize
	rdp_out_capset_header(s, header, CAPSET_TYPE_BITMAPCACHE);
}

/* Output bitmap cache v2 capability set */
void
rdp_out_bitmapcache_rev2_capset(rdpRdp * rdp, STREAM s)
{
	capsetHeaderRef header;

	header = rdp_skip_capset_header(s);
	out_uint16_le(s, rdp->settings->bitmap_cache_persist_enable ? 3 : 2); // CacheFlags
	out_uint8s(s, 1); // pad
	out_uint8(s, 3); // numCellCaches

	/* max cell size for cache 0 is 16x16, 1 = 32x32, 2 = 64x64, etc */
	out_uint32_le(s, BMPCACHE2_C0_CELLS);
	out_uint32_le(s, BMPCACHE2_C1_CELLS);
	if (pstcache_init(rdp->pcache, 2))
	{
		out_uint32_le(s, BMPCACHE2_NUM_PSTCELLS | BMPCACHE2_FLAG_PERSIST);
	}
	else
	{
		out_uint32_le(s, BMPCACHE2_C2_CELLS);
	}
	out_uint8s(s, 20);	/* other bitmap caches not used */
	rdp_out_capset_header(s, header, CAPSET_TYPE_BITMAPCACHE_REV2);
}

/* Output bitmap cache host support capability set */
void
rdp_out_bitmapcache_hostsupport_capset(rdpRdp * rdp, STREAM s)
{
	capsetHeaderRef header;

	header = rdp_skip_capset_header(s);
	out_uint8(s, BITMAPCACHE_REV2); // cacheVersion, must be set to BITMAPCACHE_REV2
	out_uint8(s, 0); // pad
	out_uint16_le(s, 0); // pad
	rdp_out_capset_header(s, header, CAPSET_TYPE_BITMAPCACHE_HOSTSUPPORT);
}

/* Process bitmap cache host support capability set */
void
rdp_process_bitmapcache_hostsupport_capset(rdpRdp * rdp, STREAM s)
{
	uint8 cacheVersion;

	in_uint8(s, cacheVersion); // cacheVersion, must be set to BITMAPCACHE_REV2
	// pad (1 byte)
	// pad (2 bytes)
}

/* Output input capability set */
void
rdp_out_input_capset(rdpRdp * rdp, STREAM s)
{
	capsetHeaderRef header;
	uint32 flags;

	header = rdp_skip_capset_header(s);
	flags = INPUT_FLAG_SCANCODES | INPUT_FLAG_MOUSEX | INPUT_FLAG_UNICODE;
	if (rdp->use_input_fast_path)
	{
		flags |= rdp->input_flags & (INPUT_FLAG_FASTPATH_INPUT | INPUT_FLAG_FASTPATH_INPUT2);
	}
	out_uint16_le(s, flags); // inputFlags
	out_uint16_le(s, 0); // pad
	out_uint32_le(s, rdp->settings->keyboard_layout); // keyboardLayout
	out_uint32_le(s, rdp->settings->keyboard_type); // keyboardType
	out_uint32_le(s, rdp->settings->keyboard_subtype); // keyboardSubType
	out_uint32_le(s, rdp->settings->keyboard_functionkeys); // keyboardFunctionKeys

	//rdp_out_unistr(rdp, s, rdp->settings->keyboard_ime, 2 * strlen(rdp->settings->keyboard_ime));
	//out_uint8s(s, 62 - 2 * strlen(rdp->settings->keyboard_ime)); // imeFileName (64 bytes)
	out_uint8s(s, 64);
	rdp_out_capset_header(s, header, CAPSET_TYPE_INPUT);
}

/* Process input capability set */
void
rdp_process_input_capset(rdpRdp * rdp, STREAM s)
{
	uint32 keyboardLayout;
	uint32 keyboardType;
	uint32 keyboardSubType;
	uint32 keyboardFunctionKeys;

	in_uint16_le(s, rdp->input_flags); // inputFlags
	if ((rdp->input_flags & INPUT_FLAG_FASTPATH_INPUT) ||
		(rdp->input_flags & INPUT_FLAG_FASTPATH_INPUT2))
	{
		rdp->use_input_fast_path = 1;
	}
	in_uint8s(s, 2); // pad
	in_uint32_le(s, keyboardLayout); // keyboardLayout
	in_uint32_le(s, keyboardType); // keyboardType
	in_uint32_le(s, keyboardSubType); // keyboardSubType
	in_uint32_le(s, keyboardFunctionKeys); // keyboardFunctionKeys
	// in_uint8s(s, 64); // imeFileName (64 bytes)
}

/* Output font capability set */
void
rdp_out_font_capset(STREAM s)
{
	capsetHeaderRef header;

	header = rdp_skip_capset_header(s);
	out_uint16_le(s, FONTSUPPORT_FONTLIST); // fontSupportFlags
	out_uint16_le(s, 0); // pad
	rdp_out_capset_header(s, header, CAPSET_TYPE_FONT);
}

/* Process font capability set */
void
rdp_process_font_capset(rdpRdp * rdp, STREAM s)
{
	uint16 fontSupportFlags;

	in_uint16_le(s, fontSupportFlags); // fontSupportFlags
	// pad (2 bytes)
}

/* Output control capability set */
void
rdp_out_control_capset(STREAM s)
{
	capsetHeaderRef header;

	header = rdp_skip_capset_header(s);
	out_uint16_le(s, 0); // controlFlags, should be set to zero
	out_uint16_le(s, 0); // remoteDetachFlag, should be set to FALSE
	out_uint16_le(s, CONTROLPRIORITY_NEVER); // controlInterest
	out_uint16_le(s, CONTROLPRIORITY_NEVER); // detachInterest
	rdp_out_capset_header(s, header, CAPSET_TYPE_CONTROL);
}

/* Output window activation capability set */
void
rdp_out_window_activation_capset(STREAM s)
{
	capsetHeaderRef header;

	header = rdp_skip_capset_header(s);
	/* All of the following should be set to FALSE (0x0) */
	out_uint16_le(s, 0); // helpKeyFlag
	out_uint16_le(s, 0); // helpKeyIndexFlag
	out_uint16_le(s, 0); // helpExtendedKeyFlag
	out_uint16_le(s, 0); // windowManagerKeyFlag
	rdp_out_capset_header(s, header, CAPSET_TYPE_ACTIVATION);
}

/* Output pointer capability set */
void
rdp_out_pointer_capset(rdpRdp * rdp, STREAM s)
{
	capsetHeaderRef header;

	header = rdp_skip_capset_header(s);
	out_uint16_le(s, 1); /* colorPointerFlag (assumed to be always true) */
	out_uint16_le(s, 20); /* colorPointerCacheSize */
	if (rdp->settings->new_cursors)
	{
		/*
		* pointerCacheSize (2 bytes):
		* Optional, if absent or set to 0 the server
		* will not use the New Pointer Update
		*/
		out_uint16_le(s, 20); /* pointerCacheSize */
	}
	rdp_out_capset_header(s, header, CAPSET_TYPE_POINTER);
}

/* Process pointer capability set */
void
rdp_process_pointer_capset(rdpRdp * rdp, STREAM s)
{
	uint16 colorPointerFlags;
	uint16 colorPointerCacheSize;
	// uint16 pointerCacheSize;

	in_uint16_le(s, colorPointerFlags); // colorPointerFlags (assumed to be always true)
	in_uint16_le(s, colorPointerCacheSize); // colorPointerCacheSize
	// int_uint16_le(s, pointerCacheSize); // pointerCacheSize
}

/* Output share capability set */
void
rdp_out_share_capset(STREAM s)
{
	capsetHeaderRef header;

	header = rdp_skip_capset_header(s);
	out_uint16_le(s, 0); // nodeID
	out_uint16_le(s, 0); // pad
	rdp_out_capset_header(s, header, CAPSET_TYPE_SHARE);
}

/* Process share capability set */
void
rdp_process_share_capset(rdpRdp * rdp, STREAM s)
{
	uint16 nodeID;

	in_uint16_le(s, nodeID); // nodeID
	// pad
}

/* Output color cache capability set */
void
rdp_out_colorcache_capset(STREAM s)
{
	capsetHeaderRef header;

	header = rdp_skip_capset_header(s);
	out_uint16_le(s, 6);	/* cache size */
	out_uint16_le(s, 0);	/* pad */
	rdp_out_capset_header(s, header, CAPSET_TYPE_COLORCACHE);
}

/* Process color cache capability set */
void
rdp_process_colorcache_capset(rdpRdp * rdp, STREAM s)
{
	uint16 cacheSize;

	in_uint16_le(s, cacheSize); // cacheSize
	// pad (2 bytes)
}

/* Output brush capability set */
void
rdp_out_brush_capset(STREAM s)
{
	capsetHeaderRef header;

	header = rdp_skip_capset_header(s);
	out_uint32_le(s, BRUSH_COLOR_FULL); // brushSupportLevel
	rdp_out_capset_header(s, header, CAPSET_TYPE_BRUSH);
}

/* Output glyph cache definition */
void
rdp_out_cache_definition(STREAM s, uint16 cacheEntries, uint16 cacheMaximumCellSize)
{
	out_uint16_le(s, cacheEntries); // cacheEntries
	out_uint16_le(s, cacheMaximumCellSize); // cacheMaximumCellSize
}

/* Output glyph cache capability set */
void
rdp_out_glyphcache_capset(STREAM s)
{
	capsetHeaderRef header;

	header = rdp_skip_capset_header(s);
	/*
		glyphCache (40 bytes):
		An array of 10 cache definition structures
		Maximum number of cache entries: 254
		Maximum size of a cache element: 2048
	 */
	rdp_out_cache_definition(s, 0x00FE, 0x0004);
	rdp_out_cache_definition(s, 0x00FE, 0x0004);
	rdp_out_cache_definition(s, 0x00FE, 0x0008);
	rdp_out_cache_definition(s, 0x00FE, 0x0008);
	rdp_out_cache_definition(s, 0x00FE, 0x0010);
	rdp_out_cache_definition(s, 0x00FE, 0x0020);
	rdp_out_cache_definition(s, 0x00FE, 0x0040);
	rdp_out_cache_definition(s, 0x00FE, 0x0080);
	rdp_out_cache_definition(s, 0x00FE, 0x0100);
	rdp_out_cache_definition(s, 0x0040, 0x0800);

	/*
		fragCache (4 bytes):
		Fragment cache data (one cache definition structure)
		Maximum number of cache entries: 256
		Maximum size of a cache element: 256
	*/
	rdp_out_cache_definition(s, 0x0040, 0x0800); // fragCache

	out_uint16_le(s, GLYPH_SUPPORT_FULL); // glyphSupportLevel
	out_uint16_le(s, 0); // pad
	rdp_out_capset_header(s, header, CAPSET_TYPE_GLYPHCACHE);
}

/* Output sound capability set */
void
rdp_out_sound_capset(STREAM s)
{
	capsetHeaderRef header;

	header = rdp_skip_capset_header(s);
	out_uint16_le(s, SOUND_BEEPS_FLAG); // soundFlags
	out_uint16_le(s, 0); // pad
	rdp_out_capset_header(s, header, CAPSET_TYPE_SOUND);
}

/* Output offscreen cache capability set */
void
rdp_out_offscreenscache_capset(STREAM s)
{
	capsetHeaderRef header;

	header = rdp_skip_capset_header(s);
	out_uint32_le(s, 1); // offscreenSupportLevel, either TRUE (0x1) or FALSE (0x0)
	out_uint16_le(s, 7680); // offscreenCacheSize, maximum is 7680 (in KB)
	out_uint16_le(s, 100); // offscreenCacheEntries, maximum is 500 entries
	rdp_out_capset_header(s, header, CAPSET_TYPE_OFFSCREENCACHE);
}

/* Output virtual channel capability set */
void
rdp_out_virtualchannel_capset(STREAM s)
{
	capsetHeaderRef header;

	header = rdp_skip_capset_header(s);
	out_uint32_le(s, VCCAPS_COMPR_SC); // virtual channel compression flags
	out_uint32_le(s, 0); // VCChunkSize, ignored when sent from client to server
	rdp_out_capset_header(s, header, CAPSET_TYPE_VIRTUALCHANNEL);
}

/* Output virtual channel capability set */
void
rdp_process_virtualchannel_capset(rdpRdp * rdp, STREAM s)
{
	uint32 virtualChannelCompressionFlags;
	uint32 virtualChannelChunkSize;
	in_uint32_le(s, virtualChannelCompressionFlags); // virtual channel compression flags
	in_uint32_le(s, virtualChannelChunkSize); // VCChunkSize
}

/* Output draw ninegrid cache capability set */
void
rdp_out_drawninegridcache_capset(STREAM s)
{
	capsetHeaderRef header;

	header = rdp_skip_capset_header(s);
	out_uint32_le(s, DRAW_NINEGRID_NO_SUPPORT); // drawNineGridSupportLevel
	out_uint16_le(s, 2560); // drawNineGridCacheSize, maximum is 2560 (in KB)
	out_uint16_le(s, 256); // drawNineGridCacheEntries, maximum is 256
	rdp_out_capset_header(s, header, CAPSET_TYPE_DRAWNINEGRIDCACHE);
}

/* Output GDI+ cache entries structure */
void
rdp_out_gdiplus_cache_entries(STREAM s)
{
	out_uint16_le(s, 10); // gdipGraphicsCacheEntries
	out_uint16_le(s, 5); // gdipBrushCacheEntries
	out_uint16_le(s, 5); // gdipPenCacheEntries
	out_uint16_le(s, 10); // gdipImageCacheEntries
	out_uint16_le(s, 2); // gdipImageAttributesCacheEntries
}

/* Output GDI+ cache chunk size structure */
void
rdp_out_gdiplus_cache_chunk_size(STREAM s)
{
	out_uint16_le(s, 512); // gdipGraphicsCacheChunkSize
	out_uint16_le(s, 2048); // gdipObjectBrushCacheChunkSize
	out_uint16_le(s, 1024); // gdipObjectPenCacheChunkSize
	out_uint16_le(s, 64); // gdipObjectImageAttributesCacheChunkSize
}

/* Output GDI+ image cache properties structure */
void
rdp_out_gdiplus_image_cache_properties(STREAM s)
{
	out_uint16_le(s, 4096); // gdipObjectImageCacheChunkSize
	out_uint16_le(s, 256); // gdipObjectImageCacheTotalSize
	out_uint16_le(s, 128); // gdipObjectImageCacheMaxSize
}

/* Output draw GDI+ capability set */
void
rdp_out_draw_gdiplus_capset(STREAM s)
{
	capsetHeaderRef header;

	header = rdp_skip_capset_header(s);
	out_uint32_le(s, DRAW_GDIPLUS_DEFAULT); // drawGDIPlusSupportLevel
	out_uint32_le(s, 0); // gdipVersion, build number for the GDI+ 1.1 subsystem
	out_uint32_le(s, DRAW_GDIPLUS_CACHE_LEVEL_DEFAULT); // drawGdiplusCacheLevel
	rdp_out_gdiplus_cache_entries(s); // gdipCacheEntries
	rdp_out_gdiplus_cache_chunk_size(s); // gdipCacheChunkSize
	rdp_out_gdiplus_image_cache_properties(s); // gdipImageCacheProperties
	rdp_out_capset_header(s, header, CAPSET_TYPE_DRAWGDIPLUS);
}

/* Process GDI+ cache entries structure */
void
rdp_process_gdiplus_cache_entries(rdpRdp * rdp, STREAM s)
{
	uint16 gdipGraphicsCacheEntries;
	uint16 gdipBrushCacheEntries;
	uint16 gdipPenCacheEntries;
	uint16 gdipImageCacheEntries;
	uint16 gdipImageAttributesCacheEntries;

	in_uint16_le(s, gdipGraphicsCacheEntries); // gdipGraphicsCacheEntries
	in_uint16_le(s, gdipBrushCacheEntries); // gdipBrushCacheEntries
	in_uint16_le(s, gdipPenCacheEntries); // gdipPenCacheEntries
	in_uint16_le(s, gdipImageCacheEntries); // gdipImageCacheEntries
	in_uint16_le(s, gdipImageAttributesCacheEntries); // gdipImageAttributesCacheEntries
}

/* Process GDI+ cache chunk size structure */
void
rdp_process_gdiplus_cache_chunk_size(rdpRdp * rdp, STREAM s)
{
	uint16 gdipGraphicsCacheChunkSize;
	uint16 gdipObjectBrushCacheChunkSize;
	uint16 gdipObjectPenCacheChunkSize;
	uint16 gdipObjectImageAttributesCacheChunkSize;

	in_uint16_le(s, gdipGraphicsCacheChunkSize); // gdipGraphicsCacheChunkSize
	in_uint16_le(s, gdipObjectBrushCacheChunkSize); // gdipObjectBrushCacheChunkSize
	in_uint16_le(s, gdipObjectPenCacheChunkSize); // gdipObjectPenCacheChunkSize
	in_uint16_le(s, gdipObjectImageAttributesCacheChunkSize); // gdipObjectImageAttributesCacheChunkSize
}

/* Process GDI+ image cache properties structure */
void
rdp_process_gdiplus_image_cache_properties(rdpRdp * rdp, STREAM s)
{
	uint16 gdipObjectImageCacheChunkSize;
	uint16 gdipObjectImageCacheTotalSize;
	uint16 gdipObjectImageCacheMaxSize;

	in_uint16_le(s, gdipObjectImageCacheChunkSize); // gdipObjectImageCacheChunkSize
	in_uint16_le(s, gdipObjectImageCacheTotalSize); // gdipObjectImageCacheTotalSize
	in_uint16_le(s, gdipObjectImageCacheMaxSize); // gdipObjectImageCacheMaxSize
}


/* Process draw GDI+ capability set */
void
rdp_process_draw_gdiplus_capset(rdpRdp * rdp, STREAM s)
{
	uint32 drawGDIPlusSupportLevel;
	uint32 gdipVersion;
	uint32 drawGdiplusCacheLevel;

	in_uint32_le(s, drawGDIPlusSupportLevel); // drawGDIPlusSupportLevel
	in_uint32_le(s, gdipVersion); // gdipVersion, build number for the GDI+ 1.1 subsystem
	in_uint32_le(s, drawGdiplusCacheLevel); // drawGdiplusCacheLevel

	rdp_process_gdiplus_cache_entries(rdp, s); // gdipCacheEntries
	rdp_process_gdiplus_cache_chunk_size(rdp, s); // gdipCacheChunkSize
	rdp_process_gdiplus_image_cache_properties(rdp, s); // gdipImageCacheProperties
}

/* Output RAIL capability set */
void
rdp_out_rail_capset(STREAM s)
{
	capsetHeaderRef header;

	header = rdp_skip_capset_header(s);
	out_uint32_le(s, RAIL_LEVEL_SUPPORTED); // RailSupportLevel
	rdp_out_capset_header(s, header, CAPSET_TYPE_RAIL);
}

/* Process RAIL capability set */
void
rdp_process_rail_capset(rdpRdp * rdp, STREAM s)
{
	uint32 railSupportLevel;
	in_uint32_le(s, railSupportLevel); // railSupportLevel
}

/* Output window list capability set */
void
rdp_out_window_capset(STREAM s)
{
	capsetHeaderRef header;

	header = rdp_skip_capset_header(s);
	out_uint32_le(s, WINDOW_LEVEL_SUPPORTED); // wndSupportLevel
	out_uint8(s, 3); // numIconCaches
	out_uint16_le(s, 12); // numIconCacheEntries
	rdp_out_capset_header(s, header, CAPSET_TYPE_WINDOW);
}

/* Process window list capability set */
void
rdp_process_window_capset(rdpRdp * rdp, STREAM s)
{
	uint32 wndSupportLevel;
	uint8 numIconCaches;
	uint16 numIconCacheEntries;

	in_uint32_le(s, wndSupportLevel); // wndSupportLevel
	in_uint8(s, numIconCaches); // numIconCaches
	in_uint16_le(s, numIconCacheEntries); // numIconCacheEntries
}

/* Output large pointer capability set */
void
rdp_out_large_pointer_capset(rdpRdp * rdp, STREAM s)
{
	capsetHeaderRef header;

	header = rdp_skip_capset_header(s);
	out_uint16_le(s, rdp->large_pointers); // largePointerSupportFlags
	rdp_out_capset_header(s, header, CAPSET_TYPE_LARGE_POINTER);
}

/* Process large pointer capability set */
void
rdp_process_large_pointer_capset(rdpRdp * rdp, STREAM s)
{
	rdp->got_large_pointer_caps = 1;
	in_uint16_le(s, rdp->large_pointers); // largePointerSupportFlags
}

/* Process surface commands capability set */
void
rdp_process_surface_commands_capset(rdpRdp * rdp, STREAM s)
{
	uint32 cmdFlags;
	in_uint32_le(s, cmdFlags); // cmdFlags
	/* Reserved (4 bytes) */
}

/* Output desktop composition capability set */
void
rdp_out_compdesk_capset(STREAM s)
{
	capsetHeaderRef header;

	header = rdp_skip_capset_header(s);
	out_uint16_le(s, COMPDESK_NOT_SUPPORTED); // CompDeskSupportLevel
	rdp_out_capset_header(s, header, CAPSET_TYPE_COMPDESK);
}

/* Process desktop composition capability set */
void
rdp_process_compdesk_capset(rdpRdp * rdp, STREAM s)
{
	uint16 compDeskSupportLevel;
	in_uint16_le(s, compDeskSupportLevel); // compDeskSupportLevel
}

/* Output multifragment update capability set */
void
rdp_out_multifragmentupdate_capset(rdpRdp * rdp, STREAM s)
{
	capsetHeaderRef header;

	header = rdp_skip_capset_header(s);
	/*
	* MaxRequestsize (4 bytes):
	* The size of the buffer used to reassemble the fragments of a
	* fast-path update. The size of this buffer places a cap on the
	* size of the largest fast-path update that can be fragmented.
	*/
	out_uint32_le(s, rdp->multifragmentupdate_request_size);
	rdp_out_capset_header(s, header, CAPSET_TYPE_MULTIFRAGMENTUPDATE);
}

/* Process multifragment update capability set */
void
rdp_process_multifragmentupdate_capset(rdpRdp * rdp, STREAM s)
{
	rdp->got_multifragmentupdate_caps = 1;
	in_uint32_le(s, rdp->multifragmentupdate_request_size);
	rdp->fragment_data = stream_new(rdp->multifragmentupdate_request_size);
}

/* Output surface commands capability set */
void
rdp_out_surface_commands_capset(STREAM s)
{
	capsetHeaderRef header;

	header = rdp_skip_capset_header(s);
	out_uint32_le(s, SURFCMDS_SETSURFACEBITS); // cmdFlags
	out_uint32_le(s, 0); // reserved for future use
	rdp_out_capset_header(s, header, CAPSET_TYPE_SURFACE_COMMANDS);
}

/* Output a bitmap codec structure */
void
rdp_out_bitmap_codec(STREAM s)
{
	// codecGUID (16 bytes)
	out_uint32_le(s, 0); // codecGUID1
	out_uint16_le(s, 0); // codecGUID2
	out_uint16_le(s, 0); // codecGUID3
	out_uint8(s, 0); // codecGUID4
	out_uint8(s, 0); // codecGUID5
	out_uint8(s, 0); // codecGUID6
	out_uint8(s, 0); // codecGUID7
	out_uint8(s, 0); // codecGUID8
	out_uint8(s, 0); // codecGUID9
	out_uint8(s, 0); // codecGUID10
	out_uint8(s, 0); // codecGUID11

	out_uint8(s, 0); // codecID
	out_uint32_le(s, 0); // codecPropertiesLength
	//out_uint8s(s, 0); // codecProperties
}

/* Output bitmap codecs capability set */
void
rdp_out_bitmap_codecs_capset(STREAM s)
{
	capsetHeaderRef header;

	header = rdp_skip_capset_header(s);
	out_uint8(s, 0); // bitmapCodecCount, the number of bitmap codec entries
	// bitmapCodecArray
	// rdp_out_bitmap_codec(s, ...);
	rdp_out_capset_header(s, header, CAPSET_TYPE_BITMAP_CODECS);
}
