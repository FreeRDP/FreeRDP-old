/*
   FreeRDP: A Remote Desktop Protocol client.
   Bitmap decompression routines

   Copyright (C) Matthew Chapman 1999-2008

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

/* three separate function for speed when decompressing the bitmaps
   when modifying one function make the change in the others
   jay.sorg@gmail.com */

/* indent is confused by this file */
/* *INDENT-OFF* */

#include "frdp.h"

#define CVAL(p)   (*(p++))
#ifdef NEED_ALIGN
#ifdef L_ENDIAN
#define CVAL2(p, v) { v = (*(p++)); v |= (*(p++)) << 8; }
#else
#define CVAL2(p, v) { v = (*(p++)) << 8; v |= (*(p++)); }
#endif /* L_ENDIAN */
#else
#define CVAL2(p, v) { v = (*((uint16*)p)); p += 2; }
#endif /* NEED_ALIGN */

#define UNROLL8(exp) { exp exp exp exp exp exp exp exp }

#define REPEAT(statement) \
{ \
	while((count & ~0x7) && ((x+8) < width)) \
		UNROLL8( statement; count--; x++; ); \
	\
	while((count > 0) && (x < width)) \
	{ \
		statement; \
		count--; \
		x++; \
	} \
}

#define MASK_UPDATE() \
{ \
	mixmask <<= 1; \
	if (mixmask == 0) \
	{ \
		mask = fom_mask ? fom_mask : CVAL(input); \
		mixmask = 1; \
	} \
}

/* 1 byte bitmap decompress */
static RD_BOOL
bitmap_decompress1(void * inst, uint8 * output, int width, int height, uint8 * input, int size)
{
	uint8 *end = input + size;
	uint8 *prevline = NULL, *line = NULL;
	int opcode, count, offset, isfillormix, x = width;
	int lastopcode = -1, insertmix = False, bicolor = False;
	uint8 code;
	uint8 color1 = 0, color2 = 0;
	uint8 mixmask, mask = 0;
	uint8 mix = 0xff;
	int fom_mask = 0;

	while (input < end)
	{
		fom_mask = 0;
		code = CVAL(input);
		opcode = code >> 4;
		/* Handle different opcode forms */
		switch (opcode)
		{
			case 0xc:
			case 0xd:
			case 0xe:
				opcode -= 6;
				count = code & 0xf;
				offset = 16;
				break;
			case 0xf:
				opcode = code & 0xf;
				if (opcode < 9)
				{
					count = CVAL(input);
					count |= CVAL(input) << 8;
				}
				else
				{
					count = (opcode < 0xb) ? 8 : 1;
				}
				offset = 0;
				break;
			default:
				opcode >>= 1;
				count = code & 0x1f;
				offset = 32;
				break;
		}
		/* Handle strange cases for counts */
		if (offset != 0)
		{
			isfillormix = ((opcode == 2) || (opcode == 7));
			if (count == 0)
			{
				if (isfillormix)
					count = CVAL(input) + 1;
				else
					count = CVAL(input) + offset;
			}
			else if (isfillormix)
			{
				count <<= 3;
			}
		}
		/* Read preliminary data */
		switch (opcode)
		{
			case 0:	/* Fill */
				if ((lastopcode == opcode) && !((x == width) && (prevline == NULL)))
					insertmix = True;
				break;
			case 8:	/* Bicolor */
				color1 = CVAL(input);
			case 3:	/* Colour */
				color2 = CVAL(input);
				break;
			case 6:	/* SetMix/Mix */
			case 7:	/* SetMix/FillOrMix */
				mix = CVAL(input);
				opcode -= 5;
				break;
			case 9:	/* FillOrMix_1 */
				mask = 0x03;
				opcode = 0x02;
				fom_mask = 3;
				break;
			case 0x0a:	/* FillOrMix_2 */
				mask = 0x05;
				opcode = 0x02;
				fom_mask = 5;
				break;
		}
		lastopcode = opcode;
		mixmask = 0;
		/* Output body */
		while (count > 0)
		{
			if (x >= width)
			{
				if (height <= 0)
					return False;
				x = 0;
				height--;
				prevline = line;
				line = output + height * width;
			}
			switch (opcode)
			{
				case 0:	/* Fill */
					if (insertmix)
					{
						if (prevline == NULL)
							line[x] = mix;
						else
							line[x] = prevline[x] ^ mix;
						insertmix = False;
						count--;
						x++;
					}
					if (prevline == NULL)
					{
						REPEAT(line[x] = 0)
					}
					else
					{
						REPEAT(line[x] = prevline[x])
					}
					break;
				case 1:	/* Mix */
					if (prevline == NULL)
					{
						REPEAT(line[x] = mix)
					}
					else
					{
						REPEAT(line[x] = prevline[x] ^ mix)
					}
					break;
				case 2:	/* Fill or Mix */
					if (prevline == NULL)
					{
						REPEAT
						(
							MASK_UPDATE();
							if (mask & mixmask)
								line[x] = mix;
							else
								line[x] = 0;
						)
					}
					else
					{
						REPEAT
						(
							MASK_UPDATE();
							if (mask & mixmask)
								line[x] = prevline[x] ^ mix;
							else
								line[x] = prevline[x];
						)
					}
					break;
				case 3:	/* Colour */
					REPEAT(line[x] = color2)
					break;
				case 4:	/* Copy */
					REPEAT(line[x] = CVAL(input))
					break;
				case 8:	/* Bicolor */
					REPEAT
					(
						if (bicolor)
						{
							line[x] = color2;
							bicolor = False;
						}
						else
						{
							line[x] = color1;
							bicolor = True; count++;
						}
					)
					break;
				case 0xd:	/* White */
					REPEAT(line[x] = 0xff)
					break;
				case 0xe:	/* Black */
					REPEAT(line[x] = 0)
					break;
				default:
					ui_unimpl(inst, "bitmap opcode 0x%x\n", opcode);
					return False;
			}
		}
	}
	return True;
}

