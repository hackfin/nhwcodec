/* Frequently used code */

#include <stdio.h>
#include "utils.h"

/*
 *  Copy from LL quadrant 'X' of an image with dimensions x * y
 *  into contiguous image buffer:
 *
 *   <-- x -->
 *
 *   +---+---+  <-+
 *   | X |   |    |
 *   +---+---+    y
 *   |   |   |    |
 *   +---+---+  <-+
 *
 */

void copy_from_quadrant(short *dst, const short *src, int x, int y)
{
	int i;
	const short *src_end;
	x >>= 1;
	for (i = 0; i < (y >> 1); i++) {
		src_end = &src[x];
		while (src < src_end) {
			*dst++ = *src++;
		}
		src += x;
	}
}

/* Likewise, copy data to LL quadrant */

void copy_to_quadrant(short *dst, const short *src, int x, int y)
{
	int i;
	short *dst_end;
	x >>= 1;
	for (i = 0; i < (y >> 1); i++) {
		dst_end = &dst[x];
		while (dst < dst_end) {
			*dst++ = *src++;
		}
		dst += x;
	}
}

void copy_buffer(short *dst, const short *src, int x, int y)
{
	int n = x * y;
	while (n--) {
		*dst++ = *src++;
	}
}

void copy_buffer8bit(short *dst, const unsigned char *src, int n)
{
	while (n--) {
		*dst++ = *src++;
	}
}

void copy_bitplane0(unsigned char *sp, int n, unsigned char *res)
{
	while (n--) {
		*res++ = ((sp[0] & 1) <<7) | ((sp[1] & 1) <<6) |
				 ((sp[2] & 1) <<5) | ((sp[3] & 1) <<4) |
				 ((sp[4] & 1) <<3) | ((sp[5] & 1) <<2) |
				 ((sp[6] & 1) <<1) | ((sp[7] & 1));

		sp += 8;

	}
}

#define _EXTRACT_BIT(x, n) (((x) >> n) & 1)

void extract_bitplane(unsigned char *scan_run,
	unsigned char *tree, int n)
{
	int i;
	unsigned char pc[8];

	for (i=0; i< n; i += 8)
	{
		pc[0] = _EXTRACT_BIT(*tree++, 1);
		pc[1] = _EXTRACT_BIT(*tree++, 1);
		pc[2] = _EXTRACT_BIT(*tree++, 1);
		pc[3] = _EXTRACT_BIT(*tree++, 1);
		pc[4] = _EXTRACT_BIT(*tree++, 1);
		pc[5] = _EXTRACT_BIT(*tree++, 1);
		pc[6] = _EXTRACT_BIT(*tree++, 1);
		pc[7] = _EXTRACT_BIT(*tree++, 1);

		*scan_run++  = ((pc[0]) << 7) | ((pc[1]) << 6)
		             | ((pc[2]) << 5) | ((pc[3]) << 4)
		             | ((pc[4]) << 3) | ((pc[5]) << 2)
		             | ((pc[6]) << 1) | ((pc[7]));

	}
}

void dump_values_u16(unsigned short *u16, int n, const char *filename, const char *comment)
{
	int i;
	FILE *f = fopen(filename, "w");
	if (!f) {
		printf("Could not open file %s\n", filename);
	} else {
		fprintf(f, "# %s\n", comment);
		for (i = 0; i < n; i++) {
			fprintf(f, "%d  0x%04x\n", i, u16[i]);
		}
	}
}
