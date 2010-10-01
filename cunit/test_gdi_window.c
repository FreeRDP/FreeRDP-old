
#include <stdio.h>
#include <stdlib.h>
#include <freerdp/freerdp.h>
#include "libfreerdpgdi.h"
#include "gdi_window.h"
#include "test_gdi_window.h"

GDI *gdi;
rdpInst *inst;

int test_width = 1024;
int test_height = 768;
int test_depth = 32;

int init_gdi_window_suite(void)
{
	inst = (rdpInst*) malloc(sizeof(rdpInst));
	memset(inst, 0, sizeof(rdpInst));

	inst->settings = (rdpSet*) malloc(sizeof(rdpSet));
	memset(inst->settings, 0, sizeof(rdpSet));

	inst->settings->width = test_width;
	inst->settings->height = test_height;
	inst->settings->server_depth = test_depth;

	return 0;
}

int clean_gdi_window_suite(void)
{
	free(inst->settings);
	free(inst);

	return 0;
}

int add_gdi_window_suite(void)
{
	add_test_suite(gdi_window);

	add_test_function(gdi_init);
	add_test_function(gdi_clip_coords);
	add_test_function(gdi_invalidate_region);

	return 0;
}

void test_gdi_init(void)
{
	CU_ASSERT(gdi_init(inst) == 0);

	gdi = GET_GDI(inst);
	CU_ASSERT(gdi != NULL);

	CU_ASSERT(gdi->width == test_width);
	CU_ASSERT(gdi->height == test_height);
	CU_ASSERT(gdi->srcBpp == test_depth);
}

void test_gdi_clip_coords(void)
{
	RGN rgn1;
	RGN rgn2;

	SetRectRgn(gdi->clip, 0, 0, 0, 0);
	gdi->clip->null = 1;

	SetRectRgn(&rgn1, 10, 20, 30, 40);
	SetRectRgn(&rgn2, 10, 20, 30, 40);

	/* a null clipping region should not change the region to be clipped */
	gdi_clip_coords(gdi, &(rgn1.x), &(rgn1.y), &(rgn1.w), &(rgn1.h), 0, 0);
	CU_ASSERT(EqualRgn(&rgn1, &rgn2) == 1);

	SetRectRgn(&rgn1, 10, 20, 30, 40);
	SetRectRgn(&rgn2, 0, 0, 0, 0);
	SetRectRgn(gdi->clip, 100, 100, 200, 200);
	gdi->clip->null = 0;

	/* coordinates are all outside on the left */
	gdi_clip_coords(gdi, &(rgn1.x), &(rgn1.y), &(rgn1.w), &(rgn1.h), 0, 0);
	CU_ASSERT(EqualRgn(&rgn1, &rgn2) == 1);

	/* coordinates are all outside on the right */
	SetRectRgn(&rgn1, 210, 220, 230, 240);
	gdi_clip_coords(gdi, &(rgn1.x), &(rgn1.y), &(rgn1.w), &(rgn1.h), 0, 0);
	CU_ASSERT(EqualRgn(&rgn1, &rgn2) == 1);

	/* coordinates are all outside on top */
	SetRectRgn(&rgn1, 110, 20, 230, 40);
	gdi_clip_coords(gdi, &(rgn1.x), &(rgn1.y), &(rgn1.w), &(rgn1.h), 0, 0);
	CU_ASSERT(EqualRgn(&rgn1, &rgn2) == 1);

	/* coordinates are all outside at the bottom */
	SetRectRgn(&rgn1, 10, 220, 30, 240);
	gdi_clip_coords(gdi, &(rgn1.x), &(rgn1.y), &(rgn1.w), &(rgn1.h), 0, 0);
	CU_ASSERT(EqualRgn(&rgn1, &rgn2) == 1);

	/* left is outside, right is inside */
	SetRectRgn(&rgn1, 60, 110, 180, 190);
	SetRectRgn(&rgn2, 100, 110, 180, 190);
	gdi_clip_coords(gdi, &(rgn1.x), &(rgn1.y), &(rgn1.w), &(rgn1.h), 0, 0);
	CU_ASSERT(EqualRgn(&rgn1, &rgn2) == 1);

	/* left is inside, right is outside */
	SetRectRgn(&rgn1, 120, 110, 260, 190);
	SetRectRgn(&rgn2, 120, 110, 200, 190);
	gdi_clip_coords(gdi, &(rgn1.x), &(rgn1.y), &(rgn1.w), &(rgn1.h), 0, 0);
	CU_ASSERT(EqualRgn(&rgn1, &rgn2) == 1);

	/* top is outside, bottom is inside */
	SetRectRgn(&rgn1, 110, 60, 190, 180);
	SetRectRgn(&rgn2, 110, 100, 190, 180);
	gdi_clip_coords(gdi, &(rgn1.x), &(rgn1.y), &(rgn1.w), &(rgn1.h), 0, 0);
	CU_ASSERT(EqualRgn(&rgn1, &rgn2) == 1);

	/* top is inside, bottom is outside */
	SetRectRgn(&rgn1, 110, 120, 190, 260);
	SetRectRgn(&rgn2, 110, 120, 190, 200);
	gdi_clip_coords(gdi, &(rgn1.x), &(rgn1.y), &(rgn1.w), &(rgn1.h), 0, 0);
	CU_ASSERT(EqualRgn(&rgn1, &rgn2) == 1);

	//printf("rgn1 x:%d y:%d w:%d h:%d\n", rgn1.x, rgn1.y, rgn1.w, rgn1.h);
	//printf("rgn2 x:%d y:%d w:%d h:%d\n", rgn2.x, rgn2.y, rgn2.w, rgn2.h);
	//printf("clip x:%d y:%d w:%d h:%d\n", gdi->clip->x, gdi->clip->y, gdi->clip->w, gdi->clip->h);
}

