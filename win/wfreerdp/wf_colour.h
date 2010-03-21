
#ifndef __WF_COLOUR_H
#define __WF_COLOUR_H

int
wf_colour_convert(wfInfo * wfi, int in_colour, int in_bpp);
uint8 *
wf_image_convert(wfInfo * wfi, int width, int height, int bpp,
	int reverse, uint8 * in_data, uint8 * out_data);
RD_HCOLOURMAP
wf_create_colourmap(wfInfo * wfi, RD_COLOURMAP * colours);
int
wf_set_colourmap(wfInfo * wfi, RD_HCOLOURMAP map);
uint8 *
wf_glyph_convert(wfInfo * wfi, int width, int height, uint8 * data);
uint8 *
wf_cursor_mask_convert(wfInfo * wfi, int width, int height, uint8 * data);

#endif
