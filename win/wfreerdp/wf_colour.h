
#ifndef __WF_COLOUR_H
#define __WF_COLOUR_H

#include "freerdp.h"

int
wf_colour_convert(wfInfo * wfi, rdpSet * settings, int colour);
uint8 *
wf_image_convert(wfInfo * wfi, rdpSet * settings, int width, int height,
	uint8 * in_data);
RD_HCOLOURMAP
wf_create_colourmap(wfInfo * wfi, rdpSet * settings, RD_COLOURMAP * colours);
int
wf_set_colourmap(wfInfo * wfi, rdpSet * settings, RD_HCOLOURMAP map);
uint8 *
wf_glyph_convert(wfInfo * wfi, int width, int height, uint8 * data);
uint8 *
wf_glyph_generate(wfInfo * wfi, int width, int height, uint8 * glyph);

#endif
