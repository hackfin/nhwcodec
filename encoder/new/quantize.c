// FIXME: Eliminate
#define COPY_WVLT(dst, src) \
	{ \
	dst[0] = src[0]; \
	dst[1] = src[1]; \
	dst[2] = src[2]; \
	dst[3] = src[3]; \
	dst[4] = src[4]; \
	dst[5] = src[5]; \
	dst[6] = src[6]; \
	}


#include "utils.h"
#include "codec.h"

#define MODULO8(x)  ((x) & 7)

// FIXME: Indexing ineffective in software, ok in HW
#define RESIII_GETXY(x, y)  \
					(((((y) >> 1) + (x)) >> 1))

#define IN_RANGE(x, a, b)  ( ((x) >= a) && ((x) <= b) )

#define FLOOR2(x) ( (x) & ~1)

extern const char quality_special_default[7];
extern const char quality_special12500[7];
extern const char quality_special10000[7];
extern const char quality_special7000[7];

void reduce_lowres_LL_q7(image_buffer *im, const char *wvlt);
void reduce_lowres_LL_q9(image_buffer *im, const char *wvlt);
void reduce_HL_LH(short *p, int mode);
void quant_zero(short tmp, short *p, char th2, char th3, int ratio);

void reduce_lowres_LH(image_buffer *im,
	const char *wvlt, int ratio)
{
	int i, j;

	int step = im->fmt.tile_size;
	int im_size = im->fmt.end / 4;

	// upper left quadrant:
	//  LL    HL    ..    ..
	// *LH*   HH    ..    ..
	//  ..    ..    ..    ..
	//  ..    ..    ..    ..

	for (i=im_size;i<(2*im_size);i+=step)
	{
		short *p = &im->im_process[i];

		for (j=0;j<(step>>1);j++,p++)
		{
			if ((abs(p[0]) >= ratio && abs(p[0]) < wvlt[0]
				  && abs(p[-1]) < ratio
				  && abs(p[1])<ratio)
			   || 
					 (abs(p[0]) == ratio
				  && (abs(p[-1]) < ratio || abs(p[1]) < ratio))
			   )  {
					p[0]=0;
			}
		}
	}
}

void reduce_lowres_generic(image_buffer *im, char *wvlt, int ratio)
{
	int quality = im->setup->quality_setting;

	if (quality <= LOW9) // Worse than LOW9?
	{
		if (quality > LOW14) wvlt[0] = 10; else wvlt[0] = 11;
		reduce_lowres_LH(im, wvlt, ratio);
	}
		
	configure_wvlt(quality, wvlt);

	if (quality < LOW7) {
			
		reduce_lowres_LL_q7(im, wvlt);
		
		if (quality <= LOW9) {
			reduce_lowres_LL_q9(im, wvlt);
		}
	}
}

// Fixme: Eliminate mode ? Will break binary compatibility.
//
// This function tags pixels whose neighbour conditions match certain specifics
inline
void reduce_HL_LH(short *p, int mode)
{

//             -8  -7  -6  -5  -4  -3  -2  -1   0   1   2   3   4   5   6   7   8
//   p[0]:          B   B   B                                       A   A   A
//   p[-1]:         B   B   B   B                               A   A   A   A
//   p[1]:          B   B   B   B                               A   A   A   A
//

	if (IN_RANGE(p[0], 5, 7)) {
		if (IN_RANGE(p[-1], 4, 7)) {
			if (IN_RANGE(p[1], 4, 7)) {
				p[0]=MARK_127;p[-1]=MARK_128;p[1]=MARK_128;   // A
			}
		}
	} else
	if (IN_RANGE(p[0], -7, -5)) {
		if (IN_RANGE(p[-1], -7, -4)) {
			if (IN_RANGE(p[1], -7, -4)) {
				p[0]=MARK_129;p[-1]=MARK_128;p[1]=MARK_128;	  // B
			}
		}
	}
	else if (p[0]==8)
	{
		if (FLOOR2(p[-1])==6 || FLOOR2(p[1])==6) p[0]=10;     // C
		else if (mode && p[1]==8) {p[0]=9;p[1]=9;}            // D
	}
	else if (p[0]==-8)
	{
		if (FLOOR2(-p[-1])==6 || FLOOR2(-p[1])==6) p[0]=-9;   // E
		else if (mode && p[1]==-8) {p[0]=-9;p[1]=-9;}         // F
	}

}