/* 2 byte bitmap decompress */
static RD_BOOL
bitmap_decompress2(void * inst, uint8 * output, int width, int height, uint8 * input, int size)
{
	uint8 *end = input + size;
	uint16 *prevline = NULL, *line = NULL;
	int opcode, count, offset, isfillormix, x = width;
	int lastopcode = -1, insertmix = False, bicolor = False;
	uint8 code;
	uint16 color1 = 0, color2 = 0;
	uint8 mixmask, mask = 0;
	uint16 mix = 0xffff;
	int fom_mask = 0;

	while (input < end)
	{
		fom_mask = 0;
		code = CVAL(input);
		opcode = code >> 4;
		/* Handle different opcode forms */
		switch (opcode)
		{
			case 0xc:
			case 0xd:
			case 0xe:
				opcode -= 6;
				count = code & 0xf;
				offset = 16;
				break;
			case 0xf:
				opcode = code & 0xf;
				if (opcode < 9)
				{
					count = CVAL(input);
					count |= CVAL(input) << 8;
				}
				else
				{
					count = (opcode < 0xb) ? 8 : 1;
				}
				offset = 0;
				break;
			default:
				opcode >>= 1;
				count = code & 0x1f;
				offset = 32;
				break;
		}
		/* Handle strange cases for counts */
		if (offset != 0)
		{
			isfillormix = ((opcode == 2) || (opcode == 7));
			if (count == 0)
			{
				if (isfillormix)
					count = CVAL(input) + 1;
				else
					count = CVAL(input) + offset;
			}
			else if (isfillormix)
			{
				count <<= 3;
			}
		}
		/* Read preliminary data */
		switch (opcode)
		{
			case 0:	/* Fill */
				if ((lastopcode == opcode) && !((x == width) && (prevline == NULL)))
					insertmix = True;
				break;
			case 8:	/* Bicolor */
				CVAL2(input, color1);
			case 3:	/* Colour */
				CVAL2(input, color2);
				break;
			case 6:	/* SetMix/Mix */
			case 7:	/* SetMix/FillOrMix */
				CVAL2(input, mix);
				opcode -= 5;
				break;
			case 9:	/* FillOrMix_1 */
				mask = 0x03;
				opcode = 0x02;
				fom_mask = 3;
				break;
			case 0x0a:	/* FillOrMix_2 */
				mask = 0x05;
				opcode = 0x02;
				fom_mask = 5;
				break;
		}
		lastopcode = opcode;
		mixmask = 0;
		/* Output body */
		while (count > 0)
		{
			if (x >= width)
			{
				if (height <= 0)
					return False;
				x = 0;
				height--;
				prevline = line;
				line = ((uint16 *) output) + height * width;
			}
			switch (opcode)
			{
				case 0:	/* Fill */
					if (insertmix)
					{
						if (prevline == NULL)
							line[x] = mix;
						else
							line[x] = prevline[x] ^ mix;
						insertmix = False;
						count--;
						x++;
					}
					if (prevline == NULL)
					{
						REPEAT(line[x] = 0)
					}
					else
					{
						REPEAT(line[x] = prevline[x])
					}
					break;
				case 1:	/* Mix */
					if (prevline == NULL)
					{
						REPEAT(line[x] = mix)
					}
					else
					{
						REPEAT(line[x] = prevline[x] ^ mix)
					}
					break;
				case 2:	/* Fill or Mix */
					if (prevline == NULL)
					{
						REPEAT
						(
							MASK_UPDATE();
							if (mask & mixmask)
								line[x] = mix;
							else
								line[x] = 0;
						)
					}
					else
					{
						REPEAT
						(
							MASK_UPDATE();
							if (mask & mixmask)
								line[x] = prevline[x] ^ mix;
							else
								line[x] = prevline[x];
						)
					}
					break;
				case 3:	/* Colour */
					REPEAT(line[x] = color2)
					break;
				case 4:	/* Copy */
					REPEAT(CVAL2(input, line[x]))
					break;
				case 8:	/* Bicolor */
					REPEAT
					(
						if (bicolor)
						{
							line[x] = color2;
							bicolor = False;
						}
						else
						{
							line[x] = color1;
							bicolor = True;
							count++;
						}
					)
					break;
				case 0xd:	/* White */
					REPEAT(line[x] = 0xffff)
					break;
				case 0xe:	/* Black */
					REPEAT(line[x] = 0)
					break;
				default:
					ui_unimpl(inst, "bitmap opcode 0x%x\n", opcode);
					return False;
			}
		}
	}
	return True;
}

