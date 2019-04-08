#include "codec.h"
#include "utils.h"
#include <assert.h>

void reduce_HL_LH_q4(image_buffer *im);

#define REDUCE(x, w)    if (abs(x) < w)  x = 0

#define FLOOR2(x) ( (x) & ~1)

#define RESIII_GETXY(x, y, off)  \
					(((((y) >> 1) + (x)) >> 1) + (off))

#define IN_RANGE(x, a, b)  ( ((x) >= a) && ((x) <= b) )

// TODO: Simplify all functions inside reduce_generic():
//
// They all basically quantize the coefficients in the high frequency
// (LH/HL) subbands, depending on their neighbours:
//
// reduce_LH_q6()
//

inline void reduce(short *q, int s, char w0, char w1, char w2, int halfsize, int half_dim)
{
	//  LL   HL  q1   ..  
	//  LH   HH  ..   .. 
	//   q   ..  AA   .. <- halfsize
	//  ..   ..  ..   ..


	short *q1= &q[half_dim];

	REDUCE(q1[0], w0); REDUCE(q1[1], w0);
	q1 += s; // Next line
	REDUCE(q1[0], w0); REDUCE(q1[1], w0);
	
	q += halfsize; // now at 'q'

	REDUCE(q[0], w1); REDUCE(q[1], w1);
	q += s; // Next line
	REDUCE(q[0], w1); REDUCE(q[1], w1);
	
	q -= half_dim; // now at AA
	REDUCE(q[0], w2); REDUCE(q[1], w2);
	q += s;
	REDUCE(q[0], w2); REDUCE(q[1], w2);
}

#define CONDITION_A(p, w, v) \
	   abs(p[4]-p[0]) < w \
	&& abs(p[4]-p[3]) < w \
	&& abs(p[1]-p[0]) < w \
	&& abs(p[3]-p[1]) < w \
	&& abs(p[3]-p[2]) < v

#define CONDITION_B(p, w, v) \
	   abs(p[4]-p[0]) < w \
	&& abs(p[4]-p[3]) < w \
	&& abs(p[1]-p[0]) < w \
	&& abs(p[3]-p[1]) < v \
	&& abs(p[3]-p[2]) < v