void test_gdi_invalidate_region(void)
{
	RGN rgn1;
	RGN rgn2;

	gdi->primary_surface = NULL;
	gdi->drawing_surface = NULL;

	SetRectRgn(gdi->invalid, 0, 0, 0, 0);
	gdi->invalid->null = 1;
	
	SetRectRgn(&rgn1, 10, 20, 30, 40);
	SetRectRgn(&rgn2, 10, 20, 30, 40);

	/* a null invalid region onto which we invalidate a region should equal that region after */
	gdi_invalidate_region(gdi, rgn1.x, rgn1.y, rgn1.w, rgn1.h);
	CU_ASSERT(EqualRgn(gdi->invalid, &rgn2) == 1);

	/* a region with no area should not affect the resulting invalid region */
	SetRectRgn(&rgn1, 0, 0, 0, 0);
	SetRectRgn(&rgn2, 10, 20, 30, 40);
	gdi_invalidate_region(gdi, rgn1.x, rgn1.y, rgn1.w, rgn1.h);
	CU_ASSERT(EqualRgn(gdi->invalid, &rgn2) == 1);

	/* multiple regions should grow the resulting invalid region */
	SetRectRgn(&rgn1, 110, 120, 130, 140);
	SetRectRgn(&rgn2, 10, 20, 130, 140);
	gdi_invalidate_region(gdi, rgn1.x, rgn1.y, rgn1.w, rgn1.h);
	CU_ASSERT(EqualRgn(gdi->invalid, &rgn2) == 1);

	/* a region inside the invalid region should not affect the resulting invalid region */
	SetRectRgn(&rgn1, 60, 60, 80, 80);
	SetRectRgn(&rgn2, 10, 20, 130, 140);
	gdi_invalidate_region(gdi, rgn1.x, rgn1.y, rgn1.w, rgn1.h);
	CU_ASSERT(EqualRgn(gdi->invalid, &rgn2) == 1);
	
	//printf("rgn1 x:%d y:%d w:%d h:%d\n", rgn1.x, rgn1.y, rgn1.w, rgn1.h);
	//printf("rgn2 x:%d y:%d w:%d h:%d\n", rgn2.x, rgn2.y, rgn2.w, rgn2.h);
	//printf("invalid x:%d y:%d w:%d h:%d\n", gdi->invalid->x, gdi->invalid->y, gdi->invalid->w, gdi->invalid->h);
}