void reduce_HL_LH_q4(image_buffer *im)
{
	int i, j;
	short *pr = im->im_process;
	int step = im->fmt.tile_size;
	int im_dim = step / 2;

//  LL   *HL*
//  LH    HH 
	
	for (i=step; i<(im->fmt.half-step);i+=step)
	{
		short *p = &pr[i+im_dim+1];
		for (j = im_dim+1;j<(step-1);j++,p++)
		{
			// Not sure if the reduction makes sense like this in HL area
			// Try transposed?
			reduce_HL_LH(p, 1);
		}
	}

	//  LL    HL 
	// *LH*   HH 

	for (i=(im->fmt.half+step);i<(im->fmt.end-step);i+=step)
	{
		short *p = &pr[i+1];
		for (j=1;j< im_dim-1;j++,p++)
		{
			reduce_HL_LH(p, 0);
		}
	}

}

// Warning: Lazy coded, can actually 'artefact' due to
// p[-1] pointing into the LL quad for the first value

inline void quant_zero(short tmp, short *p, char th2, char th3, int ratio)
{
	if (abs(tmp) < th2) p[0] = 0;
	// if (abs(resIII[(((i>>1)+(j-im_dim))>>1)+(im_dim>>1)])<th2) p[0]=0;
	else if (abs(p[0]+p[-1]) < th3 && abs(p[1]) < th3) {
		p[0] = 0; p[-1] = 0;
	}
	else if (abs(p[0]+p[1]) < th3 && abs(p[-1]) < th3) {
		p[0] = 0; p[1] = 0;
	}

	if (abs(p[-1]) < ratio && abs(p[1]) < ratio) {
		p[0]=0;
	}
}

void reduce_generic_LH_HH(image_buffer *im,
	const short *resIII, int ratio, char *wvlt, int thr)
{
	int i, j;
	int ri;
	short tmp;
	int im_size = im->fmt.end / 4;
	int step = im->fmt.tile_size;
	int im_dim = step / 2;
	short *pr = im->im_process;
	//  LL    HL 
	// *LH*   HH

	const short *r0 = &resIII[im_size >> 1];
	const short *r1 = &r0[im_dim >> 1];

	for (i=im->fmt.half, ri = 0;i<im->fmt.end;i+=step, ri += step) {
		short *p = &pr[i];
		for (j=0;j<(im_dim);j++,p++) {
			if (IN_RANGE(abs(p[0]), ratio, wvlt[0]+1)) {	
				if (i<(im->fmt.end-(2*im_dim))) { // FIXME
					tmp = r0[RESIII_GETXY(j, ri)];
				} else tmp= 0x7fff;
				quant_zero(tmp, p, wvlt[3], wvlt[4], ratio);
			}
		}
	
	//  LL    HL 
	//  LH   *HH*

		for (j=0;j<im_dim-1;j++,p++) {
			if (IN_RANGE(abs(p[0]), ratio, wvlt[1])) {	
				if (i<(im->fmt.end-step)) {
					tmp = r1[RESIII_GETXY(j, ri)];
				} else tmp=0x7fff;
				quant_zero(tmp, p, wvlt[3], wvlt[4], ratio);
			}
		}
	}


}

void reduce_HL(image_buffer *im, const short *resIII, int ratio, char *wvlt)
{
	int i, j;
	int step = im->fmt.tile_size;
	int im_dim = step / 2;
	short *pr = im->im_process;
	const short *r = &resIII[im_dim >> 1];

	int ri;
	for (i = im_dim, ri = 0;i < im->fmt.half;i+=step, ri += step) {
		short *p = &pr[i];

		for (j=0;j<im_dim;j++,p++) {
			if (IN_RANGE(abs(p[0]), ratio, wvlt[2])) {
				short tmp = r[RESIII_GETXY(j, ri)];
				quant_zero(tmp, p, wvlt[3], wvlt[4], ratio);
			}
		}
	}
}