/* 3 byte bitmap decompress */
static RD_BOOL
bitmap_decompress3(void * inst, uint8 * output, int width, int height, uint8 * input, int size)
{
	uint8 *end = input + size;
	uint8 *prevline = NULL, *line = NULL;
	int opcode, count, offset, isfillormix, x = width;
	int lastopcode = -1, insertmix = False, bicolor = False;
	uint8 code;
	uint8 color1[3] = {0, 0, 0}, color2[3] = {0, 0, 0};
	uint8 mixmask, mask = 0;
	uint8 mix[3] = {0xff, 0xff, 0xff};
	int fom_mask = 0;

	while (input < end)
	{
		fom_mask = 0;
		code = CVAL(input);
		opcode = code >> 4;
		/* Handle different opcode forms */
		switch (opcode)
		{
			case 0xc:
			case 0xd:
			case 0xe:
				opcode -= 6;
				count = code & 0xf;
				offset = 16;
				break;
			case 0xf:
				opcode = code & 0xf;
				if (opcode < 9)
				{
					count = CVAL(input);
					count |= CVAL(input) << 8;
				}
				else
				{
					count = (opcode <
						 0xb) ? 8 : 1;
				}
				offset = 0;
				break;
			default:
				opcode >>= 1;
				count = code & 0x1f;
				offset = 32;
				break;
		}
		/* Handle strange cases for counts */
		if (offset != 0)
		{
			isfillormix = ((opcode == 2) || (opcode == 7));
			if (count == 0)
			{
				if (isfillormix)
					count = CVAL(input) + 1;
				else
					count = CVAL(input) + offset;
			}
			else if (isfillormix)
			{
				count <<= 3;
			}
		}
		/* Read preliminary data */
		switch (opcode)
		{
			case 0:	/* Fill */
				if ((lastopcode == opcode) && !((x == width) && (prevline == NULL)))
					insertmix = True;
				break;
			case 8:	/* Bicolor */
				color1[0] = CVAL(input);
				color1[1] = CVAL(input);
				color1[2] = CVAL(input);
			case 3:	/* Colour */
				color2[0] = CVAL(input);
				color2[1] = CVAL(input);
				color2[2] = CVAL(input);
				break;
			case 6:	/* SetMix/Mix */
			case 7:	/* SetMix/FillOrMix */
				mix[0] = CVAL(input);
				mix[1] = CVAL(input);
				mix[2] = CVAL(input);
				opcode -= 5;
				break;
			case 9:	/* FillOrMix_1 */
				mask = 0x03;
				opcode = 0x02;
				fom_mask = 3;
				break;
			case 0x0a:	/* FillOrMix_2 */
				mask = 0x05;
				opcode = 0x02;
				fom_mask = 5;
				break;
		}
		lastopcode = opcode;
		mixmask = 0;
		/* Output body */
		while (count > 0)
		{
			if (x >= width)
			{
				if (height <= 0)
					return False;
				x = 0;
				height--;
				prevline = line;
				line = output + height * (width * 3);
			}
			switch (opcode)
			{
				case 0:	/* Fill */
					if (insertmix)
					{
						if (prevline == NULL)
						{
							line[x * 3] = mix[0];
							line[x * 3 + 1] = mix[1];
							line[x * 3 + 2] = mix[2];
						}
						else
						{
							line[x * 3] =
							 prevline[x * 3] ^ mix[0];
							line[x * 3 + 1] =
							 prevline[x * 3 + 1] ^ mix[1];
							line[x * 3 + 2] =
							 prevline[x * 3 + 2] ^ mix[2];
						}
						insertmix = False;
						count--;
						x++;
					}
					if (prevline == NULL)
					{
						REPEAT
						(
							line[x * 3] = 0;
							line[x * 3 + 1] = 0;
							line[x * 3 + 2] = 0;
						)
					}
					else
					{
						REPEAT
						(
							line[x * 3] = prevline[x * 3];
							line[x * 3 + 1] = prevline[x * 3 + 1];
							line[x * 3 + 2] = prevline[x * 3 + 2];
						)
					}
					break;
				case 1:	/* Mix */
					if (prevline == NULL)
					{
						REPEAT
						(
							line[x * 3] = mix[0];
							line[x * 3 + 1] = mix[1];
							line[x * 3 + 2] = mix[2];
						)
					}
					else
					{
						REPEAT
						(
							line[x * 3] =
							 prevline[x * 3] ^ mix[0];
							line[x * 3 + 1] =
							 prevline[x * 3 + 1] ^ mix[1];
							line[x * 3 + 2] =
							 prevline[x * 3 + 2] ^ mix[2];
						)
					}
					break;
				case 2:	/* Fill or Mix */
					if (prevline == NULL)
					{
						REPEAT
						(
							MASK_UPDATE();
							if (mask & mixmask)
							{
								line[x * 3] = mix[0];
								line[x * 3 + 1] = mix[1];
								line[x * 3 + 2] = mix[2];
							}
							else
							{
								line[x * 3] = 0;
								line[x * 3 + 1] = 0;
								line[x * 3 + 2] = 0;
							}
						)
					}
					else
					{
						REPEAT
						(
							MASK_UPDATE();
							if (mask & mixmask)
							{
								line[x * 3] =
								 prevline[x * 3] ^ mix [0];
								line[x * 3 + 1] =
								 prevline[x * 3 + 1] ^ mix [1];
								line[x * 3 + 2] =
								 prevline[x * 3 + 2] ^ mix [2];
							}
							else
							{
								line[x * 3] =
								 prevline[x * 3];
								line[x * 3 + 1] =
								 prevline[x * 3 + 1];
								line[x * 3 + 2] =
								 prevline[x * 3 + 2];
							}
						)
					}
					break;
				case 3:	/* Colour */
					REPEAT
					(
						line[x * 3] = color2 [0];
						line[x * 3 + 1] = color2 [1];
						line[x * 3 + 2] = color2 [2];
					)
					break;
				case 4:	/* Copy */
					REPEAT
					(
						line[x * 3] = CVAL(input);
						line[x * 3 + 1] = CVAL(input);
						line[x * 3 + 2] = CVAL(input);
					)
					break;
				case 8:	/* Bicolor */
					REPEAT
					(
						if (bicolor)
						{
							line[x * 3] = color2[0];
							line[x * 3 + 1] = color2[1];
							line[x * 3 + 2] = color2[2];
							bicolor = False;
						}
						else
						{
							line[x * 3] = color1[0];
							line[x * 3 + 1] = color1[1];
							line[x * 3 + 2] = color1[2];
							bicolor = True;
							count++;
						}
					)
					break;
				case 0xd:	/* White */
					REPEAT
					(
						line[x * 3] = 0xff;
						line[x * 3 + 1] = 0xff;
						line[x * 3 + 2] = 0xff;
					)
					break;
				case 0xe:	/* Black */
					REPEAT
					(
						line[x * 3] = 0;
						line[x * 3 + 1] = 0;
						line[x * 3 + 2] = 0;
					)
					break;
				default:
					ui_unimpl(inst, "bitmap opcode 0x%x\n", opcode);
					return False;
			}
		}
	}
	return True;
}

