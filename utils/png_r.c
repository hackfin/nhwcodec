/* PNG support
 *
 * added to NHW project <hackfin@section5.ch>
 *
 */

#include "imgio.h"
#include <stdarg.h>
#include <stdlib.h>
#include <png.h>

void abort_(const char * s, ...)
{
	va_list args;
	va_start(args, s);
	vfprintf(stderr, s, args);
	fprintf(stderr, "\n");
	va_end(args);
	abort();
}


static
int read_imgdata(png_structp pngp, png_infop infop, unsigned char *imagebuf, int height)
{
	png_bytep *row_pointers;
	unsigned char *p;
	int y;
	int n;
	n = png_set_interlace_handling(pngp);
	png_read_update_info(pngp, infop);

	/* read file */
	if (setjmp(png_jmpbuf(pngp)))
		abort_("[read_png_file] Error during read_image");

	row_pointers = (png_bytep *) malloc(sizeof(png_bytep) * height);
	p = imagebuf;
	y = height;
	int rowbytes = png_get_rowbytes(pngp, infop);
	while (y--) {
		row_pointers[y] = p;
		p += rowbytes;
	}

	png_read_image(pngp, row_pointers);

	free(row_pointers);
	return 0;
}

int open_png(png_structp *pngp, png_infop *infop, FILE *fp,
	int *width, int *height)
{
	int ret = 0;
	unsigned char header[8];	// 8 is the maximum size that can be checked

	ret = fread(header, 1, 8, fp);
	if (ret < 0 || png_sig_cmp(header, 0, 8))
		return -2;
	else ret = 0;


	/* initialize stuff */
	*pngp = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	
	if (!*pngp)
		abort_("[read_png_file] png_create_read_struct failed");

	*infop = png_create_info_struct(*pngp);
	if (!*infop)
		abort_("[read_png_file] png_create_info_struct failed");

	if (setjmp(png_jmpbuf(*pngp)))
		abort_("[read_png_file] Error during init_io");

	png_init_io(*pngp, fp);
	png_set_sig_bytes(*pngp, 8);

	png_read_info(*pngp, *infop);

	*width = png_get_image_width(*pngp, *infop);
	*height = png_get_image_height(*pngp, *infop);
	return ret;
}

int read_png(FILE *fp, unsigned char **imagebuf, int *width, int *height)
{
	png_structp pngp;
	png_infop infop;
	int ret;

	ret = open_png(&pngp, &infop, fp, width, height);

	printf("Open %d x %d\n", *width, *height);

	if (ret == 0) {
		*imagebuf = (unsigned char *) malloc((*width) * (*height) * 3 * sizeof(char));
		if (!*imagebuf) return -1;
		ret = read_imgdata(pngp, infop, *imagebuf, *height);
		png_destroy_info_struct(pngp, &infop);
		png_destroy_read_struct(&pngp, &infop, &infop);
	}
	return ret;
}

int read_png_tile(FILE *fp, unsigned char *imagebuf, int square_size)
{
	int height, width;
	int ret;
	png_structp pngp;
	png_infop infop;
	
	ret = open_png(&pngp, &infop, fp, &width, &height);

	if (width != height || width != square_size) {
		ret = -1;
		fprintf(stderr, "Img size %ux%u, requested %ux%u\n",
			width, height, square_size, square_size );
	} else {
		ret = read_imgdata(pngp, infop, imagebuf, square_size);
		png_destroy_info_struct(pngp, &infop);
		png_destroy_read_struct(&pngp, &infop, &infop);
	}
	return ret;
}