void reduce_lowres_LL_q7(image_buffer *im, const unsigned char *wvlt)
{
	int i, j, count, scan;
	int e;
	char w, v;

	short *pr = im->im_process;
	int quality = im->setup->quality_setting;

	int s = im->fmt.tile_size;
	int im_dim = s / 2;
	int half = im_dim / 2;
	int im_size = im->fmt.end / 4;
	int s2 = 2 * s;

	// Depending on the conditions in LL, the A, B and C quadrants
	// are subject to a coefficient reduction (DZ quantization)
	// This is done by the reduce() function.
	//
	// *LL*  HL   BB   BB  <- im_size
	//  LH   HH   BB   BB  
	//  AA   AA   CC   CC
	//  AA   AA   CC   CC
	for (i=0,scan=0;i<(im_size);i+=s)
	{
		short *p = &pr[i];
		for (scan=i,j=0;j<half-4;j++,scan++, p++)
		{
			w = wvlt[0];
			v = wvlt[1];

			if (CONDITION_A(p, w, v-2))
			{
				if      ((p[3]-p[1])>5 && (p[2]-p[3]>=0)) { p[2]=p[3]; }
				else if ((p[1]-p[3])>5 && (p[2]-p[3]<=0)) { p[2]=p[3]; }
				else if ((p[1]-p[3])>5 && (p[2]-p[1]>=0)) { p[2]=p[1]; }
				else if ((p[3]-p[1])>5 && (p[2]-p[1]<=0)) { p[2]=p[1]; }
				else if ((p[3]-p[2])>0 && (p[2]-p[1]>0))  { }
				else if ((p[1]-p[2])>0 && (p[2]-p[3]>0))  { }
				else
				                                     p[2]=(p[3]+p[1])>>1;
						
				short *q;
				for (q = &p[scan + 2]; q < &p[scan + 8]; q += 2) {
					reduce(q, s, wvlt[5], wvlt[5] + 6, wvlt[4], im->fmt.half, im_dim);
				}
					
				if (quality<=LOW9) { // FIXME: inefficient compare inside loop
					for (q = &p[1]; q < &p[4]; q++) {
						REDUCE(q[half], 11);
						REDUCE(q[im_size], 12);
						REDUCE(q[im_size+half],13);
					}
				}
			}
			else {
				w = v + 1;
				v += 6;
				if (CONDITION_B(p, w, v))
				{
					if (((p[3]-p[2])>=0 && (p[2]-p[1])>=0) ||
						((p[3]-p[2])<=0 && (p[2]-p[1])<=0)) 
					{
						short *q;
						for (q = &p[scan + 2]; q < &p[scan + 8]; q += 2) {
							reduce(q, s, wvlt[5], wvlt[5] + 6, wvlt[4], im->fmt.half, im_dim);
						}
						
						if (quality<=LOW9) {
							for (q = &p[1]; q < &p[4]; q++) {
								REDUCE(q[half], 11);
								REDUCE(q[im_size], 12);
								REDUCE(q[im_size+half],13);
							}
						}
					}
				}
			}
		}
	}

	w = wvlt[2];
	v = wvlt[3];

	// *LL*  HL
	//  LH   HH
	
	for (i=0,scan=0;i<(im_size)-s2;i+=s)
	{
		for (scan=i,j=0;j<half-2;j++,scan++)
		{
			short *p = &pr[scan];
			if (abs(p[1]-p[s2+1]) < w
			 && abs(p[s]-p[s+2])  < w
			 && abs(p[s+1]-p[s])  < (v-1)
			 && abs(p[1]-p[s+1])  < v)
			{
				e=(p[1]+p[s2+1]+p[s]+p[s+2]+2)>>2;
					
				if (abs(e-p[s])<5 || abs(e-p[s+2])<5)
				{
					p[s+1]=e;
				}
				
				count=scan+s+1;

				short *q = &pr[(count<<1)];
				reduce(q, s, wvlt[5], wvlt[5] + 6, 32, im->fmt.half, im_dim);

				if (quality<=LOW9)
				{
					for (e=count-1;e<count+2;e++)
					{
						q = &pr[e];
						REDUCE(q[half], 11);
						q += im_size;
						REDUCE(q[0], 12);
						REDUCE(q[half],13);
					}
				}
			}
		}
	}

	// *LL*  HL
	//  LH   HH
	
	for (i=0,scan=0;i<im_size-s2;i+=s)
	{
		for (scan=i,j=0;j<half-2;j++,scan++)
		{
			short *p = &pr[scan];
			if (abs(p[2]-p[1])  <w
			 && abs(p[1]-p[0])  <w
			 && abs(p[0]-p[s])  <w
			 && abs(p[2]-p[s+2])<w)
			{
				if (abs(p[s2+1]-p[s]) < w && abs(p[s]-p[s+1]) < v) 
				{
					e=(p[1]+p[s2+1]+p[s]+p[s+2]+1)>>2;
					
					if (abs(e-p[s])<5 || abs(e-p[s+2])<5)
					{
						p[s+1]=e;
					}
					
					count=scan+s+1;

					short *q = &pr[(count<<1)];

					reduce(q, s, wvlt[5], wvlt[5] + 6, 32, im->fmt.half, im_dim);
				}
				
				if (quality<=LOW9)
				{
					for (e=count-1;e<count+2;e++)
					{
						short *q = &pr[e];
						REDUCE(q[half], 11);
						q += im_size;
						REDUCE(q[0], 12);
						REDUCE(q[half],13);
					}
				}
			}
		}
	}
}

void reduce_lowres_LL_q9(image_buffer *im, const unsigned char *wvlt)
{
	int i, j, count, scan;
	char w;
	int im_size = im->fmt.end / 4;
	short *pr = im->im_process;
	int step = im->fmt.tile_size;
	int im_dim = step / 2;
	int half = im_dim / 2;

	// *LL*  HL  ..   ..  <- im_size
	//  LH   HH  ..   ..  
	//  ..   ..  ..   ..
	//  ..   ..  ..   ..
	//  <--  step    -->

	for (i=0,scan=0;i<(im_size);i+=step)
	{
		for (scan=i,j=0;j<half-2;j++,scan++)
		{
			w = wvlt[6];
			short *p = &pr[scan];
			if (abs(p[2]-p[1])  < w
			 && abs(p[2]-p[0])  < w
			 && abs(p[1]-p[0])  < w)
			{
				count=scan+1;

				reduce(&pr[(count<<1)], step, wvlt[5], wvlt[5] + 6, 34, im->fmt.half, im_dim);
					
				short *q = &pr[count];
				REDUCE(q[half], 11);
				q += im_size;
				REDUCE(q[0], 12);
				REDUCE(q[half],13);
			}
		}
	}
}

