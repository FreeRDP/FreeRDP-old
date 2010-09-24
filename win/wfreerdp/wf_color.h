
#ifndef __WF_COLOR_H
#define __WF_COLOR_H

int
wf_color_convert(wfInfo * wfi, int in_color, int in_bpp);
uint8 *
wf_image_convert(wfInfo * wfi, int width, int height, int bpp,
	int reverse, uint8 * in_data, uint8 * out_data);
RD_HCOLORMAP
wf_create_colormap(wfInfo * wfi, RD_COLORMAP * colors);
int
wf_set_colormap(wfInfo * wfi, RD_HCOLORMAP map);
uint8 *
wf_glyph_convert(wfInfo * wfi, int width, int height, uint8 * data);
uint8 *
wf_cursor_mask_convert(wfInfo * wfi, int width, int height, uint8 * data);

#endif
