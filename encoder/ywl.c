#include "codec.h"

static inline
int count_thresh_neighbours(short *p, int count, int step, int thresh)
{
	if (((abs(p[-1])))    >= thresh) count++;
	if (((abs(p[1])))     >= thresh) count++;
	if (((abs(p[-step]))) >= thresh) count++;
	if (((abs(p[step])))  >= thresh) count++;
	return count;
}

#define MODULO8(x)  ((x) & 7)

// Complex quantizer function:
//
// - Nulls coefs inside (-t0, t0)
// - 

// Note: t3 always <= t0 in all quality modes when ratio set to 8 (fixed)
// t1 must always be greater than t0, t2 always < t1

// FIXME: Turn into quantization lookup table

// This quantizer table requires one opcode bit for the neighbour threshold condition
//                  -t3   -t0      0       t0     t3
//                                 |
//           -7  -7  --    --  00  00  00  --     --   07   07
//  Op:       C   C  00    0   0   0   0   0      0    C    C
//

static
void quantize_process(short *p, int t0, int t1, int t2, int t3, int step, int cond)
{
	int a, e, f;
	int c;
	int p2;

	// Boundary condition:
	if (cond) {
		p2 = p[2];
	} else {
		p2 = 1; // Condition never met
	}

	e = p[0]; f = p[1];
	a = abs(e);
	if (a >= t0) {	
		// First pass: 
		if (a < t1) {
			// Score when neighbours are above threshold (6):
			c = count_thresh_neighbours(p, 0, step, 6);
			if ((c == 0) || (c <  3 && a < t2)) {
				if      (e < -t3) e = -7;
				else if (e >  t3) e =  7;
			}
		}


		// This condition is always met:
		// Because 6 <= t0 and above change always causes this to be true:
		// if (abs(e) > 6) {

		// This evals the next neighbour:

			if ( e >  7 && MODULO8(e) < 2) {
				if (f > 7 && f < 10000) f--;
				//if (p[step]>8) p[step]--;
			}
			// When neighbouring coefs
			else if (e == -7 && f == 8)  e = -8;
			else if (e == 8  && f ==-7)  f = -8;

			else if (e < -7  && (MODULO8(-e)) < 2) {
				if (f < -14 && f < 10000) {
					if (MODULO8(-f) == 7 || ((MODULO8(-f) < 2 && p2 <= 0)))
						f++;
				}
			}
		// }
		p[0] = e; p[1] = f;

	}
	else p[0]=0;

}

void SWAPOUT_FUNCTION(quant_ac_final)(image_buffer *im, int ratio, const short *y_wl)
{
	int i, c;
	short *pr = im->im_process;

	int step = im->fmt.tile_size;
	int halfs = step / 2;
	int im_size = im->fmt.end;

	// Notation:
	//
	//  LL  *HL*
	//  LH   HH

	for (i=step; i < ((im_size>>1)-step); i += step)
	{
		short *p = &pr[i];
		short *end = &p[step-1];

		for (p = &p[halfs+1]; p < end; p++)
		{
			quantize_process(p, (ratio-2), y_wl[1], 0x7fff, 6, step, p < &end[-1]);
		}
	}

	// Notation:
	//
	//  LL   HL
	// *LH*  HH

	// Scan LH:
	for (i=(im_size>>1);i<(im_size-step);i+=step)
	{
		short *p = &pr[i+1];
		short *end = &p[halfs-1];

		for (;p < end; p++) {
			quantize_process(p, (ratio-2), y_wl[1], y_wl[0], 0, step, p < &end[-1]);
		}
	}

	// Notation:
	//
	//  LL   HL
	//  LH  *HH*

	// Scan HH:
	for (i = (im_size>>1); i < (im_size-step); i += step)
	{
		short *p = &pr[i];
		short *end = &p[step-1];

		for (p = &p[halfs+1]; p < end; p++) {
			quantize_process(p, (ratio-1), y_wl[2], 0x7fff, 0, step, p < &end[-1]);
		}
	}
}
