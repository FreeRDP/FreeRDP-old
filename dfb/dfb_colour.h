
#ifndef __DFB_COLOUR_H
#define __DFB_COLOUR_H

#include <freerdp/freerdp.h>

int
dfb_colour_convert(dfbInfo * dfbi, int in_colour, int in_bpp);
uint8 *
dfb_image_convert(dfbInfo * dfbi, rdpSet * settings, int width, int height,
	uint8 * in_data);
RD_HCOLOURMAP
dfb_create_colourmap(dfbInfo * dfbi, rdpSet * settings, RD_COLOURMAP * colours);
int
dfb_set_colourmap(dfbInfo * dfbi, rdpSet * settings, RD_HCOLOURMAP map);
int
dfb_cursor_convert_mono(dfbInfo * dfbi, uint8 * src_glyph, uint8 * msk_glyph,
	uint8 * xormask, uint8 * andmask, int width, int height, int bpp);
int
dfb_cursor_convert_alpha(dfbInfo * dfbi, uint8 * alpha_data,
	uint8 * xormask, uint8 * andmask, int width, int height, int bpp);

#endif /* __DFB_COLOUR_H */
