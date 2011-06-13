/*
   FreeRDP: A Remote Desktop Protocol client.
   RemoteFX Codec Library - API Header

   Copyright 2011 Vic Lee

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

#ifndef __RFX_H
#define __RFX_H

#include "types/base.h"

#include <freerdp/utils/profiler.h>

#ifdef __cplusplus
extern "C" {
#endif

/* sync */
#define WF_MAGIC		0xCACCACCA
#define WF_VERSION_1_0		0x0100

/* blockType */
#define WBT_SYNC		0xCCC0
#define WBT_CODEC_VERSIONS	0xCCC1
#define WBT_CHANNELS		0xCCC2
#define WBT_CONTEXT		0xCCC3
#define WBT_FRAME_BEGIN		0xCCC4
#define WBT_FRAME_END		0xCCC5
#define WBT_REGION		0xCCC6
#define WBT_EXTENSION		0xCCC7
#define CBT_TILESET		0xCAC2
#define CBT_TILE		0xCAC3

/* properties.flags */
#define CODEC_MODE		0x02

/* properties.cct */
#define COL_CONV_ICT		0x1

/* properties.xft */
#define CLW_XFORM_DWT_53_A	0x1

/* properties.et */
#define CLW_ENTROPY_RLGR1	0x01
#define CLW_ENTROPY_RLGR3	0x04

/* properties.qt */
#define SCALAR_QUANTIZATION	0x1

enum _RLGR_MODE
{
	RLGR1,
	RLGR3
};
typedef enum _RLGR_MODE RLGR_MODE;

enum _RFX_PIXEL_FORMAT
{
	RFX_PIXEL_FORMAT_BGRA,
	RFX_PIXEL_FORMAT_RGBA,
	RFX_PIXEL_FORMAT_BGR,
	RFX_PIXEL_FORMAT_RGB
};
typedef enum _RFX_PIXEL_FORMAT RFX_PIXEL_FORMAT;

struct _RFX_RECT
{
	uint16 x;
	uint16 y;
	uint16 width;
	uint16 height;
};
typedef struct _RFX_RECT RFX_RECT;

struct _RFX_TILE
{
	uint16 x;
	uint16 y;
	uint8 * data;
};
typedef struct _RFX_TILE RFX_TILE;

struct _RFX_POOL
{
	int size;
	int count;
	RFX_TILE **tiles;
};
typedef struct _RFX_POOL RFX_POOL;

struct _RFX_MESSAGE
{
	/*
	 * The rects array represents the updated region of the frame. The UI
	 * requires to clip drawing destination base on the union of the rects.
	 */
	uint16 num_rects;
	RFX_RECT * rects;

	/* The tiles array represents the actual frame data. Each tile is always
	 * 64x64. Note that only pixels inside the updated region (represented as
	 * rects described above) are valid. Pixels outside of the region may
	 * contain arbitrary data.
	 */
	uint16 num_tiles;
	RFX_TILE** tiles;
};
typedef struct _RFX_MESSAGE RFX_MESSAGE;

struct _RFX_CONTEXT
{
	int flags;
	uint16 width;
	uint16 height;
	RLGR_MODE mode;
	uint32 version;
	uint32 codec_id;
	uint32 codec_version;
	RFX_PIXEL_FORMAT pixel_format;

	/* temporary data within a frame */
	uint8 num_quants;
	uint32 * quants;

	/* pre-allocated buffers */

	RFX_POOL* pool; /* memory pool */

	sint16 y_r_mem[4096+8]; /* 4096 = 64x64 (+ 8x2 = 16 for mem align) */
	sint16 cb_g_mem[4096+8]; /* 4096 = 64x64 (+ 8x2 = 16 for mem align) */
	sint16 cr_b_mem[4096+8]; /* 4096 = 64x64 (+ 8x2 = 16 for mem align) */
 
 	sint16* y_r_buffer;
	sint16* cb_g_buffer;
	sint16* cr_b_buffer;
 
	sint16 dwt_buffer_8[256]; /* sub-band width 8 */
	sint16 dwt_buffer_16[1024]; /* sub-band width 16 */
	sint16 dwt_buffer_32[4096]; /* sub-band width 32 */
	sint16* dwt_buffers[5]; /* sub-band buffer array */

	/* routines */
	void (* decode_YCbCr_to_RGB)(sint16 * y_r_buf, sint16 * cb_g_buf, sint16 * cr_b_buf);
	void (* encode_RGB_to_YCbCr)(sint16 * y_r_buf, sint16 * cb_g_buf, sint16 * cr_b_buf);
	void (* quantization_decode)(sint16 * buffer, const uint32 * quantization_values);

	/* profiler definitions */
	PROFILER_DEFINE(prof_rfx_decode_rgb);
	PROFILER_DEFINE(prof_rfx_decode_component);
	PROFILER_DEFINE(prof_rfx_rlgr_decode);
	PROFILER_DEFINE(prof_rfx_differential_decode);
	PROFILER_DEFINE(prof_rfx_quantization_decode);
	PROFILER_DEFINE(prof_rfx_dwt_2d_decode);
	PROFILER_DEFINE(prof_rfx_decode_YCbCr_to_RGB);
};
typedef struct _RFX_CONTEXT RFX_CONTEXT;

RFX_CONTEXT* rfx_context_new(void);
void rfx_context_free(RFX_CONTEXT * context);
void rfx_context_set_pixel_format(RFX_CONTEXT * context, RFX_PIXEL_FORMAT pixel_format);

RFX_MESSAGE* rfx_process_message(RFX_CONTEXT * context, uint8 * data, int data_size);
void rfx_message_free(RFX_CONTEXT * context, RFX_MESSAGE * message);

int rfx_compose_message_header(RFX_CONTEXT * context, uint8 * buffer, int buffer_size);
int rfx_compose_message_data(RFX_CONTEXT * context, uint8 * buffer, int buffer_size,
	const RFX_RECT * rects, int num_rects, uint8 * image_buffer, int width, int height);

#ifdef __cplusplus
}
#endif

#endif /* __RFX_H */

