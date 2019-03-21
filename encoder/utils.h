#define CLAMP2(x, low, high)  \
		if (x > high) { x = high ; } else if (x < low) { x = low; }

#define CLAMPH(x, high)  \
		if (x > high) { x = high ; }
#define CLAMPL(x, low)  \
		if (x < low) { x = low ; }

#define CODE_12000 12000
#define CODE_12100 12100
#define CODE_12200 12200
#define CODE_12500 12500
#define CODE_12600 12600
#define CODE_12300 12300
#define CODE_12400 12400
#define CODE_14000 14000
#define CODE_14100 14100
#define CODE_14500 14500
#define CODE_14900 14900

#ifndef __CONCAT
#define __CONCAT(x, y) x##y
#endif

