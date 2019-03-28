#define CLAMP2(x, low, high)  \
		if (x > high) { x = high ; } else if (x < low) { x = low; }

#define CLAMPH(x, high)  \
		if (x > high) { x = high ; }
#define CLAMPL(x, low)  \
		if (x < low) { x = low ; }

// Special code markers:

#define CODE_12000 12000
#define CODE_12100 12100
#define CODE_12200 12200
#define CODE_12300 12300
#define CODE_12500 12500
#define CODE_12600 12600
#define CODE_12300 12300
#define CODE_12400 12400
#define CODE_12900 12900
#define CODE_13000 13000
#define CODE_14000 14000
#define CODE_14100 14100
#define CODE_14500 14500
#define CODE_14900 14900

#define CODE_15800 15800

#define CODE_15300 15300
#define CODE_15400 15400
#define CODE_15500 15500
#define CODE_15600 15600
#define CODE_15700 15700
#define CODE_15800 15800

#define MARK_121   12100
#define MARK_122   12204
#define MARK_125   10204
#define MARK_126   10300
#define MARK_127   12700
#define MARK_128   10100
#define MARK_129   12900

#ifndef __CONCAT
#define __CONCAT(x, y) x##y
#endif

void copy_bitplane0(unsigned char *sp, int n, unsigned char *res);
void copy_buffer(short *dst, const short *src, int x, int y);
void copy_to_quadrant(short *dst, const short *src, int x, int y);
void copy_from_quadrant(short *dst, const short *src, int x, int y);

