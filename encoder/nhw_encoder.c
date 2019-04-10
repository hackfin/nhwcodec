/***************************************************************************
****************************************************************************
*  NHW Image Codec 													       *
*  file: nhw_encoder.c  										           *
*  version: 0.1.6 						     		     				   *
*  last update: $ 02172019 nhw exp $							           *
*																		   *
****************************************************************************
****************************************************************************

****************************************************************************
*  remark: -simple codec												   *
***************************************************************************/

/* Copyright (C) 2007-2019 NHW Project
   Written by Raphael Canut - nhwcodec_at_gmail.com */
/*
   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions
   are met:

   - Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.

   - Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.

   - Neither the name of NHW Codec, or NHW Project, nor the names of 
   specific contributors, may be used to endorse or promote products 
   derived from this software without specific prior written permission.

   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
   ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
   A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER
   OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
   EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
   PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
   PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
   LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
   NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
   SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <png.h>

#include "imgio.h"
#include "codec.h"
#include "utils.h"

#ifndef MAYBE_STATIC
#define MAYBE_STATIC
#endif

#define CLIP(x) ( (x<0) ? 0 : ((x>255) ? 255 : x) );

#define ARRAY_SIZE(a)  (sizeof(a) / sizeof(a[0]))
#define LAST_ELEMENT(a)  (ARRAY_SIZE(a)-1)

#define IS_ODD(x) (((x) & 1) == 1)

// Local protos

void encode_y(image_buffer *im, encode_state *enc, int ratio);
void reduce_lowres_LL_q7(image_buffer *im, const char *wvlt);
void reduce_lowres_LL_q9(image_buffer *im, const char *wvlt);
void reduce_lowres_LH(image_buffer *im, const char *wvlt, int ratio);
int configure_wvlt(int quality, char *wvlt);
void process_res_q8(image_buffer *im, short *res256, encode_state *enc);

void compress_s2(int quality, short *resIII, short *pr, char *wvlt,
	encode_state *enc, int ratio);

void encode_image(image_buffer *im,encode_state *enc, int ratio);

void imgbuf_init(image_buffer *im, int tile_power)
{
	im->fmt.tile_power = tile_power;
	im->fmt.tile_size = 1 << tile_power;
	im->fmt.end = im->fmt.tile_size * im->fmt.tile_size;
	im->fmt.half = im->fmt.end / 2;
}


MAYBE_STATIC
void residual_coding_q2(image_buffer *im, short *res256, int res_uv)
{
	int i, j, count;
	short *p;
	short *pr = im->im_process;
	int quad_size = im->fmt.end / 4;
	int step = im->fmt.tile_size / 2;
	int offset = step / 2;
	
	for (i=0,count=0;i<(quad_size>>1);i+=step)
	{
		p = &pr[i];
		for (j=0;j<(step>>1);j++,count++, p++)
		{
			short *q = &p[(quad_size>>1)];
			short *q1 = &q[offset];

			short d0 = p[0]-res256[count];
	// FIXME: p[1] outside buffer bounds for last scan/count pixel.
	// Currently added one 'residual' value pad.
			short d1 = p[1]-res256[count+1];

			short *pd = &p[step >> 1];

			if (d0 > 3 && d0 < 7)
			{
				if (d1 >2 && d1 <7)
				{
					if      (abs(pd[0])  < 8)
						{ pd[0]  = OFFS_C_CODE_12400;   count++;p++;j++; continue; }
					else if (abs(q[0])  < 8 )
						{  q[0]  = OFFS_C_CODE_12400;   count++;p++;j++; continue;}
					else if (abs(q1[0]) < 8 )
						{  q1[0] = OFFS_C_CODE_12400;   count++;p++;j++; continue;}
				}
			}
			else if (d0 < -3 && d0 >-7)
			{
				if (d1 < -2 && d1 > -8)
				{
					if      (abs(pd[0])  < 8)
						{ pd[0] = OFFS_C_CODE_12600;  count++;p++;j++; continue;}
					else if (abs(q[0])  < 8)
						{  q[0] = OFFS_C_CODE_12600;  count++;p++;j++; continue;}
					else if (abs(q1[0]) < 8)
						{ q1[0] = OFFS_C_CODE_12600;  count++;p++;j++; continue;}
				}
			}

			if (abs(d0) > res_uv) 
			{
				if (d0 > 0) {
					if      (abs(pd[0]) < 8 ) pd[0] = OFFS_C_CODE_12900;
					else if (abs( q[0]) < 8 )  q[0] = OFFS_C_CODE_12900; 
					else if (abs(q1[0]) < 8 ) q1[0] = OFFS_C_CODE_12900; 
				} else
				if (d0 == -5)
				{
					if (d1 <0)
					{
						if      (abs(pd[0])  < 8) pd[0] = OFFS_C_CODE_13000;
						else if (abs( q[0])  < 8)  q[0] = OFFS_C_CODE_13000; 
						else if (abs(q1[0]) < 8)  q1[0] = OFFS_C_CODE_13000; 
					}
					
				}
				else
				{
					if      (abs(pd[0]) < 8) pd[0] = OFFS_C_CODE_13000;
					else if (abs( q[0]) < 8)  q[0] = OFFS_C_CODE_13000; 
					else if (abs(q1[0]) < 8) q1[0] = OFFS_C_CODE_13000; 
				}
			}
		}
	}
}

MAYBE_STATIC
void tag_thresh_ranges(image_buffer *im, short *result)
{
	int cur;
	int i, j, count, scan;
	int s = im->fmt.tile_size;
	int halfn = s / 2;
	int tmp = 2 * halfn + 1;

	const short *pr = im->im_process;
	int quad_size = im->fmt.end / 4;

	int offset_code;
	int m;

// Tags all non LL pixels in 'result' according to threshold range
// and modulo8 values:
//
// CUR = '*'
//
//       'pr'                      'result'
// +---+---+---+---+              +---+---+
// |   | * |       |              |   | * |
// +---+---+       + <- quad_size +---+---+
// | * | * |       |              | * | * |
// +---+---+---+---+              +---+---+
// |       |       |
// +       +       +
// |       |       |
// +---+---+---+---+  <- (END) 
// ^.......^      <-  j = 0..halfn


	for (i = 0, count=0; i < im->fmt.half; i+=s, count += halfn)
	{
		for (scan = i, j = 0; j < halfn; j++,scan++) 
		{
			// FIXME: eliminate
			const short *p = &pr[scan];
			if (i >= quad_size || j >= halfn>>1)
			{
				// CUR
				cur = p[0];

				switch (cur) {
					case -7: case -6: case -5:
						TAG_PIXEL(*result, MARK_12000);
						break;
					case -4: case -3: case -2: case -1:
						break;
					case 2: case 3: case 4:
						if (scan>=tmp && (i+j) < (im->fmt.half - 2*halfn-1)
						 && (abs(p[-tmp])!=0 || abs(p[tmp])!=0))
							TAG_PIXEL(*result, MARK_12000);
						break;
					case 5: case 6: case 7:
						TAG_PIXEL(*result, MARK_16000);
						break;
					default:
						assert(cur > -128);
						if (cur < -7) {
							offset_code = MARK_16000;
							cur = -cur; m = 7;
						} else {
							offset_code = MARK_12000; m = 1;
						}
						if    (((cur) & 7) == m
						 ||    ((cur) & 7) == 0 )   TAG_PIXEL(*result, offset_code);

				}
			}
			result++;
		}
	}
}

inline
void fixup_pixel_offset(short *p, short *q)
{
	if        (*p >= MARK_16000) {
		UNTAG_PIXEL(*p, MARK_16000); q[0]++;
	} else if (*p >= MARK_12000) {
		UNTAG_PIXEL(*p, MARK_12000); q[0]--;
	}
}

// Can be done with LUT:

const short lut_d0[13] = {
	 0, 0, -1, -1,
	-1, 1,  2,  2,
     4, 4,  4,  4,
	 7
};

const short lut_comp[13] = {
	 0, 0,  0,  0,
	 0, 1,  2,  2,
     4, 4,  4,  4,
	 7
};

inline
int reduce_offset_comp_LL(int val)
{
	int a, s;

	s = val < 0 ? -1 : 1;
	a = abs(val);

	val -= s * lut_comp[(a >= 12 ? a = 12 : a)];

	return val;
}

MAYBE_STATIC
void offset_compensation_LL(image_buffer *im, short *res256)
{
//
//
//
//       'pr'          <<<<        'result'
// +---+---+---+---+              +---+---+
// |SE0|SE1|       |              |SC0|SC1|
// +---+---+       +  < quad_size +---+---+
// |SE2|SE3|       |              |SC2|SC3|
// +---+---+---+---+  <- (END)    +---+---+
// |       |       |                               
// +       +       +
// |       |       |
// +---+---+---+---+

// ^.......^      <-  e = 0..s2
// ^...^      (halfn)

	//                   (END) 
	int i, j;


	const short lut_d1[] = {
		-1, -1, -1, 0, 1, 2
	};

	int s = im->fmt.tile_size;
	int s2 = s / 2;

	int l;

	short *pr = im->im_process;
	short *r = res256;

	// Upper half:
	for (i = 0; i < im->fmt.half; i += s)
	{
		short *p = &pr[i];
		short *dst = &im->im_jpeg[i];

		for (j=0;j<s2;j++,r++,p++)
		{
			// SE0        SC0
			int d0, d1;

			d0 = p[0] - r[0];
			short comp;
			int sgn;
			if (d0 < 0) { d0 = -d0; sgn = -1; }
			else          {  sgn = 1; }

			l = (d0 > LAST_ELEMENT(lut_d0)) ? LAST_ELEMENT(lut_d0) : d0;
			comp = lut_d0[l];

			if (comp >= 0) {
				comp *= -sgn;
			} else {
				comp = 0;
				d1 = (p[1]-r[1]);

				d1 = reduce_offset_comp_LL(d1);

				if (r > res256) { // HACK
					d1+=(p[-1]-r[-1]);
				}

				short sgn_d1 = 1;
				l = d1;
				d1 *= sgn;

				if      (d0 == 4 && d1 >=1)    {  comp = 1; comp *= sgn; }
				else if (d0 == 3 && d1 >=0)    {  comp = 1; comp *= sgn; }
				else if (abs(d1)>=3)  // case
				{
					if (d0 == 2 && d1 >= 1)     {  comp = 1; comp *= sgn; }
					else {
						if (l < 0) { l = -l; sgn_d1 = -1; }
						l = l > LAST_ELEMENT(lut_d1) ? LAST_ELEMENT(lut_d1) : l;
						comp = lut_d1[l];
						comp *= sgn_d1;
					}
				}
				else comp = 0;

				comp = -comp;
			}
			dst[0] = r[0] + comp; p[0] += comp;
			dst++;
		}
	}
}

MAYBE_STATIC
void revert_compensate_offsets(image_buffer *im, short *res256)
{
	int i, j, line;

	int s = im->fmt.tile_size;
	int s2 = s >> 1;
	int s4 = s2 >> 1;

	int quad_size = im->fmt.end / 4;

	short *pr = im->im_process;

// Reverts the offset fixup and inc/dec corresponding process pixel values,
// accordingly
//
//       'pr'          <<<<        'result'
// +---+---+---+---+              +---+---+
// |   | * |       |              |   |SQ0|
// +---+---+       +  < quad_size +---+---+
// | * | * |       |              |SQ1|SQ2|
// +---+---+---+---+  <- (END)    +---+---+
// |       |       |                               
// +       +       +
// |       |       |
// +---+---+---+---+

// ^.......^      <-  j = 0..IM_DIM
// ^...^      (s4)

	// Upper half:
	line = 0;
	int shift = im->fmt.tile_power;
	// Step size for transposed j index:
	int tstep = 2 * (1 << shift);
	shift -= 1;

	for (i = 0; i < quad_size; i +=s)
	{
		short *p = &res256[line];
		short *q = &pr[(i >> shift) + s]; // FIXME: improve transposed access
		int k;
		line += s2;

		// We never tag the top left LL quadrant
#ifdef CONDITION_POSSIBLY_NEVER_MET
		// LL:
		for (j = 0;j < s4; j++, p++) {
			if        (*p > 14000) *p -= 16000;
			else if   (*p > 10000) *p -= 12000;
		}
#else
		p += s4;
#endif
		// Right half:
		for (j = s4, k = 0; j < s2; j++, p++, k += tstep) {
			fixup_pixel_offset(p, &q[k]);
		}
	}

	// Lower half:
	for (i = quad_size; i < (2*quad_size); i += s) {
		// Left half:
		int k;
		short *p = &res256[line];
		short *q = &pr[((i-quad_size)>>8) + 1];
		line += s2;
		for (j = 0, k = 0; j < s4; j++, p++, k+= tstep) {
			fixup_pixel_offset(p, &q[k]);
		}
		// Right half:

		q += s;
		for (j = s4, k = 0; j < s2; j++, p++, k+= tstep) {
			fixup_pixel_offset(p, &q[k]);
		}
	}
	offset_compensation_LL(im, res256);
}


// This initializes tree1 elements to the current
// 'scan' (cursor) value
//
// FIXME: Move position variables a and e into encode_state?
static int code_tree(short scan, int i, int j, int a, int e, encode_state *enc)
{
	if (scan > 255) {
		enc->exw_Y[e++] = i;	enc->exw_Y[e++]=j+128;
		scan -= 255; CLAMPH(scan, 255); enc->exw_Y[e++]= scan;
		enc->tree1[a]  = enc->tree1[a-1];
		enc->res_ch[a] = enc->tree1[a-1];
	} else if (scan < 0)  {
		enc->exw_Y[e++] = i;enc->exw_Y[e++]=j;
		CLAMPL(scan, -255);
		enc->exw_Y[e++] = -scan;
		enc->tree1[a] = enc->tree1[a-1];
		enc->res_ch[a]= enc->tree1[a-1];
	} else {
		enc->res_ch[a] = scan;
		enc->tree1[a] = scan & ~1;
	}
	return e;
}

inline void cond_modify0(short scan, int step, short *q)
{
	if (IS_ODD(scan)) {
		if (IS_ODD(q[0]) && IS_ODD(q[1])) {
			if (IS_ODD(q[step]) && !IS_ODD(q[(2*step)])) {
				if (!IS_TAG(q[0])) q[0]++;
			}
		}
	}
}

MAYBE_STATIC
void tree_compress_q3(image_buffer *im,  encode_state *enc)
{
	int i, j, scan;
	int stage, res;
	int a, e;
	int y;

	int im_size = im->fmt.end / 4;

	short *pr = im->im_process;
	int step = im->fmt.tile_size;

	res = mark_res_q3(im); // Mark all values in quad pixel packs which are odd
	enc->nhw_res4_len=res;
	enc->nhw_res4=(ResIndex *)calloc(enc->nhw_res4_len,sizeof(ResIndex));

	// First line
	res = 0;
	stage = 0;
	a = e = 0;

	// FIXME: Simplify complex loop construct

	// First pixel special treatment:
	//
	scan = pr[0];

#define CHECK_TAGGED_PIXEL(x) \
	if IS_TAG(scan) {     \
		if IS_TAG_RES(scan) {     \
			enc->nhw_res4[res++]= x;     \
			stage++;     \
			UNTAG_PIXEL(scan, MARK_24000); \
		} else \
			UNTAG_PIXEL(scan, MARK_16000); \
	}

	CHECK_TAGGED_PIXEL(1);

	CLAMP2(scan, 0, 255);
	enc->res_ch[a] = scan;
	enc->tree1[a++] = scan & ~1;
	pr[0] = 0;

	// First line:
	//
	short *p = &pr[1];

	for (j=1; j< (step>>2)-2 ;j++, p++)
	{
		scan = p[0];

		CHECK_TAGGED_PIXEL(j+1)
		else  {
			// South pixel (next line):
			short *q = &p[step];

			if (IS_ODD(scan) && IS_ODD(p[1])) {
				char cond = IS_ODD(q[0]) && IS_ODD(q[1]) && !IS_ODD(q[2]);

				if (IS_ODD(p[2])) {
					if (abs(scan-p[2])>1) p[1]++;
				} else if (cond) {
					if (!IS_TAG(q[0])) q[0]++;
				}

			}
		}
		e = code_tree(scan, 0, j, a, e, enc); a++;
	}

	// Last two columns:

	for (; j < (step>>2); j++, p++) {
		scan = p[0];

		CHECK_TAGGED_PIXEL(j+1)
		else  {
			short *q = &p[step];

			if (IS_ODD(scan) && IS_ODD(p[1])) {	
				char cond = IS_ODD(q[0]) && IS_ODD(q[1]) && !IS_ODD(q[2]);
				if (cond) {
					if (!IS_TAG(q[0])) q[0]++;
				}
			}
		}
		e = code_tree(scan, 0, j, a, e, enc); a++;
		p[0] = 0;
	}

	if (!stage) enc->nhw_res4[res++]  = 128;
	else        enc->nhw_res4[res-1] += 128; // Potential out of bounds
	stage=0;
	// Second line:
	for (i = step, y = 1; i < im_size; i += step, y++)
	{
		p = &pr[i];
		scan = p[0];

		// First row:
		j = 0;
		CHECK_TAGGED_PIXEL(j+1)
		else 
		if (i < (im_size-(3*step))) {
			cond_modify0(scan, step, &p[step]);
		}
		e = code_tree(scan, y, j, a, e, enc); a++;
		p[0] = 0;
		p++;

	// second row:
		for (j = 1; j< (step>>2)-2 ;j++, p++)
		{
			scan = p[0];

			CHECK_TAGGED_PIXEL(j+1)
			else {
				short *q = &p[step];

				if (IS_ODD(scan)
				 && IS_ODD(p[1])) {

					char cond = IS_ODD(q[0]) && IS_ODD(q[1]) && !IS_ODD(q[2]);

					if (IS_ODD(p[2])) {
						if (abs(scan-p[2])>1) p[1]++;
					} else if (i<(im_size-step-2) && cond ) {
						if (!IS_TAG(q[0])) q[0]++;
					}

				} else

				if (i >= step && i < (im_size-(3*step))) {
					cond_modify0(scan, step, q);
				}
			} 
			e = code_tree(scan, y, j, a, e, enc); a++;
			p[0] = 0;
		}

		for (; j < (step>>2); j++, p++) {
			scan = p[0];

			CHECK_TAGGED_PIXEL(j+1)
			else  {
			// FIXME: eliminate loop variable check
				short *q = &p[step];

				if (IS_ODD(scan)
				 && IS_ODD(p[1])) {
					char cond = IS_ODD(q[0]) && IS_ODD(q[1]) && !IS_ODD(q[2]);

					if (i<(im_size-step-2) && cond) { // FIXME
						if (!IS_TAG(q[0])) q[0]++;
					}
				} else
				// FIXME (i)
				if (i < (im_size-(3*step))) { // FIXME
					if (IS_ODD(scan)) {
						if (IS_ODD(q[0]) && IS_ODD(q[1])) {
							if (IS_ODD(q[step]) && !IS_ODD(q[(2*step)])) {
								if (!IS_TAG(q[0])) q[0]++;
							}
						}
					}
				}
			}
			e = code_tree(scan, y, j, a, e, enc); a++;
			p[0] = 0;
		}
		if (!stage) enc->nhw_res4[res++] =  128;
		else        enc->nhw_res4[res-1] += 128;
		stage = 0;
	}

	enc->exw_Y_end = e;
}

void tree_compress_q(image_buffer *im, encode_state *enc)
{
	int i, j;
	int a, e;
	int y;

	short *pr = im->im_process;
	int dim = im->fmt.tile_size;
	int im_size = im->fmt.end / 4;

	a = 0;
	e = 0; // Counter

	// First pixel:
	enc->res_ch[0] = pr[a];
	enc->tree1[0]  = pr[a] & ~1;    a++;

	// Collect all coordinates of marked pixels in LL
	// in the exw_Y array:

	short *p = &pr[1];
	for (j=1;j<(dim>>2);j++, p++) {
		e = code_tree(p[0], 0, j, a++, e, enc);
		p[0]=0;
	}
	y = 1;
	for (i=dim;i<im_size;i+=dim) {
		p = &pr[i];
		for (j=0;j<(dim>>2);j++, p++) {
			e = code_tree(p[0], y, j, a++, e, enc);
			p[0]=0;
		}
		y++;
	}
	enc->exw_Y_end = e;
}

MAYBE_STATIC
void tree_compress(image_buffer *im, encode_state *enc)
{
	int quality = im->setup->quality_setting;

	if (quality > LOW3)  {
		tree_compress_q3(im,  enc);
	} else {
		tree_compress_q(im, enc);
	}
}

MAYBE_STATIC
int mark_res_q3(image_buffer *im)
{
	int i, j;
	int res;
	int stage;

	// Tag all values whose condition match:
	// * Quad-pack has odd values
	// * Absolute difference of first and last value is > 1

	// *LL*  HL   ..   ..  <- im_size
	//  LH   HH   ..   ..  
	//  ..   ..   ..   ..
	//  ..   ..   ..   ..


	short *pr = im->im_process;
	int im_size = im->fmt.end / 4;
	int step = im->fmt.tile_size;
	for (i=0,res=0,stage=0; i < im_size; i+=step)
	{
		short *p = &pr[i];

		for (j=0;j<((step/4)-3);j++,p++)
		{
			if (IS_ODD(p[0]) &&
			    IS_ODD(p[1]) &&
			    IS_ODD(p[2]) &&
			    IS_ODD(p[3]) &&
			    abs(p[0]-p[3])>1)
			{
				assert(p[0] > -128);
				assert(p[1] > -128);
				assert(p[2] > -128);
				assert(p[3] > -128);

				TAG_PIXEL(p[0], MARK_24000);
				TAG_PIXEL(p[1], MARK_16000);
				TAG_PIXEL(p[2], MARK_16000);
				TAG_PIXEL(p[3], MARK_16000);
				res++;stage++;j+=3;p+=3;


			}
		}

		if (!stage) res++;
		stage=0;
	}
	return res;
}

void SWAPOUT_FUNCTION(process_hires_q8)(image_buffer *im,
	ResIndex *highres, short *res256, encode_state *enc)
{
	int i, j, stage;
	int Y;
	ResIndex *ch_comp;
	unsigned char *scan_run;
	unsigned char *nhw_res1I_word;
	int res;
	int count, e;

	int quad_size = im->fmt.end / 4;
	int im_dim = im->fmt.tile_size / 2;

	count = enc->res1.word_len;
	count = (count + 7) & ~7; // Round up to next multiple of 8 for padding

	nhw_res1I_word = (unsigned char*) malloc(count * sizeof(char));
	
	for (i=0,count=0,e=0;i<quad_size;i+=im_dim)
	{
		short *p = &res256[i];

		for (j=0;j<im_dim;j++, p++)
		{
			if (j==(im_dim-2)) // FIXME: move outside loop
			{
				p[0]=0; p[1]=0;
				highres[count++]=(im_dim-2);j++; 
			} else {
#ifdef USE_OPCODES
				short r = p[0];
				if (IS_OPCODE(r)) {
					if (r & RES1) {
						highres[count++]=j;
						nhw_res1I_word[e++] = ((r & RI) >> RI_SHFT);
					}
				}
#else
				switch (p[0]) {
					case OP_R11_N:
						highres[count++]=j;
						p[0]=0; nhw_res1I_word[e++]=1;
						break;
					case OP_R10_P: 
						highres[count++]=j;
						p[0]=0; nhw_res1I_word[e++]=0;
						break;
					case OP_R3R0P: 
						highres[count++]=j;
						p[0]=OP_R30_P; nhw_res1I_word[e++]=0;
						break;
					case OP_R3R1N: 
						highres[count++]=j;
						p[0]=OP_R31_N; nhw_res1I_word[e++]=1;
						break;
					case OP_R5R1N: 
						highres[count++]=j;
						p[0]=OP_R51_N; nhw_res1I_word[e++]=1;
						break;
					case OP_R5R1P: 
						highres[count++]=j;
						p[0]=OP_R50_P; nhw_res1I_word[e++]=0;
				}
#endif
			}
		}
	}

	scan_run = &nhw_res1I_word[e];

	// Pad remaining with zeros:
	for (i = e % 8; i < 8; i++) *scan_run++ = 0;

	ch_comp=(ResIndex *)malloc(count*sizeof(ResIndex));
	memcpy(ch_comp,highres,count*sizeof(ResIndex));

	for (i=1,res=1;i<count-1;i++)
	{
		if (ch_comp[i]==(im_dim-2))
		{
			if (ch_comp[i-1]!=(im_dim-2) && ch_comp[i+1]!=(im_dim-2))
			{
				if (ch_comp[i-1]<=ch_comp[i+1]) highres[res++]=ch_comp[i];
			}
			else highres[res++]=ch_comp[i];
		}
		else highres[res++]=ch_comp[i];
	}

	highres[res++]=ch_comp[count-1];
	free(ch_comp);

	enc->res1.len = res;
	enc->res1.word_len = e;

	enc->res1.res=(ResIndex *) malloc((enc->res1.len) * sizeof(ResIndex));

	for (i=0;i<enc->res1.len;i++) enc->res1.res[i]=highres[i];

	res = (res + 7) & ~7; // Round up to next multiple of 8 for padding

	scan_run=(unsigned char*) malloc(res * sizeof(char));

	for (i=0;i<enc->res1.len;i++) scan_run[i]=enc->res1.res[i]>>1;

	highres[0]=scan_run[0];

	for (i=1,count=1;i<enc->res1.len-1;i++)
	{
		int d = scan_run[i]-scan_run[i-1];
		if (d>=0 && d<8)
		{
			int d1 = scan_run[i+1]-scan_run[i];
			if (d1 >= 0 && d1 < 16) {
				highres[count++]= 128 + (d << 4) + d1;
				i++;
			}
			else highres[count++]=scan_run[i];
		}
		else highres[count++]=scan_run[i];
	}

	for (i=0,stage=0;i<enc->res1.len;i++) 
	{
		if (enc->res1.res[i]!=254) scan_run[stage++]=enc->res1.res[i];
	}

	res = (stage + 7) & ~7; // Round up to next mul 8 and zero-pad:
	for (i = stage; i < res; i++) scan_run[i]=0;

	Y = res >> 3;
	enc->res1.bit_len = Y;
	enc->res1.res_bit = (unsigned char*) malloc(Y * sizeof(char));
	copy_bitplane0(scan_run, Y, enc->res1.res_bit);
	free(scan_run); // no longer needed

	enc->res1.len=count;

	Y = enc->res1.word_len + 7;
	Y = Y >> 3;
	enc->res1.res_word = (unsigned char*) malloc((Y+1)*sizeof(char));
	copy_bitplane0(nhw_res1I_word, Y, enc->res1.res_word);
	enc->res1.word_len = Y;

	for (i = 0; i < count; i++) enc->res1.res[i] = highres[i];

	free(nhw_res1I_word);
}


MAYBE_STATIC
void SWAPOUT_FUNCTION(process_res3_q1)(image_buffer *im,
	ResIndex *highres, short *res256, encode_state *enc)
{
	int i, j, scan, e, Y, count;
	int res;
	ResIndex *ch_comp;
	ResIndex *scan_run;
	unsigned char *nhw_res3I_word;
	int res3I_word_len;
	ResIndex *sp;

	int quad_size = im->fmt.end / 4;
	int step = im->fmt.tile_size / 2;

	e = enc->res3.word_len;
	e = (e + 7) & ~7; // Round up to next multiple of 8 for padding

	nhw_res3I_word = (unsigned char*) calloc(e, sizeof(char));
	enc->res3.word_len = e;

	e = 0;

	for (i=0,count=0;i<quad_size;i+=step)
	{
		for (scan=i,j=0;j<step-2;j++,scan++)
		{
#ifdef USE_OPCODES
			short r = res256[scan];
			if (IS_OPCODE(r)) {
				if (r & RES3) {
					highres[count++]=j;
					nhw_res3I_word[e++] = ((r & RI) >> RI_SHFT);
					res256[scan] = 0;
				}

			}
#else
			switch (res256[scan]) {
				case OP_R30_P:
					highres[count++]=j;
					res256[scan]=0;nhw_res3I_word[e++]=0;
					break;
				case OP_R31_N:
					highres[count++]=j;
					res256[scan]=0;nhw_res3I_word[e++]=1;
					break;
				case OP_R32_P:
					highres[count++]=j;
					res256[scan]=0;nhw_res3I_word[e++]=2;
					break;
				case OP_R33_N:
					highres[count++]=j;
					res256[scan]=0;nhw_res3I_word[e++]=3;
					break;
			}
#endif

		}
		res256[scan]=0;
		res256[scan+1]=0;
		highres[count++]=(step-2);
	}

	// Store effective length:
	res3I_word_len = e;

	ch_comp=(ResIndex *)malloc(count*sizeof(ResIndex));
	memcpy(ch_comp,highres,count*sizeof(ResIndex));

	for (i=1,res=1;i<count-1;i++)
	{
		if (ch_comp[i]==(step-2))
		{
			if (ch_comp[i-1]!=(step-2) && ch_comp[i+1]!=(step-2))
			{
				if (ch_comp[i-1]<=ch_comp[i+1]) highres[res++]=ch_comp[i];
			}
			else highres[res++]=ch_comp[i];
		}
		else highres[res++]=ch_comp[i];
	}

	highres[res++]=ch_comp[count-1];
	free(ch_comp);

	enc->res3.len=res;

	enc->res3.res=(ResIndex *)calloc((enc->res3.len),sizeof(ResIndex));

	for (i=0;i<enc->res3.len;i++) enc->res3.res[i]=highres[i];

	scan_run= (ResIndex *) malloc((enc->res3.len+8) * sizeof(ResIndex));

	for (i=0;i<enc->res3.len;i++) scan_run[i]=enc->res3.res[i]>>1;

	highres[0]=scan_run[0];

	for (i=1,count=1;i<enc->res3.len-1;i++)
	{
		if ((scan_run[i]-scan_run[i-1])>=0 && (scan_run[i]-scan_run[i-1])<8)
		{
			if ((scan_run[i+1]-scan_run[i])>=0 && (scan_run[i+1]-scan_run[i])<16)
			{
				highres[count++]= 128
					+ ((scan_run[i]-scan_run[i-1])<<4)
					+ scan_run[i+1]-scan_run[i];
				i++;
			}
			else highres[count++]=scan_run[i];
		}
		else highres[count++]=scan_run[i];
	}

	int c = 0; // Count all != 245:

	sp = scan_run;

	for (i=0; i<enc->res3.len; i++) {
		if (enc->res3.res[i] != 254) { *sp++ = enc->res3.res[i]; c++; }
	}

	// Pad remaining:
	for (i = c % 8; i < 8; i++) *sp++ = 0;

	c = (c + 7) & ~7; // Round up to next multiple of 8 for padding

	Y = (c >> 3); // Number of bytes

	enc->res3.bit_len = Y;

	enc->res3.res_bit=(unsigned char*)calloc(enc->res3.bit_len,sizeof(char));

	copy_bitplane0(scan_run, Y, enc->res3.res_bit);

	enc->res3.len=count;

	free(scan_run);

	// Target word residuals, bitplane-encoded:
	enc->res3.res_word=(unsigned char*) malloc((enc->res3.bit_len * 2) * sizeof(char));

	sp = nhw_res3I_word;

	Y = res3I_word_len;
	Y = (Y + 7) & ~7; // Round up to next multiple of 8 for padding
	Y >>= 2;

	// assert((2*Y + 2) * 4 <= enc->res3.word_len );
		
	for (i=0; i < Y;i++)
	{

#define _SLICE_BITS_POS(val, mask, shift) (((val) & mask) << shift)

		enc->res3.res_word[i] = _SLICE_BITS_POS(sp[0], 0x3, 6) |
		                        _SLICE_BITS_POS(sp[1], 0x3, 4) |
		                        _SLICE_BITS_POS(sp[2], 0x3, 2) |
		                        _SLICE_BITS_POS(sp[3], 0x3, 0);

		sp += 4;

	}

	enc->res3.word_len = Y;

	for (i=0; i<enc->res3.len; i++) enc->res3.res[i] = highres[i];

	free(nhw_res3I_word);
}


MAYBE_STATIC
void SWAPOUT_FUNCTION(process_res5_q1)(image_buffer *im,
	ResIndex *highres, short *res256, encode_state *enc)
{
	int i, j, scan, e, Y, stage, count;
	int res;
	ResIndex *ch_comp;
	unsigned char *nhw_res5I_word;
	ResIndex *scan_run;

	int quad_size = im->fmt.end / 4;
	int step = im->fmt.tile_size / 2;

	nhw_res5I_word=(unsigned char*)calloc(enc->res5.word_len,sizeof(char));

	for (i=0,count=0,res=0,e=0;i<quad_size;i+=step)
	{
		for (scan=i,j=0;j<step-2;j++,scan++)
		{
#ifdef USE_OPCODES
			int r = res256[scan];
			if (IS_OPCODE(r)) {
				if (r & RES5) {
					highres[count++]=j;
					nhw_res5I_word[e++] = ((r & RI) >> RI_SHFT);
					res256[scan]=0;
				}
			}
#else
			if (res256[scan]==OP_R51_N) {
				highres[count++]=j;
				res256[scan]=0;nhw_res5I_word[e++]=1;
			} else if (res256[scan]==OP_R50_P) 
			{
				highres[count++]=j;
				res256[scan]=0;nhw_res5I_word[e++]=0;
			}
#endif

		}

		res256[scan]=0;
		res256[scan+1]=0;
		highres[count++]=j++;
		scan++;
	}

	ch_comp = (ResIndex *)malloc(count*sizeof(ResIndex));
	memcpy(ch_comp,highres,count*sizeof(char));

	for (i=1,res=1;i<count-1;i++)
	{
		if (ch_comp[i]==(step-2))
		{
			if (ch_comp[i-1]!=(step-2) && ch_comp[i+1]!=(step-2))
			{
				if (ch_comp[i-1]<=ch_comp[i+1]) highres[res++]=ch_comp[i];
			}
			else highres[res++]=ch_comp[i];
		}
		else highres[res++]=ch_comp[i];
	}

	highres[res++]=ch_comp[count-1];
	free(ch_comp);

	enc->res5.len=res;
	enc->res5.word_len=e;
	enc->res5.res=(ResIndex *)calloc((enc->res5.len),
		sizeof(ResIndex));

	for (i=0;i<enc->res5.len;i++) enc->res5.res[i]=highres[i];

	scan_run=(ResIndex *)calloc((enc->res5.len+8),
		sizeof(ResIndex));

	for (i=0;i<enc->res5.len;i++) scan_run[i]=enc->res5.res[i]>>1;

	highres[0]=scan_run[0];

	for (i=1,count=1;i<enc->res5.len-1;i++)
	{
		if ((scan_run[i]-scan_run[i-1])>=0 && (scan_run[i]-scan_run[i-1])<8)
		{
			if ((scan_run[i+1]-scan_run[i])>=0 && (scan_run[i+1]-scan_run[i])<16)
			{
				highres[count++]=128+((scan_run[i]-scan_run[i-1])<<4)+scan_run[i+1]-scan_run[i];
				i++;
			}
			else highres[count++]=scan_run[i];
		}
		else highres[count++]=scan_run[i];
	}

	for (i=0,stage=0;i<enc->res5.len;i++) 
	{
		if (enc->res5.res[i]!=254) scan_run[stage++]=enc->res5.res[i];
	}

	for (i=stage;i<stage+8;i++) scan_run[i]=0;

	Y = stage>>3;
	enc->res5.bit_len = Y+1;
	enc->res5.res_bit=(unsigned char*)calloc(enc->res5.bit_len,
		sizeof(char));
	copy_bitplane0(scan_run, Y + 1, enc->res5.res_bit);
	enc->res5.len=count;

	Y = enc->res5.word_len>>3;
	free(scan_run);
	enc->res5.res_word = (unsigned char*) calloc((enc->res5.bit_len<<1),
		sizeof(char));
	copy_bitplane0(nhw_res5I_word, Y + 1, enc->res5.res_word);
	enc->res5.word_len = Y+1;

	for (i=0;i<count;i++) enc->res5.res[i]=highres[i];
	
	free(nhw_res5I_word);
}

short s_comp_lut_w0[12] = {
	-5, 5, -3, 3, -4, 4, -2, 2, -9, 9, -8, 8
};

MAYBE_STATIC
void SWAPOUT_FUNCTION(process_res_hq)(image_buffer *im, const short *res256)
{
	int i, j;
	int k = 0;
	short *w;

	short *wl_first_order = im->im_wavelet_first_order;
	int quad_size = im->fmt.end / 4;
	int step = im->fmt.tile_size / 2;

	for (i=0;i<quad_size;i+=step, res256 += 2)
	{
		// Walk the transposed way:
		w = &wl_first_order[k++];
		for (j=0; j<step-2; j++, w += step)
		{
			short r = *res256++;

#ifdef USE_OPCODES

// Sign extend value in bit field:
#define _EXTENDBF(val, mask, msb)   \
	( ((val & mask) ^ (1 << (msb)) ) - (1 << (msb))) >> mask##_SHFT

			if (IS_OPCODE(r)) {
				short tmp = s_comp_lut_w0[(r & ADD_W0) >> ADD_W0_SHFT];
				// Probably a LUT is more efficient
				short tmp2 = _EXTENDBF(r, ADD_W1, ADD_W1_SHFT+2);
				w[0] += tmp;
				w[1] += tmp2;
				switch (r & ADD_W2) {
					case ADD_W2_PLUS2:  w[2] += 2; break;
					case ADD_W2_MINUS2: w[2] -= 2; break;
				}
			}
#else
			if (r !=0) {
				switch (r) {
					case OP_R11_N: w[0] += -5;                      break;
					case OP_R10_P: w[0] +=  5;                      break;
					case OP_R51_N: w[0] += -3;                      break;
					case OP_R50_P: w[0] +=  3;                      break;
					case OP_R31_N: w[0] += -4; w[1] +=-3;           break;
					case OP_R30_P: w[0] +=  4; w[1] += 3;           break;
					case OP_R32_P: w[0] +=  2; w[1] += 2; w[2]+=2;  break;
					case OP_R33_N: w[0] += -2; w[1] +=-2; w[2]-=2;  break;
					case OP_R3R0P: w[0] +=  9; w[1] += 3;           break;
					case OP_R3R1N: w[0] += -9; w[1] +=-3;           break;
					case OP_R5R1N: w[0] += -8;                      break;
					case OP_R5R1P: w[0] +=  8;                      break;
				}
			}
#endif
		}
	}
}

void copy_thresholds(short *process, const short *resIII, int size, int step)
{
	int i, j;

	int im_dim = step / 2;
	const short *r = resIII;

	for (i=0; i < size; i += step) {
		short *p = &process[i];
		// LL:
		for (j = 0; j < im_dim >> 1; j++, r++) { 
// Possibly this condition is never met, because we never tag the resIII array...
			if (*r > 8000) {
				*p++ = *r; assert(0);
			} else {
				*p++ = 0;
			}
		}

		// LH:
		for (j = im_dim >> 1; j < im_dim; j++) { 
			*p++ = *r++;
		}
	}

	for (i = size; i < (2*size); i += step) {
		short *p = &process[i];
		// HL and HH
		for (j = 0; j < im_dim; j++) { 
			*p++ = *r++;
		}
	}
}

static const short y_threshold_table[][3] = {
	{ 9, 9, 11 },   // LOW3
	{ 8, 9, 11 },   // LOW2
	{ 8, 9, 11 },   // LOW1
	{ 8, 9, 11 },   // NORM
	{ 8, 9, 11 },   // HIGH1
	{ 8, 9, 11 },   // HIGH2
	{ 8, 4, 8 },    // HIGH3
};

const short *lookup_ywlthreshold(int quality)
{
	quality -= LOW3;
	if (quality < 0) quality = 0;
	else if (quality > 8) quality = 8;

	return y_threshold_table[quality];
}

void SWAPOUT_FUNCTION(encode_y)(image_buffer *im, encode_state *enc, int ratio)
{
	int quality = im->setup->quality_setting;
	int res;
	int end_transform;
	short *res256, *resIII;
	ResIndex *highres;
	short *pr;
	char wvlt[7];
	pr=(short*)im->im_process;

	// First order quadrant size:
	int quad_size = im->fmt.end / 4;

	int n = im->fmt.tile_size;
	int halfn = n / 2;

	end_transform=0;
	// Unused:
	// wavelet_order=im->setup->wvlts_order;
	//for (stage=0;stage<wavelet_order;stage++) wavelet_analysis(im,(2*IM_DIM)>>stage,end_transform++,1); 

	wavelet_analysis(im, n,end_transform++,1);

	// Add some head room for padding (PAD is initialized to 0!)
	res256 = (short*) calloc((quad_size + n), sizeof(short));
	resIII = (short*) malloc(quad_size*sizeof(short));

	// copy upper left LL1 tile into res256 array
	copy_from_quadrant(res256, im->im_jpeg, n, n);

	im->setup->RES_HIGH=0;

	wavelet_analysis(im, n >> 1, end_transform,1);

	if (quality > LOW14) // Better quality than LOW14?
	{
		tag_thresh_ranges(im, res256);
		offsetY_recons256(im,enc,ratio,1);
		wavelet_synthesis(im, n>>1, end_transform-1,1);
		// Modifies all 3 buffers:
		revert_compensate_offsets(im, res256);
		wavelet_analysis(im, n >> 1, end_transform,1);
	}
	
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
	
	copy_from_quadrant(resIII, pr, n, n);
	
	enc->tree1=(unsigned char*) calloc(((96*halfn)+4),sizeof(char));
	enc->exw_Y=(unsigned char*) malloc(32*halfn*sizeof(short));
	
	enc->res_ch=(unsigned char*)calloc((quad_size>>2),sizeof(char));

	tree_compress(im, enc);

	Y_highres_compression(im, enc);

	free(enc->res_ch);

	copy_to_quadrant(pr, resIII, n, n);
	
	if (quality > LOW8) { // Better than LOW8?

		offsetY_recons256(im, enc, ratio, 0);

		wavelet_synthesis(im, n>>1, end_transform-1,1);

		if (quality>HIGH1) {
			im->im_wavelet_first_order=(short*)malloc(quad_size*sizeof(short));
			copy_from_quadrant(im->im_wavelet_first_order, im->im_jpeg, n, n);
		}
	}

	// After last call of offsetY_recons256() we can free:
	free(enc->highres_mem);
	
	// Release original data buffer
	//
	free(im->im_jpeg); // XXX

////////////////////////////////////////////////////////////////////////////

	reduce_generic(im, resIII, wvlt, enc, ratio);
	
	if (quality > LOW8) {
		preprocess_res_q8(im, res256, enc);

		highres=(ResIndex *)calloc(((96*halfn)+1),sizeof(ResIndex));

		if (quality > HIGH1) {
			process_res_hq(im, res256);
		}
	
		process_hires_q8(im, highres, res256, enc);

		// Further residual processing:
		if (quality>=LOW1) {
			process_res3_q1(im, highres, res256, enc);
			if (quality>=HIGH1)
			{
				process_res5_q1(im, highres, res256, enc);
			}
		}
	
		free(highres);
	}

	free(res256);

	copy_thresholds(pr, resIII, im->fmt.end / 4, n);

	free(resIII);
	quant_ac_final(im, ratio, lookup_ywlthreshold(quality));
	offsetY(im,enc,ratio);

	if (quality>HIGH1) {
		im->im_wavelet_band=(short*)calloc(quad_size,sizeof(short));
		// These two functions use .im_wavelet_band:
		im_recons_wavelet_band(im);
		wavelet_synthesis_high_quality_settings(im,enc);
		free(im->im_wavelet_first_order);
		free(im->im_wavelet_band);
		free(im->im_quality_setting);
	}
}

void process_uv_exw(image_buffer *im, encode_state *enc, int a)
{
	int Y;
	short scan;
	int i, j;
	int s2 = im->fmt.tile_size / 2;
	int imquart = im->fmt.end / 16;

	for (i=0; i<(imquart);i+=(s2))
	{
		short *p = &im->im_process[i];
		for (j=0;j<((s2)>>2);j++,p++)
		{
			scan = p[0];

			if (scan>255 && (j>0 || i>0)) 
			{
				enc->exw_Y[enc->exw_Y_end++]=(i>>8);
				enc->exw_Y[enc->exw_Y_end++]=j+128;
				Y=scan-255;if (Y>255) Y=255;
				enc->exw_Y[enc->exw_Y_end++]=Y;
				enc->tree1[a]=enc->tree1[a-1];
				a++; p[0]=0;
			}
			else if (scan<0 && (j>0 || i>0)) 
			{
				enc->exw_Y[enc->exw_Y_end++]=(i>>8);
				enc->exw_Y[enc->exw_Y_end++]=j;
				if (scan<-255) scan=-255;
				enc->exw_Y[enc->exw_Y_end++]=-scan;
				enc->tree1[a]=enc->tree1[a-1];
				a++; p[0]=0;
			}
			else 
			{
				if (scan>255) { scan=255; }
				else if (scan<0) { scan=0; }
				enc->tree1[a++]=scan& ~1;
				p[0]=0;
			}
		}
	}
}


MAYBE_STATIC
void encode_uv(image_buffer *im,
	encode_state *enc, int ratio, int res_uv, int uv)
{
	int end_transform;
	unsigned char *scan_run;
	short *res256;
	short *resIII;
	int offset;

	int s2         = im->fmt.tile_size / 2;
	int quad_size  = im->fmt.end / 4; // Size of U/V component buffer
	int imquart    = im->fmt.end / 16;
	int imhalf     = im->fmt.end / 8;

	int quality = im->setup->quality_setting;

	if (uv) {
		copy_buffer8bit(im->im_jpeg, im->im_bufferV, quad_size);
	} else {
		copy_buffer8bit(im->im_jpeg, im->im_bufferU, quad_size);
	}
	
	if (quality <= LOW6) pre_processing_UV(im);

	end_transform=0; im->setup->RES_HIGH=0;

	wavelet_analysis(im, s2, end_transform++, 0);

	// Add one pad unit, see residual_coding_q2()::FIXME
	res256 = (short*) malloc((imquart + 1)*sizeof(short));
	res256[imquart] = 0; // Pad to 0

	resIII = (short*) malloc(imquart*sizeof(short));

	copy_from_quadrant(res256, im->im_jpeg, s2, s2);
	
	if (quality <= LOW4) {
		reduce_uv_q4(im, ratio);
	}

	wavelet_analysis(im, s2 / 2, end_transform,0); 

	offsetUV_recons256(im, ratio, 1);

	wavelet_synthesis(im, s2 / 2, end_transform-1,0); 

	int t0, t1;

	if (uv) {
		t0 = 1; t1 = -1;
	} else {
		t0 = 0; t1 = 0;
	}

	lowres_uv_compensate(im->im_jpeg, im->im_process, res256, imhalf, s2,
		t0, t1);

	wavelet_analysis(im, s2 / 2, end_transform, 0);
	copy_from_quadrant(resIII, im->im_process, s2, s2);
	offsetUV_recons256(im,ratio,0);
	wavelet_synthesis(im, s2 / 2, end_transform-1,0);

	if (quality >= LOW2) { 
		residual_coding_q2(im, res256, res_uv);
	}

	copy_to_quadrant(im->im_process, resIII, s2, s2);

	if (quality <= LOW9) {
		reduce_uv_q9(im);
	}

	if (uv) {
		offset = imquart+(imquart>>2);
	} else {
		offset = imquart;
	}

	// Insert 'separator' ?
	enc->exw_Y[enc->exw_Y_end++]=0;
	enc->exw_Y[enc->exw_Y_end++]=0;

	process_uv_exw(im, enc, offset);

	if (quality>LOW5) 
	{
		if (uv) {
			enc->res_V_64=(unsigned char*)calloc((s2<<1),sizeof(char));
			scan_run=(unsigned char*)enc->res_V_64;
		} else {		
			enc->res_U_64=(unsigned char*)calloc((s2<<1),sizeof(char));
			scan_run=(unsigned char*)enc->res_U_64;
		}

		extract_bitplane(scan_run, &enc->tree1[offset], (16*s2));
	}

	offsetUV(im,enc,ratio);

	if (uv) 
		copy_uv_chunks(&im->im_nhw[im->fmt.end+1], im->im_process, s2);
	else
		copy_uv_chunks(&im->im_nhw[im->fmt.end], im->im_process, s2);

	free(res256);
	free(resIII);
}

void SWAPOUT_FUNCTION(encode_image)(image_buffer *im,encode_state *enc, int ratio)
{
	int res_uv;

	im->im_process=(short*)calloc(im->fmt.end,sizeof(short));

	//if (im->setup->quality_setting<=LOW6) block_variance_avg(im);

	if (im->setup->quality_setting<HIGH2) 
	{
		pre_processing(im);
	}

	encode_y(im, enc, ratio);

	im->im_nhw=(unsigned char*)calloc(im->fmt.end * 3,sizeof(char));

	// This fills the Y part of the im_nhw array
	scan_run_code(im, enc);

	free(im->im_process);
	
////////////////////////////////////////////////////////////////////////////	
	// Size of a U and V buffer for YUV420 format

	int quad_size = im->fmt.end / 4;

	im->im_process = (short*) calloc(quad_size,sizeof(short)); // {
	im->im_jpeg    = (short*) calloc(quad_size,sizeof(short)); // Work buffer {

	if (im->setup->quality_setting > LOW3) res_uv=4;
	else                                   res_uv=5;


	// Each of the encode_uv fills the remainder of the im_nhw array
	// with the interleaved U/V data
	encode_uv(im, enc, ratio, res_uv, 0);
	free(im->im_bufferU); // Previously reserved buffer XXX

	encode_uv(im, enc, ratio, res_uv, 1);
	free(im->im_bufferV); // Previously reserved buffer XXX

	free(im->im_jpeg); // Free Work buffer }

	free(im->im_process); // }

	// This frees previously allocated enc->tree[1,2]:
	highres_compression(im, enc);
	free(enc->tree1);
	free(enc->highres_comp);

	// This creates new enc->tree structures
	wavlts2packet(im,enc);

	free(im->im_nhw);

}

int write_compressed_file(image_buffer *im,encode_state *enc, const char *outfilename)
{
	FILE *compressed;

	int q = im->setup->quality_setting;

	compressed = fopen(outfilename,"wb");
	if( NULL == compressed )
	{
		printf("Failed to create compressed .nhw file %s\n", outfilename);
		return (-1);
	}

	im->setup->RES_HIGH+=im->setup->wavelet_type;

	fwrite(&im->setup->RES_HIGH,1,1,compressed);
	fwrite(&im->setup->quality_setting,1,1,compressed);
	fwrite(&enc->size_tree1,2,1,compressed);
	fwrite(&enc->size_tree2,2,1,compressed);
	fwrite(&enc->size_data1,4,1,compressed);
	fwrite(&enc->size_data2,4,1,compressed);
	fwrite(&enc->tree_end,2,1,compressed);
	fwrite(&enc->exw_Y_end,2,1,compressed);
	if (q > LOW8) fwrite(&enc->res1.len,2,1,compressed);
	
	if (q >= LOW1)
	{
		fwrite(&enc->res3.len,2,1,compressed);
		fwrite(&enc->res3.bit_len,2,1,compressed);
	}
	if (q > LOW3)
	{
		fwrite(&enc->nhw_res4_len,2,1,compressed);
	}
	
	if (q > LOW8) fwrite(&enc->res1.bit_len,2,1,compressed);

	if (q >= HIGH1)
	{
		fwrite(&enc->res5.len,2,1,compressed);
		fwrite(&enc->res5.bit_len,2,1,compressed);
	}

	if (q > HIGH1)
	{
		fwrite(&enc->nhw_res6_len,4,1,compressed);
		fwrite(&enc->nhw_res6_bit_len,2,1,compressed);
		fwrite(&enc->nhw_char_res1_len,2,1,compressed);
	}

	if (q > HIGH2)
	{
		fwrite(&enc->qsetting3_len,2,1,compressed);
	}

	fwrite(&enc->nhw_select1,2,1,compressed);
	fwrite(&enc->nhw_select2,2,1,compressed);
	
	if (q > LOW5)
	{
		fwrite(&enc->highres_comp_len,2,1,compressed);
	}
	
	fwrite(&enc->end_ch_res,2,1,compressed);
	fwrite(enc->tree1,enc->size_tree1,1,compressed);
	fwrite(enc->tree2,enc->size_tree2,1,compressed);
	fwrite(enc->exw_Y,enc->exw_Y_end,1,compressed);
	
	if (q > LOW8)
	{
		fwrite(enc->res1.res,enc->res1.len,1,compressed);
		fwrite(enc->res1.res_bit,enc->res1.bit_len,1,compressed);
		fwrite(enc->res1.res_word,enc->res1.word_len,1,compressed);
	}
	
	if (q > LOW3)
	{
		fwrite(enc->nhw_res4,enc->nhw_res4_len,1,compressed);
	}
	
	if (q >=LOW1)
	{
		fwrite(enc->res3.res,enc->res3.len,1,compressed);
		fwrite(enc->res3.res_bit,enc->res3.bit_len,1,compressed);
		fwrite(enc->res3.res_word,enc->res3.word_len,1,compressed);
	}
	
	if (q >= HIGH1)
	{
		fwrite(enc->res5.res,enc->res5.len,1,compressed);
		fwrite(enc->res5.res_bit,enc->res5.bit_len,1,compressed);
		fwrite(enc->res5.res_word,enc->res5.word_len,1,compressed);
	}

	if (q > HIGH1)
	{
		fwrite(enc->nhw_res6,enc->nhw_res6_len,1,compressed);
		fwrite(enc->nhw_res6_bit,enc->nhw_res6_bit_len,1,compressed);
		fwrite(enc->nhw_res6_word,enc->nhw_res6_word_len,1,compressed);
		fwrite(enc->nhw_char_res1,enc->nhw_char_res1_len,2,compressed);
	}

	if (q > HIGH2)
	{
		fwrite(enc->high_qsetting3,enc->qsetting3_len,4,compressed);
	}

	fwrite(enc->nhw_select_word1,enc->nhw_select1,1,compressed);
	fwrite(enc->nhw_select_word2,enc->nhw_select2,1,compressed);

	if (q > LOW5) 
	{
		fwrite(enc->res_U_64,im->fmt.tile_size,1,compressed);
		fwrite(enc->res_V_64,im->fmt.tile_size,1,compressed);
		fwrite(enc->highres_word,enc->highres_comp_len,1,compressed);
	}
	
	fwrite(enc->res_ch,enc->end_ch_res,1,compressed);
	fwrite(enc->encode,enc->size_data2*4,1,compressed);

	fclose(compressed);

	free(enc->encode);
	
	if (q > LOW3)
	{
		free(enc->nhw_res4);
	}
	
	if (q > LOW8)
	{
		free(enc->res1.res);
		free(enc->res1.res_bit);
		free(enc->res1.res_word);
	}
	
	free(enc->nhw_select_word1);
	free(enc->nhw_select_word2);

	if (q >= LOW1)
	{
		free(enc->res3.res);
		free(enc->res3.res_bit);
		free(enc->res3.res_word);
	}

	if (q >= HIGH1)
	{
		free(enc->res5.res);
		free(enc->res5.res_bit);
		free(enc->res5.res_word);
	}

	if (q > HIGH1)
	{
		free(enc->nhw_res6);
		free(enc->nhw_res6_bit);
		free(enc->nhw_res6_word);
		free(enc->nhw_char_res1);
	}

	if (q > HIGH2)
	{
		free(enc->high_qsetting3);
	}

	free(enc->exw_Y);
	free(enc->res_ch);

	if (q > LOW5)  	
	{
		free(enc->res_U_64);
		free(enc->res_V_64);
	}
	free(enc->highres_word);
	return 0;
}

/* Legacy NHW tile file encoding */
int nhw_encode(image_buffer *im, const char *output_filename, int rate)
{
	encode_state enc;
	decode_state dec;

	memset(&enc, 0, sizeof(encode_state)); // Set all to 0

	printf("Running with quality setting %d\n", im->setup->quality_setting);
	downsample_YUV420(im, &enc, rate);

	encode_image(im, &enc, rate);

	write_compressed_file(im, &enc, output_filename);
	free(enc.tree1);
	free(enc.tree2);
	return 0;
}


