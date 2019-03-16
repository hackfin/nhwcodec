#include "codec.h"

static inline
int count_cond(short *p, int scan, int step)
{
	if (((abs(p[-1])))    >= 6) scan++;
	if (((abs(p[1])))     >= 6) scan++;
	if (((abs(p[-step]))) >= 6) scan++;
	if (((abs(p[step])))  >= 6) scan++;
	return scan;
}

#define MODULO7(x)  ((x) & 7)


inline reduce_yterms(short *p, short e, short f, int step, int condition)
{
	if (abs(e) > 6) {
		if ( e>= 8 && MODULO7(e) < 2) {
			if (f > 7 && f < 10000) f--;
			//if (p[step]>8) p[step]--;
		}
		else if (e == -7 && f ==8)  e = -8;
		else if (e == 8  && f ==-7) f = -8;
		else if (e < -7  && (MODULO7(-e)) < 2) {
			if (f < -14 && f < 10000) {
				if (MODULO7(-f) == 7
				|| ((MODULO7(-f) < 2
				 && condition && p[2] <= 0))) f++;
			}
		}
	}
	p[0] = e; p[1] = f;
}

void ywl(int quality, short *pr, int ratio)
{

	int y_wl[2];
	int i, j, count, scan;
	int e, f;
	int a;

	int step = 2 * IM_DIM;
	int halfs = IM_DIM;
	int im_size = 4*IM_SIZE;

	if (quality>HIGH2) {
		y_wl[0]=8 ;y_wl[1]=4;
	} else {
		y_wl[0]=9 ;y_wl[1]=9;
	}

	// Notation:
	//
	// LL HL
	// LH HH

	// Scan HL:
	for (i=step,count=0,scan=0; i < ((im_size>>1)-step); i += step)
	{
		for (j=(halfs+1);j<(step-1);j++)
		{
			short *p = &pr[i+j];
			e = p[0]; f = p[1];
			a = abs(p[0]);
// Possible conditions for a:
// We have  6 < y_wl[0], select = 8

// If 'a' in [(ratio-2)..ywl2]:
			if (a >= (ratio-2)) {
				if (a < y_wl[1]) {
					scan = count_cond(p, scan, step);
					if (scan < 3) {
						//printf("%d %d %d\n",p[-1],p[0],p[1]);
						if      (e < -6) e = -7;
						else if (e >  6) e =  7;
					}
					scan=0;
				}
				reduce_yterms(p, e, f, step, j<(step-2));
			}
			else p[0]=0;
		}
	}

	if (quality>HIGH2)     { y_wl[0] = 8 ; y_wl[1] = 4; }
	else if (quality>LOW3) { y_wl[0] = 8 ; y_wl[1] = 9; }
	else                   { y_wl[0] = 9 ; y_wl[1] = 9; }

	// Notation:
	//
	// LL HL
	// LH HH

	// Scan LH:
	for (i=(im_size>>1),scan=0;i<(im_size-step);i+=step)
	{
		for (j=1;j<(halfs);j++)
		{
			short *p = &pr[i+j];

			e = p[0]; f = p[1];
			a = abs(e);

			if (a >= (ratio-2)) {	
				if (a < y_wl[1]) {
					scan = count_cond(p, scan, step);
					if ((scan <  3 && a < y_wl[0])
					 || (scan == 0)) {
						if (e < 0) e = -7 ; else e =7;
					}
					scan=0;
				}
				reduce_yterms(p, e, f, step, j<(step-2));
			}
			else p[0] = 0;
		}
	}

	if (quality > HIGH2) y_wl[0] = 8;
	else                 y_wl[0] = 11;

	// Notation:
	//
	// LL HL
	// LH HH

	// Scan HH:
	for (i = (im_size>>1), scan=0, count=0; i < (im_size-step); i += step)
	{
		for (j=(halfs+1);j<(step-1);j++)
		{
			short *p = &pr[i+j];
			e = p[0]; f = p[1];
			a = abs(e);

			if (a >= (ratio-1)) {	
				if (a < y_wl[0]) {
					scan = count_cond(p, scan, step);
					if (scan < 3) {
						if (e < 0) e = -7;else e = 7;
					}
					scan = 0;
				}
				reduce_yterms(p, e, f, step, j<(step-2));
			}
			else p[0]=0;
		}
	}


}