void reduce_LH_HH_q56(image_buffer *im, int ratio, char *wvlt)
{
	int i, j;
	//  LL    HL
	// *LH*  *HH*

	short *pr = im->im_process;
	int step = im->fmt.tile_size;
	int im_dim = step / 2;

	for (i=im->fmt.half;i<im->fmt.end;i+=step)
	{
		short *p = &pr[i];

		for (j=0;j< im_dim; j++,p++)
		{		
			if (abs(p[0])>=ratio && abs(p[0])<wvlt[0]) { p[0] = 0; }
		}

		for (j= im_dim;j<step;j++,p++)
		{
			if (abs(p[0])>=ratio && abs(p[0]) < wvlt[1]) {	
				if      (p[0] >= 14)       p[0]=7;
				else if (p[0] <= -14)      p[0]=-7;	
				else 	                   p[0]=0;	
			}
		}
	}
}

//
//       'pr'                       'resIII'
// +---+---+---+---+  <-            +---+---+
// |LLL|LHL|       |                |   | * |
// +---+---+  HL   +   i            +---+---+
// |LLH|LHH|       |                | * | * |
// +---+---+---+---+  <-            +---+---+
// |       |       |
// +  LH   +  HH   +
// |       |       |
// +---+---+---+---+  <- (END) 
//         ^.......^  <- j

// This function does a simple dead zone quantization, depending
// on the [pr] values and the corresponding low res ones in [resIII]

void reduce_generic_simplified(image_buffer *im,
	const short *resIII, char *wvlt, encode_state *enc, int ratio)
{
	// printf("%s(): simplified quantization\n", __FUNCTION__);

	reduce_HL(im, resIII, ratio, wvlt);
	reduce_generic_LH_HH(im, resIII, ratio, wvlt, 16);
}

static inline
int count_thresh_neighbours(short *p, int count, int step, int thresh)
{
	if (((abs(p[-1])))    >= thresh) count++;
	if (((abs(p[1])))     >= thresh) count++;
	if (((abs(p[-step]))) >= thresh) count++;
	if (((abs(p[step])))  >= thresh) count++;
	return count;
}

// FIXME: For HL/LH bands, frequency considerations should be taken
// into account for neighbour scoring

static
void quantize_process_simple(short *p, int t0, int t1, int t2, int t3, int step, int cond)
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

#if 0
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
#endif
		p[0] = e; p[1] = f;

	}
	else p[0]=0;

}

void quant_ac_final(image_buffer *im, int ratio, const short *y_wl)
{
	int i;
	short *pr = im->im_process;

	int step = im->fmt.tile_size;
	int halfs = step / 2;
	int im_size = im->fmt.end;

	// printf("%s(): simplified quantization\n", __FUNCTION__);

	// Notation:
	//
	//  LL  *HL*  ..   ..
	//  LH   HH
	//  ..
	//  ..

	for (i=step; i < ((im_size>>1)-step); i += step)
	{
		short *p = &pr[i];
		short *end = &p[step-1];

		for (p = &p[halfs+1]; p < end; p++) {
			quantize_process_simple(p, (ratio-2), y_wl[1], 0x7fff, 6, step, p < &end[-1]);
		}
	}

	//  LL   HL  ..   ..
	// *LH*  HH
	//  ..
	//  ..

	// Scan LH:
	for (i=(im_size>>1);i<(im_size-step);i+=step)
	{
		short *p = &pr[i+1];
		short *end = &p[halfs-1];

		for (;p < end; p++) {
			quantize_process_simple(p, (ratio-2), y_wl[1], y_wl[0], 0, step, p < &end[-1]);
		}
	}

	//  LL   HL  ..   ..
	//  LH  *HH*
	//  ..
	//  ..

	// Scan HH:
	for (i = (im_size>>1); i < (im_size-step); i += step)
	{
		short *p = &pr[i];
		short *end = &p[step-1];

		for (p = &p[halfs+1]; p < end; p++) {
			quantize_process_simple(p, (ratio-1), y_wl[2], 0x7fff, 0, step, p < &end[-1]);
		}
	}
}
