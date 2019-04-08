/* Image I/O */


#include <stdio.h>

int read_png_tile(FILE *fp, unsigned char *imagebuf, int square_size);
int read_png(FILE *fp, unsigned char **imagebuf, int *width, int *height);
int write_png(FILE *fp, unsigned char *imagebuf, int height, int width, int isrgb,
	int upside_down);


