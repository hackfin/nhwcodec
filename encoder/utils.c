/* Frequently used code */

/** Copy top left quadrant from source image to destination */
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


