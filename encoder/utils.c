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