void SWAPOUT_FUNCTION(reduce_lowres_LH)(image_buffer *im,
	const unsigned char *wvlt, int ratio)
{
	int i, j, scan;

	int step = im->fmt.tile_size;
	int im_size = im->fmt.end / 4;

	// upper left quadrant:
	//  LL    HL    ..    ..
	// *LH*   HH    ..    ..
	//  ..    ..    ..    ..
	//  ..    ..    ..    ..

	for (i=im_size;i<(2*im_size);i+=step)
	{
		for (scan=i,j=0;j<(step>>1);j++,scan++)
		{
			short *p = &im->im_process[scan];
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

const char threshold_table[][7] = {
	{ 11, 15, 10, 15, 36, 20,   21          },  // LOW20
	{ 11, 15, 10, 15, 36, 20,   21          },
	{ 11, 15, 10, 15, 36, 19,   20          },
	{ 11, 15, 10, 15, 36, 18,   18          },
	{ 11, 15, 10, 15, 36, 17,   17          },
	{ 11, 15, 10, 15, 36, 17,   17          },
	{ 11, 15, 10, 15, 36, 17,   17          },
	{ 10, 15,  9, 14, 36, 17,   17          },
	{  8, 13,  6, 11, 34, 15,   15          },
	{  8, 13,  6, 11, 34, 15,   15          },
	{  8, 13,  6, 11, 34, 15,   15          },
	{  8, 13,  6, 11, 34, 15,   15          },
	{  8, 13,  6, 11, 34, 14,   0xff        }, // LOW8
	{ 15, 27, 10,  6,  3, 0xff, 0xff        }, // LOW7
	{ 16, 28, 11,  8,  5, 0xff, 0xff        }, // LOW6
	
};


const char quality_special_default[7] = { 16, 28, 11, 8, 5, 0xff, 0xff };
const char quality_special12500[7] = { 19, 31, 13, 9, 6, 0xff, 0xff };
const char quality_special10000[7] = { 18, 30, 12, 8, 6, 0xff, 0xff };
const char quality_special7000[7] = { 17, 29, 11, 8, 5, 0xff, 0xff };

int configure_wvlt(int quality, char *wvlt)
{
	const char *w;

	if (quality < 0) quality = 0;
	else if (quality > LOW6) quality = LOW6;

	w = threshold_table[quality];
	COPY_WVLT(wvlt, w)
	return 0;
}

void SWAPOUT_FUNCTION(reduce_generic_LH_HH)(image_buffer *im,
	short *resIII, int ratio, char *wvlt, int thr)
{
	int i, scan, j, res3=0;
	short tmp;
	int im_size = im->fmt.end / 4;
	int step = im->fmt.tile_size;
	int im_dim = step / 2;
	short *pr = im->im_process;
	//  LL    HL 
	// *LH*   HH

	for (i=im->fmt.half;i<im->fmt.end;i+=step)
	{
		for (scan=i,j=0;j<(im_dim);j++,scan++)
		{
			short *p = &pr[scan];
			if (abs(p[0])>=ratio &&  abs(p[0])<(wvlt[0]+2)) 
			{	
				if (i<(im->fmt.end-(2*im_dim)))
				{
					tmp = resIII[RESIII_GETXY(j, (i-im->fmt.half), im_size >> 1)];
					res3=1;
				}
				else res3=0;
				
				if (res3 && abs(tmp) < wvlt[3]) p[0]=0;
				else if (abs(p[0]+p[-1])<wvlt[4] && abs(p[1])<wvlt[4]) {
					p[0]=0;p[-1]=0;
				} else if (abs(p[0]+p[1])<wvlt[4] && abs(p[-1])<wvlt[4]) {
					p[0]=0;p[1]=0;
				}
			}
			
			if (abs(p[0])>=ratio && abs(p[0])<wvlt[0]) 
			{	
				if ((abs(p[-1]) < (ratio) && abs(p[1])<(ratio))
				 || (abs(p[0]) < (wvlt[0]-4)) ) {
					p[0]=0;
				}
			}
		}
	
	//  LL    HL 
	//  LH   *HH*

		for (scan=i+(im_dim),j=(im_dim);j<(step-1);j++,scan++)
		{
			short *p = &pr[scan];
			if (abs(p[0])>=ratio) {
				if (i<(im->fmt.end-step))
				{
					int index = RESIII_GETXY(j-im_dim, i-im->fmt.half, (im_size>>1)+(im_dim>>1));
					tmp = resIII[index];
					
					res3=1;
				}
				else res3=0;
				
				if (abs(p[0])<(wvlt[1]+1)) 
				{	
					if (res3 && abs(tmp)<(wvlt[3]+1)) p[0]=0;
					else if (abs(p[0]+p[-1]) < wvlt[4]
						  && abs(p[1])       < wvlt[4]) 
					{
						p[0]=0;p[-1]=0;
					}
					else if (abs(p[0]+p[1]) < wvlt[4]
						  && abs(p[-1])     < wvlt[4]) 
					{
						p[0]=0;p[1]=0;
					}
				}
				
				if (((abs(p[0])<wvlt[1]
				 && (abs(p[-1])<ratio
					 && abs(p[1])<ratio ) )
					 || (abs(p[0])<(wvlt[1]-5)))) {
						if      (p[0] >=  thr) p[0] = 7;
						else if (p[0] <= -thr) p[0] = -7;
						else     p[0]=0;							
				}
			}
		}
	}


}

void reduce_LH_q5(image_buffer *im, int ratio)
{
	int i, j, scan;

	short *pr = im->im_process;
	int step = im->fmt.tile_size;
	int im_dim = step / 2;

		//  Full tile:
		//  LL    HL
		// *LH*   HH

	for (i=im->fmt.half;i<im->fmt.end;i+=step)
	{
		for (scan=i,j=0;j<im_dim;j++,scan++)
		{
			short *p = &pr[scan];

			if (abs(p[0])>=ratio && abs(p[0])<9) 
			{	
				 if (p[0]>0) p[0]=7;else p[0]=-7;	
			}
		}

		for (scan=i+(im_dim),j=(im_dim);j<step;j++,scan++)
		{
			short *p = &pr[scan];
			if (abs(p[0])>=ratio && abs(p[0])<=14) 
			{	
				 if (p[0]>0) p[0]=7;else p[0]=-7;	
			}
		}
	}
}

void SWAPOUT_FUNCTION(reduce_LH_HH_q56)(image_buffer *im, int ratio, char *wvlt)
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

int count_threshold(short *pr, int from, int to, int thresh)
{
	int count;
	int i;
	for (i = from,count=0;i < to; i++) {
		if (abs(pr[i]) >= thresh) count++;
	}
	return count;
}

void SWAPOUT_FUNCTION(reduce_HL_LH_q4)(image_buffer *im)
{
	int i, j, scan;
	short *pr = im->im_process;
	int step = im->fmt.tile_size;
	int im_dim = step / 2;

//  LL   *HL*
//  LH    HH 

	
	for (i=step; i<(im->fmt.half-step);i+=step)
	{
		for (scan=i+ im_dim+1,j= im_dim+1;j<(step-1);j++,scan++)
		{
			short *p = &pr[scan];

//             -8  -7  -6  -5  -4  -3  -2  -1   0   1   2   3   4   5   6   7   8
//   p[0]:          B   B   B                                       A   A   A
//   p[-1]:         B   B   B   B                               A   A   A   A
//   p[1]:          B   B   B   B                               A   A   A   A
//

			if (IN_RANGE(p[0], 5, 7)) {
				if (IN_RANGE(p[-1], 4, 7)) {
					if (IN_RANGE(p[1], 4, 7)) {
						p[0]=MARK_127;p[-1]=MARK_128;p[1]=MARK_128; // A
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
				if (FLOOR2(p[-1])==6 || FLOOR2(p[1])==6) p[0]=10;
				else if (p[1]==8) {p[0]=9;p[1]=9;}
			}
			else if (p[0]==-8)
			{
				if (FLOOR2(-p[-1])==6 || FLOOR2(-p[1])==6) p[0]=-9;
				else if (p[1]==-8) {p[0]=-9;p[1]=-9;}
			}
		}
	}

	//  LL    HL 
	// *LH*   HH 

	for (i=(im->fmt.half+step);i<(im->fmt.end-step);i+=step)
	{
		for (scan=i+1,j=1;j< im_dim-1;j++,scan++)
		{
			short *p = &pr[scan];
			if (IN_RANGE(p[0], 5, 7)) {
				if (IN_RANGE(p[-1], 4, 7)) {
					if (IN_RANGE(p[1], 4, 7)) {
						p[0]=MARK_127;p[-1]=MARK_128;p[1]=MARK_128; // A
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
#if 0 // CONDITION NEVER MET
			else if (p[0]==-6 || p[0]==-7)
			{
				if (p[1]==-7)
				{
					p[0]=MARK_125;p[1]=MARK_128;
				}
				else if (p[-step]==-7)
				{
					if (abs(p[im_dim])<8) p[im_dim]=MARK_125;p[0]=MARK_128;
				}
				
			}
			else if (p[0]==7)
			{
				if (p[1]==7)
				{
					p[0]=MARK_126;p[1]=MARK_128;
				}
				else if (p[-step]==7)
				{
					if (abs(p[im_dim])<8) p[im_dim]=MARK_126;p[0]=MARK_128;
				}
			}
#endif
			else if (p[0]==8)
			{
				if (FLOOR2(p[-1])==6 || FLOOR2(p[1])==6) p[0]=10;
			}
			else if (p[0]==-8)
			{
				if (FLOOR2(-p[-1])==6 || FLOOR2(-p[1])==6) p[0]=-9;
			}
		}
	}

}


void SWAPOUT_FUNCTION(reduce_HL)(image_buffer *im,
short *resIII, int ratio, char *wvlt)
{
	int i, j, scan;
	int step = im->fmt.tile_size;
	int im_dim = step / 2;
	short *pr = im->im_process;
	//  Full tile, first stage:
	//
	//  LL   *HL*
	//  LH    HH 
	
	for (i=0;i < im->fmt.half;i+=step)
	{
		for (scan=i+im_dim,j=im_dim;j<step;j++,scan++)
		{
			short *p = &pr[scan];
			if (abs(p[0]) >= ratio) {
				if (abs(p[0]) < (wvlt[2]+2)) 
				{	
					short tmp = resIII[RESIII_GETXY(j-im_dim, i, im_dim >> 1)];

					if (abs(tmp) < wvlt[3])
						p[0] = 0;
					// if (abs(resIII[(((i>>1)+(j-im_dim))>>1)+(im_dim>>1)])<wvlt[3]) p[0]=0;
					else if (abs(p[0]+p[-1]) < wvlt[4] && abs(p[1]) < wvlt[4]) {
						p[0] = 0; p[-1] = 0;
					}
					else if (abs(p[0]+p[1]) < wvlt[4] && abs(p[-1]) < wvlt[4]) {
						p[0] = 0; p[1] = 0;
					}
				}
				
				if (abs(p[0]) < wvlt[2]) 
				{	
					if (abs(p[-1])<ratio && abs(p[1])<ratio) 
					{
						p[0]=0;
					}
				}
			}
		}
	}
}

void reduce_generic(image_buffer *im, short *resIII, char *wvlt, encode_state *enc, int ratio)
{
	int count;
	int quality = im->setup->quality_setting;
	short *pr = im->im_process;

	if (IN_RANGE(quality, LOW4, HIGH1)) {
		reduce_LH_q5(im, ratio);
	} else
	if (IN_RANGE(quality, LOW6, LOW5)) {
		wvlt[0] = 11;

		if      (quality == LOW5) wvlt[1]=19;
		else if (quality == LOW6) wvlt[1]=20;
		
		reduce_LH_HH_q56(im, ratio, wvlt);

	} else if (quality < LOW6) { 
		if (quality <= LOW8) {
			count = count_threshold(pr, im->fmt.half, im->fmt.end, 12);

			//if (count>MARK_15000) {wvlt[0]=20;wvlt[1]=32;wvlt[2]=13;wvlt[3]=8;wvlt[4]=5;}
			if      (count > 12500)      COPY_WVLT(wvlt, quality_special12500)
			else if (count > 10000)      COPY_WVLT(wvlt, quality_special10000)
			else if (count >= 7000)      COPY_WVLT(wvlt, quality_special7000)
			else                         COPY_WVLT(wvlt, quality_special_default);
			if (quality == LOW9) {
				if (count > 12500) { wvlt[0]++;wvlt[1]++;wvlt[2]++;wvlt[3]++;wvlt[4]++; }
				else wvlt[0]++;
			} else if (quality <= LOW10) 
			{
				if (count > 12500) { wvlt[0]+=3;wvlt[1]+=3;wvlt[2]+=2;wvlt[3]+=3;wvlt[4]+=3; }
				else               { wvlt[0]+=3;wvlt[1]+=2;wvlt[2]+=2;wvlt[3]+=2;wvlt[4]+=2; }
			}
		}
	
		reduce_HL(im, resIII, ratio, wvlt);

		if (quality > LOW10) {
			reduce_generic_LH_HH(im, resIII, ratio, wvlt, 16);
		} else {
			reduce_generic_LH_HH(im, resIII, ratio, wvlt, 0xffff);
		}

	}

	if (quality > LOW4) { 
		reduce_HL_LH_q4(im);
	}
}

void reduce_uv_q4(image_buffer *im, int ratio)
{
	int i, j;
	int imhalf  = im->fmt.end / 8;
	int quad_size = im->fmt.end / 4;
	int s2 = im->fmt.tile_size / 2;
	int s4 = s2 >> 1;

	short *pr;

	for (i = 0 ;i < imhalf;i += s2)
	{
		pr = &im->im_process[i + s4];
		for (j= s4; j< s2; j++, pr++)
		{
			if (abs(*pr) >= ratio && abs(*pr) < 24) *pr = 0;
		}
	}	
		
	for (i = imhalf; i < quad_size;i += s2)
	{
		pr = &im->im_process[i];
		for (j = 0;j < s4;j++, pr++) {
			if (abs(*pr) >= ratio && abs(*pr) < 32) *pr = 0;
		}

		pr = &im->im_process[i + s4];
		for (j = s4;j< s2;j++,pr++) {
			if (abs(*pr) >= ratio && abs(*pr) < 48) *pr = 0;
		}
	}
}


void reduce_uv_q9(image_buffer *im)
{
	int i, j;
	unsigned char wvlt[7];
	int s2 = im->fmt.tile_size / 2;

	wvlt[0] = 2;
	wvlt[1] = 3;
	wvlt[2] = 5;
	wvlt[3] = 8;
	int imquart = im->fmt.end / 16;

	for (i=0; i < imquart-(2*s2); i+=s2) {
		short *p = &im->im_process[i];
		short *q = &p[s2];

		for (j=0;j< (s2>>2)-2;j++,p++,q++) {

			if (abs(p[1]-q[s2+1]) <  wvlt[2]
			 && abs(q[0]-q[2])    <  wvlt[2]
			 && abs(q[1]-q[0])    < (wvlt[3]-1)
			 && abs(p[1]-q[1])    <  wvlt[3])
			{
				q[1] = (p[1] + q[s2+1] + q[0] + q[2]+2) >> 2;
			}
		}
	}
	
	for (i=0;i<imquart-(2*s2);i+=(s2)) {
		short *p = &im->im_process[i];
		short *q = &p[s2];

		for (j=0;j<(s2>>2)-2;j++,p++, q++) {

			if (abs(p[2]-p[1])     < wvlt[2]
			 && abs(p[1]-p[0])     < wvlt[2]
			 && abs(p[0]-q[0])     < wvlt[2]
			 && abs(p[2]-q[2])     < wvlt[2]
			 && abs(q[s2+1]-q[0])  < wvlt[2]
			 && abs(q[0]-q[1])     < wvlt[3])
			{
				q[1] = (p[1] + q[s2+1]+ q[0] + q[2] + 1) >> 2;
			}
		}
	}

}

void lowres_uv_compensate(short *dst, const short *pr, const short *lowres, int end, int step, int t0, int t1)
{
	int i, j;
	int val;
	short l;
	short z;

	for (i = 0; i < end;i += step) {
		const short *p = &pr[i];
		short *q = &dst[i];
		for (j=0;j<(step>>1);j++)
		{
			l = lowres[0];

			val = *p - l;
	
			if      (val > 10)  { z = l - 6;}
			else if (val > 7)   { z = l - 3;}
			else if (val > 4)   { z = l - 2;}
			else if (val > 3)     z = l - 1;
			else if (val > 2 && (p[1]-lowres[1]) >= t0)
			                      z = l - 1;
			else if (val < -10) { z = l + 6;}
			else if (val < -7)  { z = l + 3;}
			else if (val < -4)  { z = l + 2;}
			else if (val < -3)    z = l + 1;
			else if (val < -2 && (p[1]-lowres[1]) <= t1)
			                      z = l + 1;
			else                  z = l;
			
			*q++ = z;
			p++; lowres++;
		}
	}
}


void preprocess_res_q8(image_buffer *im, short *res256, encode_state *enc)
{
	int i, j, count, res, stage, scan;
	int res_setting;
	int step = im->fmt.tile_size;
	int im_dim = step / 2;

	int quality = im->setup->quality_setting;
	short *pr = im->im_process;

	// Put into LUT:
	if      (quality >= NORM) res_setting=3;
	else if (quality >= LOW2) res_setting=4;
	else if (quality >= LOW5) res_setting=6;
	else                      res_setting=8;

	// *LL*  HL
	//  LH   HH

	// TODO: Pad last 2 lines with something sensible

	int shift = im->fmt.tile_power;

	for (j=0,count=0,res=0,stage=0;j<im_dim;j++)
	{
		for (scan=j,count=j,i=0;
			i<(im->fmt.half-step);i+=step,scan+=step,count+=im_dim)
		{
			int r0 = res256[count+im_dim];
			int rcs = res256[count+step]; // This is stepping out of image bounds on the last line

			int k = (i >> shift) + (j << shift);
			short *p;
			p = &pr[scan];

			int a;

			stage = k + im_dim;

			short *q = &pr[stage];
			
			short *p1 = &p[step];
			short *p2 = &p1[step];

			res = p[0] - res256[count];
			a = p[step] - r0;

			short qd = *p2 - rcs;


////////////////////////////////////////////////////////////////////////////
// XXX Unfortunately, this can't simply be put into a table.
// Do we need to develop a VLIW machine?
////////////////////////////////////////////////////////////////////////////

#define _MOD_ASSIGN(x, y) \
	{ res256[count]=__CONCAT(CODE_,x); *p1 += y; *p2 += y; }



			if (res==2 && a==2 && qd>=2)
			{
				if (qd<5 || qd>6) _MOD_ASSIGN(12400, -2);
			}
			else if (((res==2 && a==3) || (res==3 && a==2)) && qd>1 && qd<6)
				 _MOD_ASSIGN(12400, -2)
			else if ((res==3 && a==3))
			{
				if (qd>0 && qd<6)
					 _MOD_ASSIGN(12400, -2)
				else if (quality>=LOW1)
				{
					res256[count]=CODE_12100;p[step]=r0;
				}
			}
			else if (a==-4 && (res==2 || res==3) && (qd==2 || qd==3))
			{
				if (res==2 && qd==2) p[step]++;
				else _MOD_ASSIGN(12400, -2)
			} // DONE
			else if (res==1 && a==3 && qd==2) // DONE
			{
				// FIXME: No compare of loop variable inside loop
				if (i>0) 
				{
					if ((p[-step]-res256[count-im_dim])>=0) 
						_MOD_ASSIGN(12400, -2)
				}
			}
			else if ((res==3 || res==4 || res==5 || res>6) &&
				(a==3 ||
				(a& ~1)==4))
			{
				if ((res)>6) {res256[count]=CODE_12500;p[step]=r0;}
				else if (quality>=LOW1) // REDUNDANT
				{
					res256[count]=CODE_12100;p[step]=r0;
				}
				else if (quality==LOW2) // DONE
				{
					if (res<5 && a==5)       p[step] = CODE_14100;
					else if (res>=5)   res256[count] = CODE_14100;
					else if (res==3 && a>=4) p[step] = CODE_14100;
					
					res256[count+im_dim] = p[step];
				}
			}
			else if ((res==2 || res==3) && (a==2 || a==3))
			{ 
				if (qd == 0 || qd == 1)
				{
					if ((p[1]-res256[count+1])==2 || (p[1]-res256[count+1])==3)
					{
						if ((p1[1]-res256[count+im_dim+1])==2 || (p1[1]-res256[count+im_dim+1])==3)
						{
							if ((p2[1]-res256[count+2*im_dim+1])>0)
							{
								 _MOD_ASSIGN(12400, -2)
							}
						}
					}
				}
			}
			else if (a==4 && (res==-2 || res==-3) && (qd==-2 || qd==-3)) // DONE
			{
				if (res==-2 && qd==-2) p[step]--;
				else _MOD_ASSIGN(12300, 2)
			}
			else if ((res==-3 || res==-4 || res==-5 || res<-7) &&
				(a == -3 || a == -4 || a == -5))
			{
				if (res<-7) 
				{
					res256[count]=CODE_12600;p[step]=r0;
				}
				else if (quality>=LOW1)
				{
					res256[count]=CODE_12200;p[step]=r0;
				}
				else if (quality==LOW2)
				{
					if (res>-5 && a==-5)       p[step]=CODE_14000;
					else if (res<=-5) res256[count]=CODE_14000;
					else if (res==-3 && a<=-4) p[step]=CODE_14000;
					
					res256[count+im_dim]=p[step];
				}
			}
			else if (a==-2 || a==-3)
			{
				if (res==-2 || res==-3)
				{
					if(qd<0) _MOD_ASSIGN(12300, 2)
					else if (res==-3 && quality>=HIGH1)
					{
						res256[count]=CODE_14500;
					}
					else if (qd == 0)
					{
						if ((p[1]-res256[count+1])==-2 || (p[1]-res256[count+1])==-3)
						{
							if ((p[(2*im_dim+1)]-res256[count+(im_dim+1)])==-2
							 || (p[(2*im_dim+1)]-res256[count+(im_dim+1)])==-3)
							{
								if ((p2[(1)]-res256[count+(2*im_dim+1)])<0)
									_MOD_ASSIGN(12300, 2)
							}
						}
					}
					else if (res==-2) goto L_W2;
					else goto L_W3;
				}
				else if (res==-1 && a==-3 && qd==-2)
				{
					// FIXME: No compare of loop variable inside loop
					if (i>0) 
					{
						if ((p[-step]-res256[count-im_dim])<=0) 
							_MOD_ASSIGN(12300, 2)
					}
				}
				else if (res==-1)
				{
					if (qd==-3)
						_MOD_ASSIGN(12300, 2)
					else goto L_W1;

				}
				else if (res==-4)
				{
					if (qd<-1)
					{
						if(qd>-4)
							_MOD_ASSIGN(12300, 2)
						else goto L_W5;
					}
					else goto L_W5;
				}
			}
			else if (!res || res==-1)
			{
L_W1:
				if (q[0]==7)
				{
					if (q[0-1]>=0 && q[0-1]<8) q[0]+=2;
				}
				else if (q[0]==8)
				{
					if (q[0-1]>=-2 && q[0-1]<8) q[0]+=2;
				}
			}
			else if (res==-2)
			{
L_W2:

				if (q[0]<-14)
				{
					if (!((-q[0])&7) || ((-q[0])&7)==7) q[0]++;
				}
				else if (q[0]==7 || (q[0]& ~1)==8)
				{
					if (q[0-1]>=-2) q[0]+=3;
				}
			}
			else if (res==-3) 
			{
L_W3:			if (quality>=HIGH1) {res256[count]=CODE_14500;} // Redundant
				else if (q[0]<-14)
				{
					if (!((-q[0])&7) || ((-q[0])&7)==7)
					{
						q[0]++;
					}
				}
				else if (q[0]>=0 && ((q[0]+2)& ~3)==8)
				{
					if (q[0-1]>=-2) q[0]=10;
				}
				else if (q[0]>14 && (q[0]&7)==7)
				{
					q[0]++;
				}
			}
			else if (res<(-res_setting))
			{
L_W5:			res256[count]=CODE_14000;

				if (res==-4)
				{

					if (q[0]==-7 || q[0]==-8) 
					{
						if (q[0-1]<2 && q[0-1]>-8) q[0]=-9;
					}
				}
				else if (res<-6)
				{
					if (res<-7 && quality>=HIGH1) {res256[count]=CODE_14900;}
					else
					{
						if (q[0]<-14)
						{
							if (!((-q[0])&7) || ((-q[0])&7)==7) q[0]++;
						}
						else if (q[0]==7 || q[0]==8)
						{
							if (q[0-1]>=-1 && q[0-1]<8) q[0]+=3;
						}
					}
				}
			}

		}	
	}

	enc->res1.word_len=0;
	enc->res3.word_len=0;
	enc->res5.word_len=0;

	// Process all values from res256 array. When tagged, replace by opcode,
	// else set to 0.

	for (i=0,count=0,stage=0,res=0;i<(im->fmt.half);i+=step)
	{
		for (scan=i,j=0;j<im_dim;j++,scan++,count++)
		{
			short r = res256[count];
			if (!IS_CODE(r))
			{
				// Transposed check pointer:
				short *p = &pr[(j << shift)+(i >> shift)+(im_dim)];

				res= pr[scan]-res256[count];
				res256[count]=0;

				switch (res) {
					case 0:
					case 1:
						if (p[0]==-7 || p[0]==-8) {
							if (p[-1]<2 && p[-1]>-8) p[0]=-9;
						}
						break;
					case 2:
						switch (p[0]) {
							case -7:
							case -8:
								if (p[-1] <= 1) p[0] = -9;
								break;
							case -6:
								if (p[-1] <= -1 && p[-1]>-8) p[0]=-9;
								break;
							default:
								if (p[0] > 15 && (p[0] & 7) == 0) p[0]--;
						}
						break;
					case 3:
						if (quality>=HIGH1)
							{ res256[count] = OP_R51_N; enc->res5.word_len++; }
						else
						{
							if (p[0] > 15 && (p[0] & 7) == 0) p[0]--;
							else if (p[0] <= 0
							 && ((((-p[0]) + 2) & ~3) == 8)) 
							{
								if (p[-1] <= 2) p[0]=-10;
							}
						}
						break;
					default:
						if (res>res_setting) 
						{
							res256[count]=OP_R11_N;enc->res1.word_len++;

							if (res == 4)
							{
								if (p[0] == 7 || (p[0] & ~1)==8)
								{
									if (p[-1] >= 0 && p[-1] < 8) p[0] += 2;
								}
							}
							else if (res>6) 
							{
								if (res > 7 && quality>=HIGH1) 
								{
									res256[count]=OP_R5R1N;
									enc->res5.word_len++;
									enc->res1.word_len++;
								}
								else
								{
									if (p[0] > 15 && (p[0] & 7) == 0) p[0]--;
									else if (p[0] == -6 || p[0]== -7 || p[0] == -8) 
									{
										if (p[-1] < 0 && p[-1] > -8) p[0] = -9;
									}
								}
							}
						}
				}
			} else {
// USE_OPCODE: Eliminate this one
				switch (r) {
					case CODE_14000:
						r = OP_R10_P; enc->res1.word_len++; 
						break;
					case CODE_14500:
						r = OP_R50_P; enc->res5.word_len++;
						break;
					case CODE_12200:
						r = OP_R30_P; enc->res3.word_len++; 
						break;
					case CODE_12100:
						r = OP_R31_N; enc->res3.word_len++;
						break;
					case CODE_12300:
						r = OP_R32_P; enc->res3.word_len++; 
						break;
					case CODE_12400:
						r = OP_R33_N; enc->res3.word_len++;
						break;
					case CODE_14100:
						r = OP_R11_N; enc->res1.word_len++; 
						break;
					case CODE_12500:
						r = OP_R3R1N; enc->res3.word_len++;
						enc->res1.word_len++;
						break;
					case CODE_12600:
						r = OP_R3R0P;enc->res3.word_len++;
						enc->res1.word_len++;
						break;
					case CODE_14900:
						r = OP_R5R1P;enc->res5.word_len++;
						enc->res1.word_len++;
						break;
					default:
						assert(0);
				}	

				res256[count] = r;
			}
		}
	}
}