/* decompress a color plane */
static int
process_plane(uint8 * in, int width, int height, uint8 * out, int size)
{
	int indexw;
	int indexh;
	int code;
	int collen;
	int replen;
	int color;
	int x;
	int revcode;
	uint8 * last_line;
	uint8 * this_line;
	uint8 * org_in;
	uint8 * org_out;

	org_in = in;
	org_out = out;
	last_line = 0;
	indexh = 0;
	while (indexh < height)
	{
		out = (org_out + width * height * 4) - ((indexh + 1) * width * 4);
		color = 0;
		this_line = out;
		indexw = 0;
		if (last_line == 0)
		{
			while (indexw < width)
			{
				code = CVAL(in);
				replen = code & 0xf;
				collen = (code >> 4) & 0xf;
				revcode = (replen << 4) | collen;
				if ((revcode <= 47) && (revcode >= 16))
				{
					replen = revcode;
					collen = 0;
				}
				while (collen > 0)
				{
					color = CVAL(in);
					*out = color;
					out += 4;
					indexw++;
					collen--;
				}
				while (replen > 0)
				{
					*out = color;
					out += 4;
					indexw++;
					replen--;
				}
			}
		}
		else
		{
			while (indexw < width)
			{
				code = CVAL(in);
				replen = code & 0xf;
				collen = (code >> 4) & 0xf;
				revcode = (replen << 4) | collen;
				if ((revcode <= 47) && (revcode >= 16))
				{
					replen = revcode;
					collen = 0;
				}
				while (collen > 0)
				{
					x = CVAL(in);
					if (x & 1)
					{
						x = x >> 1;
						x = x + 1;
						color = -x;
					}
					else
					{
						x = x >> 1;
						color = x;
					}
					x = last_line[indexw * 4] + color;
					*out = x;
					out += 4;
					indexw++;
					collen--;
				}
				while (replen > 0)
				{
					x = last_line[indexw * 4] + color;
					*out = x;
					out += 4;
					indexw++;
					replen--;
				}
			}
		}
		indexh++;
		last_line = this_line;
	}
	return (int) (in - org_in);
}

