/* Image I/O */


#include <stdio.h>

int read_png(FILE *fp, unsigned char *imagebuf, int square_size);
int write_png(FILE *fp, unsigned char *imagebuf, int height, int width, int isrgb);
