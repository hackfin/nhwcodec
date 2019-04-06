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

#define RESIII_GETXY(x, y, off)  \
					(((((y) >> 1) + (x)) >> 1) + (off))

#define IN_RANGE(x, a, b)  ( ((x) >= a) && ((x) <= b) )

#define FLOOR2(x) ( (x) & ~1)

extern const char quality_special_default[7];
extern const char quality_special12500[7];
extern const char quality_special10000[7];
extern const char quality_special7000[7];

void reduce_lowres_LL_q7(image_buffer *im, const char *wvlt);
void reduce_lowres_LL_q9(image_buffer *im, const char *wvlt);

void reduce_lowres_LH(image_buffer *im,
	const unsigned char *wvlt, int ratio)
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
	short *resIII, int ratio, char *wvlt, int thr)
{
	int i, j;
	short tmp;
	int im_size = im->fmt.end / 4;
	int step = im->fmt.tile_size;
	int im_dim = step / 2;
	short *pr = im->im_process;
	//  LL    HL 
	// *LH*   HH

	for (i=im->fmt.half;i<im->fmt.end;i+=step) {
		short *p = &pr[i];
		for (j=0;j<(im_dim);j++,p++) {
			if (IN_RANGE(abs(p[0]), ratio, wvlt[0]+1)) {	
				if (i<(im->fmt.end-(2*im_dim))) { // FIXME
					tmp = resIII[RESIII_GETXY(j, (i-im->fmt.half), im_size >> 1)];
				} else tmp= 0x7fff;
				quant_zero(tmp, p, wvlt[3], wvlt[4], ratio);
			}
		}
	
	//  LL    HL 
	//  LH   *HH*

		for (j=(im_dim);j<(step-1);j++,p++)
		{
			if (IN_RANGE(abs(p[0]), ratio, wvlt[1])) {	
				if (i<(im->fmt.end-step)) {
					int index = RESIII_GETXY(j-im_dim, i-im->fmt.half, (im_size>>1)+(im_dim>>1));
					tmp = resIII[index];
				} else tmp=0x7fff;
				quant_zero(tmp, p, wvlt[3], wvlt[4], ratio);
			}
		}
	}


}

void reduce_HL(image_buffer *im, short *resIII, int ratio, char *wvlt)
{
	int i, j;
	int step = im->fmt.tile_size;
	int im_dim = step / 2;
	short *pr = im->im_process;
	//  Full tile, first stage:
	//
	//  LL   *HL*
	//  LH    HH 
	
	for (i=0;i < im->fmt.half;i+=step) {
		short *p = &pr[i + im_dim];

		for (j=im_dim;j<step;j++,p++) {
			if (IN_RANGE(abs(p[0]), ratio, wvlt[2])) {
				short tmp = resIII[RESIII_GETXY(j-im_dim, i, im_dim >> 1)];
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

void reduce_generic_simplified(image_buffer *im,
	short *resIII, char *wvlt, encode_state *enc, int ratio)
{
	reduce_HL(im, resIII, ratio, wvlt);
	reduce_generic_LH_HH(im, resIII, ratio, wvlt, 16);
}

