
#ifndef __XF_COLOUR_H
#define __XF_COLOUR_H

#include <freerdp/freerdp.h>

int
xf_colour_convert(xfInfo * xfi, rdpSet * settings, int colour);
uint8 *
xf_image_convert(xfInfo * xfi, rdpSet * settings, int width, int height,
	uint8 * in_data);
RD_HCOLOURMAP
xf_create_colourmap(xfInfo * xfi, rdpSet * settings, RD_COLOURMAP * colours);
int
xf_set_colourmap(xfInfo * xfi, rdpSet * settings, RD_HCOLOURMAP map);
int
xf_cursor_convert_mono(xfInfo * xfi, uint8 * src_glyph, uint8 * msk_glyph,
	uint8 * xormask, uint8 * andmask, int width, int height, int bpp);
int
xf_cursor_convert_alpha(xfInfo * xfi, uint8 * alpha_data,
	uint8 * xormask, uint8 * andmask, int width, int height, int bpp);

#endif