/* 4 byte bitmap decompress */
static RD_BOOL
bitmap_decompress4(uint8 * output, int width, int height, uint8 * input, int size)
{
	int code;
	int bytes_pro;
	int total_pro;

	code = CVAL(input);
	if (code != 0x10)
	{
		return False;
	}
	total_pro = 1;
	bytes_pro = process_plane(input, width, height, output + 3, size - total_pro);
	total_pro += bytes_pro;
	input += bytes_pro;
	bytes_pro = process_plane(input, width, height, output + 2, size - total_pro);
	total_pro += bytes_pro;
	input += bytes_pro;
	bytes_pro = process_plane(input, width, height, output + 1, size - total_pro);
	total_pro += bytes_pro;
	input += bytes_pro;
	bytes_pro = process_plane(input, width, height, output + 0, size - total_pro);
	total_pro += bytes_pro;
	return size == total_pro;
}

/* main decompress function */
RD_BOOL
bitmap_decompress(void * inst, uint8 * output, int width, int height, uint8 * input, int size, int Bpp)
{
	RD_BOOL rv = False;

	switch (Bpp)
	{
		case 1:
			rv = bitmap_decompress1(inst, output, width, height, input, size);
			break;
		case 2:
			rv = bitmap_decompress2(inst, output, width, height, input, size);
			break;
		case 3:
			rv = bitmap_decompress3(inst, output, width, height, input, size);
			break;
		case 4:
			rv = bitmap_decompress4(output, width, height, input, size);
			break;
		default:
			ui_unimpl(inst, "Bpp %d\n", Bpp);
			break;
	}
	return rv;
}

/* *INDENT-ON* */
