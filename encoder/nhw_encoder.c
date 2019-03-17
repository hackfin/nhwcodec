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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <png.h>

#include "imgio.h"
#include "codec.h"

#ifndef MAYBE_STATIC
#define MAYBE_STATIC
#endif

#define CLIP(x) ( (x<0) ? 0 : ((x>255) ? 255 : x) );

// XXX This is for restructuring tests only:
#ifdef SWAPOUT
#define SWAPOUT_FUNCTION(f)  __CONCAT(orig,f)
#else
#define SWAPOUT_FUNCTION(f)  f
#endif

#define IS_ODD(x) (((x) & 1) == 1)

void encode_y(image_buffer *im, encode_state *enc, int ratio);

void copy_bitplane0(unsigned char *sp, int n, unsigned char *res);

void copy_from_quadrant(short *dst, const short *src, int x, int y);
void copy_to_quadrant(short *dst, const short *src, int x, int y);
void copy_buffer(short *dst, const short *src, int x, int y);

void reduce_q7_LL(int quality, short *pr, const char *wvlt);
void reduce_q9_LL(short *pr, const char *wvlt, int s);
void reduce_q9_LH(short *pr, const char *wvlt, int ratio, int s);
void reduce_generic(int quality, short *resIII, short *pr, char *wvlt,
	encode_state *enc, int ratio);
int configure_wvlt(int quality, char *wvlt);
void ywl(int quality, short *pr, int ratio);
void process_res_q8(int quality, short *pr, short *res256, encode_state *enc);

void compress_s2(int quality, short *resIII, short *pr, char *wvlt, encode_state *enc, int ratio);
void scan_run_code(unsigned char *s, const short *pr, encode_state *enc);

void encode_image(image_buffer *im,encode_state *enc, int ratio);

int main(int argc, char **argv) 
{	
	image_buffer im;
	encode_state enc;
	int select;
	char *arg;

	if (argv[1]==NULL || argv[1]==0)
	{
		printf("\n Copyright (C) 2007-2013 NHW Project (Raphael C.)\n");
		printf("\n-> nhw_encoder.exe filename.bmp");
		printf("\n  (with filename a bitmap color 512x512 image)\n");
		exit(-1);
	}

	im.setup=(codec_setup*)malloc(sizeof(codec_setup));
	im.setup->quality_setting=NORM;

	if (argv[2]==NULL || argv[2]==0) select=8;
	else
	{
		arg=argv[2];

		if (strcmp(arg,"-h3")==0) im.setup->quality_setting=HIGH3;
		else if (strcmp(arg,"-h2")==0) im.setup->quality_setting=HIGH2; 
		else if (strcmp(arg,"-h1")==0) im.setup->quality_setting=HIGH1; 
		else if (strcmp(arg,"-l1")==0) im.setup->quality_setting=LOW1; 
		else if (strcmp(arg,"-l2")==0) im.setup->quality_setting=LOW2; 
		else if (strcmp(arg,"-l3")==0) im.setup->quality_setting=LOW3; 
		else if (strcmp(arg,"-l4")==0) im.setup->quality_setting=LOW4; 
		else if (strcmp(arg,"-l5")==0) im.setup->quality_setting=LOW5; 
		else if (strcmp(arg,"-l6")==0) im.setup->quality_setting=LOW6; 
		else if (strcmp(arg,"-l7")==0) im.setup->quality_setting=LOW7; 
		else if (strcmp(arg,"-l8")==0) im.setup->quality_setting=LOW8; 
		else if (strcmp(arg,"-l9")==0) im.setup->quality_setting=LOW9;
		else if (strcmp(arg,"-l10")==0) im.setup->quality_setting=LOW10;
		else if (strcmp(arg,"-l11")==0) im.setup->quality_setting=LOW11;
		else if (strcmp(arg,"-l12")==0) im.setup->quality_setting=LOW12;
		else if (strcmp(arg,"-l13")==0) im.setup->quality_setting=LOW13;
		else if (strcmp(arg,"-l14")==0) im.setup->quality_setting=LOW14;
		else if (strcmp(arg,"-l15")==0) im.setup->quality_setting=LOW15;
		else if (strcmp(arg,"-l16")==0) im.setup->quality_setting=LOW16;
		else if (strcmp(arg,"-l17")==0) im.setup->quality_setting=LOW17;
		else if (strcmp(arg,"-l18")==0) im.setup->quality_setting=LOW18;
		else if (strcmp(arg,"-l19")==0) im.setup->quality_setting=LOW19;

		select=8; //for now...
	}

	menu(argv,&im,&enc,select);

	/* Encode Image */
	encode_image(&im,&enc,select);


	write_compressed_file(&im,&enc,argv);
	return 0;
}

MAYBE_STATIC
void residual_coding_q2(short *pr, short *res256, int res_uv)
{
	int i, j, count, scan;
	short *p;

	int step = (IM_DIM>>1);
	
	for (i=0,count=0;i<(IM_SIZE>>1);i+=IM_DIM)
	{
		for (scan=i,j=0;j<(IM_DIM>>1);j++,scan++,count++)
		{
			p = &pr[scan];
			short *q = &p[(IM_SIZE>>1)];
			short *q1 = &q[step];

			short d0 = p[0]-res256[count];
			short d1;
#warning "OUT_OF_BOUNDS Fix"
			if (count < (IM_SIZE >>2) - 1) {
				d0 = p[1]-res256[count+1];
			} else {
				d0 = 0; // Assumption ok?
			}

			p += IM_DIM >> 1;

			if (d0 > 3 && d0 < 7)
			{
				if (d1 >2 && d1 <7)
				{
					if (abs(p[0]) < 8)
						{ p[0]=12400;     count++;scan++;j++; continue; }
					else if (abs(q[0]) <8)
						{ q[0]=12400;     count++;scan++;j++;continue;}
					else if (abs(q1[0]) <8)
						{ q1[0]=12400;    count++;scan++;j++; continue;}
				}
			}
			else if (d0 < -3 && d0 >-7)
			{
				if (d1 < -2 && d1 > -8)
				{
					if      (abs(p[0])  < 8) {  p[0] = 12600;count++;scan++;j++;continue;}
					else if (abs(q[0])  < 8) {  q[0] = 12600;count++;scan++;j++;continue;}
					else if (abs(q1[0]) < 8) { q1[0] = 12600;count++;scan++;j++;continue;}
				}
			}

			if (abs(d0) > res_uv) 
			{
				if (d0 > 0) {
					if      (abs(p[0])  < 8 )  p[0] = 12900;
					else if (abs(q[0])  < 8 )  q[0] = 12900; 
					else if (abs(q1[0]) < 8 ) q1[0] = 12900; 
				} else
				if (d0 == -5)
				{
					if (d1 <0)
					{
						if      (abs(p[0])  < 8)  p[0] = 13000;
						else if (abs(q[0])  < 8)  q[0] = 13000; 
						else if (abs(q1[0]) < 8) q1[0] = 13000; 
					}
					
				}
				else
				{
					if      (abs( p[0]) < 8)  p[0] = 13000;
					else if (abs( q[0]) < 8)  q[0] = 13000; 
					else if (abs(q1[0]) < 8) q1[0] = 13000; 
				}
			}
		}
	}
}


MAYBE_STATIC
void preprocess14(const short *pr, short *result)
{
	int stage;
	int i, j, count, scan;
	int s = 2 * IM_DIM; // scan line size
	int halfn = IM_DIM;
	int tmp = 2 * halfn + 1;


// Checks high frequency parts (CUR = HH2)
//       'pr'                      'result'
// +---+---+---+---+              +---+---+
// |   |   |       |              |   |   |
// +---+---+       +  < IM_SIZE   +---+---+
// |   |CUR|       |              |   |MAR|
// +---+---+---+---+  <- (END)    +---+---+
// |       |       |
// +       +       +
// |       |       |
// +---+---+---+---+

// ^.......^      <-  j = 0..halfn

	for (i=0,count=0;i<(4*IM_SIZE>>1);i+=s,count+=halfn)
	{
		for (scan=i,j=0;j<halfn;j++,scan++) 
		{
			int k = count+j;
			// FIXME: eliminate
			const short *p = &pr[scan];
			if (i>=IM_SIZE || j>=(halfn>>1))
			{
				// CUR
				stage=pr[scan];

				if (stage<-7)
				{
					// MAR
					if (((-stage)&7)==7)    result[k]+=16000;
					else if (!((-stage)&7)) result[k]+=16000;
				}
				else if (stage<-4) result[k]+=12000;
				else if (stage>=0)
				{
					if (stage>=2 && stage<5) 
					{
						if (scan>=tmp && (i+j)<(2*IM_SIZE-2*halfn-1))
						{
							if (abs(p[-tmp])!=0 || abs(p[tmp])!=0)
							{
								result[k]+=12000;
							}
							//else result[k]+=8000;
						}
					}
					else if (!(stage&7)) result[k]+=12000;
					else if ((stage&7)==1) result[k]+=12000;
					else if (stage>4 && stage<=7) result[k]+=16000;
				}
			}
		}
	}
}

MAYBE_STATIC
void postprocess14(short *dst, short *pr, short *res256)
{
	int i, j, count, scan;
	int a, e;

	int s = 2 * IM_DIM;
	int halfn = IM_DIM >> 1;


// Does some scoring depending on the SQ* coefficient markings
//
//       'pr'          <<<<        'result'
// +---+---+---+---+              +---+---+
// |   |   |       |              |   |SQ0|
// +---+---+       +  < IM_SIZE   +---+---+
// |   |   |       |              |SQ1|SQ2|
// +---+---+---+---+  <- (END)    +---+---+
// |       |       |                               
// +       +       +
// |       |       |
// +---+---+---+---+

// ^.......^      <-  j = 0..IM_DIM
// ^...^      (halfn)


	for (i=0,count=0;i<(4*IM_SIZE>>1);i+=s,count+=IM_DIM)
	{
		for (j=0;j<IM_DIM;j++) 
		{
			int k = count + j;
			if (res256[k]>14000) 
			{
				res256[k]-=16000;
				// FIXME: eliminate ifs
				if (i < IM_SIZE) {
					// SQ0
					if (j>=halfn)
						pr[(i>>8)+((j-halfn)<<10)+s]++;
				} else {
					if (j<halfn) // SQ1
						pr[((i-IM_SIZE)>>8)+(j<<10)+1]++; 
					else // SQ3
						pr[((i-IM_SIZE)>>8)+((j-halfn)<<10)+(s+1)]++; 
				}
			}
			else if (res256[k]>10000) 
			{
				res256[k]-=12000;
				if (i < IM_SIZE) {
					if (j>=halfn) pr[(i>>8)+((j-halfn)<<10)+s]--;
				} else {
					if (j<halfn)
						pr[((i-IM_SIZE)>>8)+(j<<10)+1]--; 
					else 
						pr[((i-IM_SIZE)>>8)+((j-halfn)<<10)+(s+1)]--;
				}
			}
		}
	}

//       'pr'          <<<<        'result'
// +---+---+---+---+              +---+---+
// |SE0|SE1|       |              |SC0|SC1|
// +---+---+       +  < IM_SIZE   +---+---+
// |SE2|SE3|       |              |SC2|SC3|
// +---+---+---+---+  <- (END)    +---+---+
// |       |       |                               
// +       +       +
// |       |       |
// +---+---+---+---+

// ^.......^      <-  e = 0..IM_DIM
// ^...^      (halfn)

	//                   (END) 

#define _MOD(c) \
					{ dst[e] = res256[count]+c;pr[e] += c; }

	for (i=0,count=0;i<(4*IM_SIZE>>1);i+=s)
	{
		for (e=i,j=0;j<IM_DIM;j++,count++,e++)
		{
			// SE0        SC0
			scan=pr[e]-res256[count];

			if(scan>11)        _MOD(-7)
			else if (scan>7)   _MOD(-4)
			else if (scan>5)   _MOD(-2)
			else if (scan>4)   _MOD(-1)
			else if (scan<-11) _MOD(7)
			else if (scan<-7)  _MOD(4)
			else if (scan<-5)  _MOD(2)
			else if (scan<-4)  _MOD(1)
			else if (abs(scan)>1)
			{
				a=(pr[e+1]-res256[count+1]);
				if (abs(a)>4)
				{
					if (a>0)
					{
						if (a>11) a-=7;
						else if (a>7) a-=4;
						else if (a>5) a-=2;
						else a--;
					}
					else
					{
						if (a<-11) a+=7;
						else if (a<-7) a+=4;
						else if (a<-5) a+=2;
						else a++;
					}
				}

#warning "OUT_OF_BOUNDS FIX"
				if (e > 0 && count > 0) {
					a+=(pr[e-1]-res256[count-1]);
				}

				if (scan>=4 && a>=1)         _MOD(-1)
				else if (scan<=-4 && a<=-1)  _MOD(1)
				else if (scan==3 && a>=0)    _MOD(-1)
				else if (scan==-3 && a<=0)   _MOD(1)
				else if (abs(a)>=3) 
				{
					if (scan>0 && a>0)       _MOD(-1)
					else if (scan<0 && a<0)  _MOD(1)
					else if (a>=5)           _MOD(-2)
					else if (a<=-5)          _MOD(2)
					else if (a>=4)           _MOD(-1)
					else if (a<=-4)          _MOD(1)
					else
						dst[e] = res256[count];
				}
				else dst[e]=res256[count];
			}
			else dst[e] = res256[count];
		}
	}
}

MAYBE_STATIC
void compress1(int quality, short *pr, encode_state *enc)
{
	int i, j, count, scan;
	int Y, stage, res;
	int a, e;
	int step = 2 * IM_DIM;

	for (i=0,a=0,e=0,count=0,res=0,Y=0,stage=0;i<((4*IM_SIZE)>>2);i+=step)
	{
		for (count=i,j=0;j<(step>>2);j++,count++)
		{
			short *p = &pr[count];

			scan = pr[count];
			// FIXME: Move quality check outside loop
			if (quality>LOW3 && scan>10000) 
			{
				if (scan>20000) {
					scan-=24000;
					enc->nhw_res4[res++]= j + 1;
					stage++;
				}
				else scan-=16000;
			} else
			if (IS_ODD(scan)
			 && count > i
			 && IS_ODD(p[1]) /*&& !(p[-1]&1)*/) {
				if (j < ((IM_DIM>>1)-2)
				 && IS_ODD(p[2]) /*&& !(p[3]&1)*/) 
				{
					if (abs(scan-p[2])>1 && quality>LOW3) p[1]++;
				}
				else if (i<(IM_SIZE-step-2)
					&& IS_ODD(p[step])
					&& IS_ODD(p[(2*IM_DIM+1)])
					&& !IS_ODD(p[(2*IM_DIM+2)]))
				{
					if (p[step]<10000 && quality>LOW3) 
					{
						p[step]++;
					}
				}
			} else
			if (IS_ODD(scan) && i>=step && i<(IM_SIZE-(6*IM_DIM))) {
				if ((p[step]&1)==1 && (p[(2*IM_DIM+1)]&1)==1)
				{
					if ((p[(4*IM_DIM)]&1)==1 && !(p[(6*IM_DIM)]&1)) 
					{
						if (p[step]<10000 && quality>LOW3) 
						{
							p[step]++;
						}
					}
				}
			}

			if (scan>255 && (j>0 || i>0))
			{
				enc->exw_Y[e++]=(i>>9);	enc->exw_Y[e++]=j+128;
				Y=scan-255;if (Y>255) Y=255;enc->exw_Y[e++]=Y;
				enc->tree1[a]=enc->tree1[a-1];enc->ch_res[a]=enc->tree1[a-1];a++;p[0]=0;

			}
			else if (scan<0 && (j>0 || i>0))  
			{
				enc->exw_Y[e++]=(i>>9);enc->exw_Y[e++]=j;
				if (scan<-255) scan=-255;
				enc->exw_Y[e++]=-scan;
				enc->tree1[a]=enc->tree1[a-1];enc->ch_res[a]=enc->tree1[a-1];a++;p[0]=0;
			}
			else 
			{
				if (scan>255) scan=255;else if (scan<0) scan=0;
				enc->ch_res[a]=scan;enc->tree1[a++]=scan&254;p[0]=0;
			}

		}

		if (quality>LOW3)
		{
			if (!stage) enc->nhw_res4[res++]=128;
			else enc->nhw_res4[res-1]+=128;
			stage=0;
		}
	}
	enc->exw_Y_end=e;
}




int process_res_q3(short *pr)
{
	int i, j, count;
	int res;
	int stage;
	int step = 2 * IM_DIM;

	for (i=0,count=0,res=0,stage=0;i<((4*IM_SIZE)>>2);i+=step)
	{
		for (count=i,j=0;j<((step>>2)-3);j++,count++)
		{
			if (IS_ODD(pr[count])   &&
			    IS_ODD(pr[count+1]) &&
			    IS_ODD(pr[count+2]) &&
			    IS_ODD(pr[count+3]) &&
			    abs(pr[count]-pr[count+3])>1)
			{
				pr[count]  +=24000;
				pr[count+1]+=16000;
				pr[count+2]+=16000;
				pr[count+3]+=16000;
				res++;stage++;j+=3;count+=3;
			}
		}

		if (!stage) res++;
		stage=0;
	}
	return res;
}




void process_hires_q8(unsigned char *highres, short *res256, encode_state *enc)
{
	int i, j, stage, scan;
	int Y;
	unsigned char *ch_comp;
	unsigned char *scan_run;
	unsigned char *nhw_res1I_word;
	int res;
	int count, e;


#warning "Possibly not completely initalized"
	nhw_res1I_word=(unsigned char*)malloc(enc->nhw_res1_word_len*sizeof(char));
	
	for (i=0,count=0,res=0,e=0;i<IM_SIZE;i+=IM_DIM)
	{
		for (scan=i,j=0;j<IM_DIM;j++,scan++)
		{
			short *p = &res256[scan];
			if (j==(IM_DIM-2))
			{
				p[0]=0;
				p[1]=0;
				highres[count++]=(IM_DIM-2);j++; 
			} else {
				switch (p[0]) {
					case 141:
						highres[count++]=j;
						p[0]=0; nhw_res1I_word[e++]=1;
						break;
					case 140: 
						highres[count++]=j;
						p[0]=0; nhw_res1I_word[e++]=0;
						break;
					case 126: 
						highres[count++]=j;
						p[0]=122; nhw_res1I_word[e++]=0;
						break;
					case 125: 
						highres[count++]=j;
						p[0]=121; nhw_res1I_word[e++]=1;
						break;
					case 148: 
						highres[count++]=j;
						p[0]=144; nhw_res1I_word[e++]=1;
						break;
					case 149: 
						highres[count++]=j;
						p[0]=145; nhw_res1I_word[e++]=0;
				}
			}
		}
	}

	ch_comp=(unsigned char*)malloc(count*sizeof(char));
	memcpy(ch_comp,highres,count*sizeof(char));

	for (i=1,res=1;i<count-1;i++)
	{
		if (ch_comp[i]==(IM_DIM-2))
		{
			if (ch_comp[i-1]!=(IM_DIM-2) && ch_comp[i+1]!=(IM_DIM-2))
			{
				if (ch_comp[i-1]<=ch_comp[i+1]) highres[res++]=ch_comp[i];
			}
			else highres[res++]=ch_comp[i];
		}
		else highres[res++]=ch_comp[i];
	}

	highres[res++]=ch_comp[count-1];
	free(ch_comp);

	enc->nhw_res1_len=res;
	enc->nhw_res1_word_len=e;
	enc->nhw_res1=(unsigned char*)calloc((enc->nhw_res1_len),sizeof(char));

	for (i=0;i<enc->nhw_res1_len;i++) enc->nhw_res1[i]=highres[i];

	scan_run=(unsigned char*)malloc((enc->nhw_res1_len+8)*sizeof(char));

	for (i=0;i<enc->nhw_res1_len;i++) scan_run[i]=enc->nhw_res1[i]>>1;

	highres[0]=scan_run[0];

	for (i=1,count=1;i<enc->nhw_res1_len-1;i++)
	{
		int d = scan_run[i]-scan_run[i-1];
		if (d>=0 && d<8)
		{
			int d1 = scan_run[i+1]-scan_run[i];
			if (d1 >=0 && d1 < 16) {
				highres[count++]=128+(d<<4)+d1;
				i++;
			}
			else highres[count++]=scan_run[i];
		}
		else highres[count++]=scan_run[i];
	}

	for (i=0,stage=0;i<enc->nhw_res1_len;i++) 
	{
		if (enc->nhw_res1[i]!=254) scan_run[stage++]=enc->nhw_res1[i];
	}

	for (i=stage;i<stage+8;i++) scan_run[i]=0;


	Y= (stage>>3) + 1;
	enc->nhw_res1_bit_len=Y;
	enc->nhw_res1_bit = (unsigned char*)malloc(Y*sizeof(char));
	copy_bitplane0(scan_run, Y, enc->nhw_res1_bit);
	enc->nhw_res1_len=count;

	Y= (enc->nhw_res1_word_len >> 3) + 1;
	free(scan_run);
	enc->nhw_res1_word=(unsigned char*)malloc((Y)*sizeof(char));
	copy_bitplane0(nhw_res1I_word, Y, enc->nhw_res1_word);
	enc->nhw_res1_word_len=Y;

	for (i=0;i<count;i++) enc->nhw_res1[i]=highres[i];

	free(nhw_res1I_word);
}


MAYBE_STATIC
void process_res3_q1(unsigned char *highres, short *res256, encode_state *enc)
{
	int i, j, scan, e, Y, stage, count;
	int res;
	unsigned char *ch_comp;
	unsigned char *scan_run;
	unsigned char *nhw_res3I_word;
	nhw_res3I_word = (unsigned char*) calloc(enc->nhw_res3_word_len,sizeof(char));

	for (i=0,count=0,res=0,e=0;i<IM_SIZE;i+=IM_DIM)
	{
		for (scan=i,j=0;j<IM_DIM;j++,scan++)
		{
			if (j==(IM_DIM-2))
			{
				res256[scan]=0;
				res256[scan+1]=0;
				highres[count++]=(IM_DIM-2);j++;
			}
			else if (res256[scan]==121) 
			{
				highres[count++]=j;
				res256[scan]=0;nhw_res3I_word[e++]=1;
			}
			else if (res256[scan]==122) 
			{
				highres[count++]=j;
				res256[scan]=0;nhw_res3I_word[e++]=0;
			}
			else if (res256[scan]==123) 
			{
				highres[count++]=j;
				res256[scan]=0;nhw_res3I_word[e++]=2;
			}
			else if (res256[scan]==124) 
			{
				highres[count++]=j;
				res256[scan]=0;nhw_res3I_word[e++]=3;
			}
		}
	}

	ch_comp=(unsigned char*)malloc(count*sizeof(char));
	memcpy(ch_comp,highres,count*sizeof(char));

	for (i=1,res=1;i<count-1;i++)
	{
		if (ch_comp[i]==(IM_DIM-2))
		{
			if (ch_comp[i-1]!=(IM_DIM-2) && ch_comp[i+1]!=(IM_DIM-2))
			{
				if (ch_comp[i-1]<=ch_comp[i+1]) highres[res++]=ch_comp[i];
			}
			else highres[res++]=ch_comp[i];
		}
		else highres[res++]=ch_comp[i];
	}

	highres[res++]=ch_comp[count-1];
	free(ch_comp);

	enc->nhw_res3_len=res;
	enc->nhw_res3_word_len=e;
	enc->nhw_res3=(unsigned char*)calloc((enc->nhw_res3_len),sizeof(char));

	for (i=0;i<enc->nhw_res3_len;i++) enc->nhw_res3[i]=highres[i];

	scan_run=(unsigned char*)malloc((enc->nhw_res3_len+8)*sizeof(char));

	for (i=0;i<enc->nhw_res3_len;i++) scan_run[i]=enc->nhw_res3[i]>>1;

	highres[0]=scan_run[0];

	for (i=1,count=1;i<enc->nhw_res3_len-1;i++)
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

	for (i=0,stage=0;i<enc->nhw_res3_len;i++) 
	{
		if (enc->nhw_res3[i]!=254) scan_run[stage++]=enc->nhw_res3[i];
	}

	for (i=stage;i<stage+8;i++) scan_run[i]=0;

	enc->nhw_res3_bit_len=((stage>>3)+1);

	enc->nhw_res3_bit=(unsigned char*)calloc(enc->nhw_res3_bit_len,sizeof(char));

	Y=stage>>3;

	copy_bitplane0(scan_run, Y+1, enc->nhw_res3_bit);

	enc->nhw_res3_len=count;

	Y=enc->nhw_res3_word_len>>3;
	free(scan_run);

	scan_run=(unsigned char*)nhw_res3I_word;
	enc->nhw_res3_word=(unsigned char*)malloc((enc->nhw_res3_bit_len<<1)*sizeof(char));


	unsigned char *sp = scan_run;

#warning "OUT_OF_BOUNDS Fix"
	for (i=0,stage=0; i < 2*Y;i++)
	{

#define _SLICE_BITS_POS(val, mask, shift) (((val) & mask) << shift)

		enc->nhw_res3_word[i]=  _SLICE_BITS_POS(sp[0], 0x3, 6) |
		                        _SLICE_BITS_POS(sp[1], 0x3, 4) |
		                        _SLICE_BITS_POS(sp[2], 0x3, 2) |
		                        _SLICE_BITS_POS(sp[3], 0x3, 0);

		sp += 4;

	}

	// Set to null. If not done, decoded picture show funny artefacts
	// FIXME
	enc->nhw_res3_word[i++]= 0;
	enc->nhw_res3_word[i]= 0;

	enc->nhw_res3_word_len = 2 * Y + 2;

	for (i=0;i<count;i++) enc->nhw_res3[i]=highres[i];

	
	free(nhw_res3I_word);
}


MAYBE_STATIC
void process_res5_q1(unsigned char *highres, short *res256, encode_state *enc)
{
	int i, j, scan, e, Y, stage, count;
	int res;
	unsigned char *ch_comp;
	unsigned char *nhw_res5I_word;
	unsigned char *scan_run;

	nhw_res5I_word=(unsigned char*)calloc(enc->nhw_res5_word_len,sizeof(char));

	for (i=0,count=0,res=0,e=0;i<IM_SIZE;i+=IM_DIM)
	{
		for (scan=i,j=0;j<IM_DIM;j++,scan++)
		{
			if (j==(IM_DIM-2))
			{
				res256[scan]=0;
				res256[scan+1]=0;
				highres[count++]=(IM_DIM-2);j++;
			}
			else if (res256[scan]==144) 
			{
				highres[count++]=j;
				res256[scan]=0;nhw_res5I_word[e++]=1;
			}
			else if (res256[scan]==145) 
			{
				highres[count++]=j;
				res256[scan]=0;nhw_res5I_word[e++]=0;
			}
		}
	}

	ch_comp=(unsigned char*)malloc(count*sizeof(char));
	memcpy(ch_comp,highres,count*sizeof(char));

	for (i=1,res=1;i<count-1;i++)
	{
		if (ch_comp[i]==(IM_DIM-2))
		{
			if (ch_comp[i-1]!=(IM_DIM-2) && ch_comp[i+1]!=(IM_DIM-2))
			{
				if (ch_comp[i-1]<=ch_comp[i+1]) highres[res++]=ch_comp[i];
			}
			else highres[res++]=ch_comp[i];
		}
		else highres[res++]=ch_comp[i];
	}

	highres[res++]=ch_comp[count-1];
	free(ch_comp);

	enc->nhw_res5_len=res;
	enc->nhw_res5_word_len=e;
	enc->nhw_res5=(unsigned char*)calloc((enc->nhw_res5_len),sizeof(char));

	for (i=0;i<enc->nhw_res5_len;i++) enc->nhw_res5[i]=highres[i];

	scan_run=(unsigned char*)calloc((enc->nhw_res5_len+8),sizeof(char));

	for (i=0;i<enc->nhw_res5_len;i++) scan_run[i]=enc->nhw_res5[i]>>1;

	highres[0]=scan_run[0];

	for (i=1,count=1;i<enc->nhw_res5_len-1;i++)
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

	for (i=0,stage=0;i<enc->nhw_res5_len;i++) 
	{
		if (enc->nhw_res5[i]!=254) scan_run[stage++]=enc->nhw_res5[i];
	}

	for (i=stage;i<stage+8;i++) scan_run[i]=0;

	Y = stage>>3;
	enc->nhw_res5_bit_len = Y+1;
	enc->nhw_res5_bit=(unsigned char*)calloc(enc->nhw_res5_bit_len,sizeof(char));
	copy_bitplane0(scan_run, Y + 1, enc->nhw_res5_bit);
	enc->nhw_res5_len=count;

	Y = enc->nhw_res5_word_len>>3;
	free(scan_run);
	enc->nhw_res5_word = (unsigned char*) calloc((enc->nhw_res5_bit_len<<1),sizeof(char));
	copy_bitplane0(nhw_res5I_word, Y + 1, enc->nhw_res5_word);
	enc->nhw_res5_word_len = Y+1;

	for (i=0;i<count;i++) enc->nhw_res5[i]=highres[i];
	
	free(nhw_res5I_word);
}

MAYBE_STATIC
void process_res_hq(int quality, short *wl_first_order, short *res256)
{
	int i, j, count, scan;

	for (i=0,count=0;i<IM_SIZE;i+=IM_DIM)
	{
		for (scan=i,j=0;j<IM_DIM-2;j++,scan++)
		{
			if (res256[scan]!=0)
			{
				count=(j<<8)+(i>>8);

				if (res256[scan]==141) 
				{
					wl_first_order[count]-=5;
				}
				else if (res256[scan]==140) 
				{
					wl_first_order[count]+=5;
				}
				else if (res256[scan]==144) 
				{
					wl_first_order[count]-=3;
				}
				else if (res256[scan]==145) 
				{
					wl_first_order[count]+=3;
				}
				else if (res256[scan]==121) 
				{
					wl_first_order[count]-=4;
					wl_first_order[count+1]-=3;
				}
				else if (res256[scan]==122) 
				{
					wl_first_order[count]+=4;
					wl_first_order[count+1]+=3;
				}
				else if (res256[scan]==123) 
				{
					wl_first_order[count]+=2;
					wl_first_order[count+1]+=2;
					wl_first_order[count+2]+=2;
				}
				else if (res256[scan]==124) 
				{
					wl_first_order[count]-=2;
					wl_first_order[count+1]-=2;
					wl_first_order[count+2]-=2;
				}
				else if (res256[scan]==126) 
				{
					wl_first_order[count]+=9;
					wl_first_order[count+1]+=3;
				}
				else if (res256[scan]==125) 
				{
					count=(j<<8)+(i>>8);
					wl_first_order[count]-=9;
					wl_first_order[count+1]-=3;
				}
				else if (res256[scan]==148) 
				{
					wl_first_order[count]-=8;
				}
				else if (res256[scan]==149) 
				{
					wl_first_order[count]+=8;
				}
			}
		}
	}
}

void copy_thresholds(short *process, const short *resIII, int n)
{
	int i, j, count;

	for (i=0, count=0; i < (2*IM_SIZE); i += n)
	{
		short *p = &process[i];

		for (j=0;j<IM_DIM;j++)
		{ 
			// FIXME: eliminate
			if (j<(IM_DIM>>1) && i<IM_SIZE) {
				if (resIII[count]>8000) {
					*p++ = resIII[count++];
				} else {
					*p++ = 0;count++;
				}
			}
			else *p++ = resIII[count++];
		}
	}
}

void SWAPOUT_FUNCTION(encode_y)(image_buffer *im, encode_state *enc, int ratio)
{
	int quality = im->setup->quality_setting;
	int res;
	int end_transform;
	short *res256, *resIII;
	unsigned char *highres;
	short *pr;
	char wvlt[7];
	pr=(short*)im->im_process;

	int n = 2 * IM_DIM; // line size Y

	end_transform=0;
	// Unused:
	// wavelet_order=im->setup->wvlts_order;
	//for (stage=0;stage<wavelet_order;stage++) wavelet_analysis(im,(2*IM_DIM)>>stage,end_transform++,1); 

	wavelet_analysis(im, n,end_transform++,1);

#warning "OUT_OF_BOUNDS Fix"
	// Add some head room for padding:
	res256 = (short*) malloc((IM_SIZE + n) * sizeof(short));
	resIII = (short*) malloc(IM_SIZE*sizeof(short));

	// copy upper left LL1 tile into res256 array
	copy_from_quadrant(res256, im->im_jpeg, n, n);

	im->setup->RES_HIGH=0;

	wavelet_analysis(im, n >> 1, end_transform,1);

	if (quality > LOW14) // Better quality than LOW14?
	{
		preprocess14(pr, res256);
		offsetY_recons256(im,enc,ratio,1);
		wavelet_synthesis(im, n>>1,end_transform-1,1);
		postprocess14(im->im_jpeg, pr, res256);
		wavelet_analysis(im, n>>1,end_transform,1);
	}

	configure_wvlt(quality, wvlt);
	
	if (quality <= LOW9) // Worse than LOW9?
	{
		if (quality > LOW14) wvlt[0] = 10; else wvlt[0] = 11;
		reduce_q9_LH(pr, wvlt, ratio, n);
	}

	if (quality < LOW7) {
			
		reduce_q7_LL(quality, pr, wvlt);
		
		if (quality <= LOW9)
		{
			reduce_q9_LL(pr, wvlt, n);
		}
	}
	
	copy_from_quadrant(resIII, pr, n, n);
	
	// Must look at this, might not be completely initialized:
	enc->tree1=(unsigned char*)malloc(((96*IM_DIM)+1)*sizeof(char));
	enc->exw_Y=(unsigned char*)malloc(32*IM_DIM*sizeof(short));
	
	if (quality > LOW3)
	{
		res = process_res_q3(pr);
		enc->nhw_res4_len=res;
		enc->nhw_res4=(unsigned char*)calloc(enc->nhw_res4_len,sizeof(char));
	}

	enc->ch_res=(unsigned char*)calloc((IM_SIZE>>2),sizeof(char));

	compress1(quality, pr, enc);

	Y_highres_compression(im, enc);

	free(enc->ch_res);

	copy_to_quadrant(pr, resIII, n, n);

	//free(resIII);
	
	if (quality > LOW8) { // Better than LOW8?

		offsetY_recons256(im, enc, ratio, 0);

		wavelet_synthesis(im,n>>1,end_transform-1,1);

		if (quality>HIGH1) {
			im->im_wavelet_first_order=(short*)malloc(IM_SIZE*sizeof(short));
			copy_from_quadrant(im->im_wavelet_first_order, im->im_jpeg, n, n);
		}
	}

	// After last call of offsetY_recons256() we can free:
	free(enc->highres_mem);

////////////////////////////////////////////////////////////////////////////

	reduce_generic(quality, resIII, pr, wvlt, enc, ratio);
	
	if (quality > LOW8) {
		process_res_q8(quality, pr, res256, enc);
	}

	highres=(unsigned char*)calloc(((96*IM_DIM)+1),sizeof(char));

	if (quality > HIGH1) {
		process_res_hq(quality, im->im_wavelet_first_order, res256);
	}
	
	if (quality > LOW8)
	{
		process_hires_q8(highres, res256, enc);
	}

	// Further residual processing:
	if (quality>=LOW1) {
		process_res3_q1(highres, res256, enc);
		if (quality>=HIGH1)
		{
			process_res5_q1(highres, res256, enc);
		}
	}
	

	free(highres);
	free(res256);

	copy_thresholds(pr, resIII, n);

	free(resIII);
	ywl(quality, pr, ratio);
	offsetY(im,enc,ratio);

	if (quality>HIGH1) {
		im->im_wavelet_band=(short*)calloc(IM_SIZE,sizeof(short));
		// These two functions use .im_wavelet_band:
		im_recons_wavelet_band(im);
		wavelet_synthesis_high_quality_settings(im,enc);
		free(im->im_wavelet_first_order);
		free(im->im_wavelet_band);
		free(im->im_quality_setting);
	}
}


MAYBE_STATIC
void encode_uv(image_buffer *im, encode_state *enc, int ratio, int res_uv, int uv)
{
	int i, j, a, e, Y, scan, count;
	int end_transform;
	unsigned char wvlt[7];
	unsigned char *ch_comp;
	unsigned char *scan_run;
	short *res256;
	short *resIII;
	short *pr;
	const unsigned char *buf;

	int s2 = IM_DIM;
	int s4 = s2 >> 1;

	int imquart = IM_SIZE >> 2;
	int imhalf  = IM_SIZE >> 1;

	int quality = im->setup->quality_setting;

	if (uv) {
		buf = im->im_bufferV;
	} else {
		buf = im->im_bufferU;
	}
	for (i=0;i<IM_SIZE;i++) im->im_jpeg[i]=buf[i];

	pr= (short*) im->im_process;
	
	if (quality <= LOW6) pre_processing_UV(im);

	end_transform=0; im->setup->RES_HIGH=0;

	wavelet_analysis(im, s2,end_transform++,0);

	res256 = (short*) malloc(imquart*sizeof(short));
	resIII = (short*) malloc(imquart*sizeof(short));

	copy_from_quadrant(res256, im->im_jpeg, s2, s2);
	
	if (quality <= LOW4)
	{
		for (i = 0 ;i < imhalf;i += s2)
		{
			for (scan= i + s4,j= s4; j< s2; j++, scan++)
			{
				if (abs(pr[scan])>=ratio && abs(pr[scan])<24) 
				{	
					 pr[scan]=0;	
				}
			}
		}	
			
		for (i = imhalf; i < IM_SIZE;i += s2)
		{
			for (scan = i, j = 0;j < s4;j++,scan++)
			{
				if (abs(pr[scan])>=ratio && abs(pr[scan])<32) 
				{	
					 pr[scan]=0;	
				}
			}

			for (scan=i + s4,j = s4;j< s2;j++,scan++)
			{
				if (abs(pr[scan])>=ratio && abs(pr[scan])<48) 
				{	
					 pr[scan]=0;		
				}
			}
		}
	}

	wavelet_analysis(im,s2>>1,end_transform,0); 

	offsetUV_recons256(im,ratio,1);

	wavelet_synthesis(im,s2>>1,end_transform-1,0); 

	int t0, t1;

	if (uv) {
		t0 = 1; t1 = -1;
	} else {
		t0 = 0; t1 = 0;
	}

	for (i=0,count=0;i< imhalf;i+=s2)
	{
		for (e=i,j=0;j<(s2>>1);j++,count++,e++)
		{
			scan=pr[e]-res256[count];
	
			if      (scan >10)  {im->im_jpeg[e]=res256[count]-6;}
			else if (scan >7)   {im->im_jpeg[e]=res256[count]-3;}
			else if (scan >4)   {im->im_jpeg[e]=res256[count]-2;}
			else if (scan >3)    im->im_jpeg[e]=res256[count]-1;
			else if (scan >2 && (pr[e+1]-res256[count+1])>=t0)
			                     im->im_jpeg[e]=res256[count]-1;
			else if (scan <-10) {im->im_jpeg[e]=res256[count]+6;}
			else if (scan <-7)  {im->im_jpeg[e]=res256[count]+3;}
			else if (scan <-4)  {im->im_jpeg[e]=res256[count]+2;}
			else if (scan <-3)   im->im_jpeg[e]=res256[count]+1;
			else if (scan<-2 && (pr[e+1]-res256[count+1])<=t1)
			                     im->im_jpeg[e]=res256[count]+1;
			else                 im->im_jpeg[e]=res256[count];
		}
	}

	wavelet_analysis(im, s2>>1, end_transform, 0);
	copy_from_quadrant(resIII, pr, s2, s2);
	offsetUV_recons256(im,ratio,0);
	wavelet_synthesis(im, s2>>1, end_transform-1,0);

	if (quality >= LOW2) { 
		residual_coding_q2(pr, res256, res_uv);
	}

	copy_to_quadrant(pr, resIII, s2, s2);

	if (quality <= LOW9)
	{
		//if (quality==LOW9 || quality==LOW10)
		//{
			wvlt[0]=2;
			wvlt[1]=3;
			wvlt[2]=5;
			wvlt[3]=8;
		//}
		
		for (i=0,scan=0; i < imquart-(2*s2); i+=s2)
		{
			for (scan=i,j=0;j< (s2>>2)-2;j++,scan++)
			{
				short *p = &pr[scan];
				short *q = &p[s2];

				if (abs(p[1]-q[s2+1]) <  wvlt[2]
				 && abs(q[0]-q[2])    <  wvlt[2]
				 && abs(q[1]-q[0])    < (wvlt[3]-1)
				 && abs(p[1]-q[1])    <  wvlt[3])
				{
					q[1] = (p[1] + q[s2+1] + q[0] + q[2]+2) >> 2;
				}
			}
		}
		
		for (i=0,scan=0;i<imquart-(2*s2);i+=(s2))
		{
			for (scan=i,j=0;j<(s2>>2)-2;j++,scan++)
			{
				short *p = &pr[scan];
				short *q = &p[s2];

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

	enc->exw_Y[enc->exw_Y_end++]=0;
	enc->exw_Y[enc->exw_Y_end++]=0;


	if (uv) {
		a = imquart+(imquart>>2);
	} else {
		a = imquart;
	}

	for (i=0; i<(imquart);i+=(s2))
	{
		for (j=0;j<((s2)>>2);j++)
		{
			short *p = &pr[i+j];
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
				enc->tree1[a++]=scan&254;
				p[0]=0;
			}
		}
	}

	if (quality>LOW5) 
	{
		int code;
		if (uv) {
			enc->res_V_64=(unsigned char*)calloc((s2<<1),sizeof(char));
			scan_run=(unsigned char*)enc->res_V_64;
			code = 20480;
		} else {		
			enc->res_U_64=(unsigned char*)calloc((s2<<1),sizeof(char));
			scan_run=(unsigned char*)enc->res_U_64;
			code = 16384;
		}

		ch_comp=(unsigned char*)calloc((16*s2),sizeof(char));

		int c;

		for (i=0, e=0; i< (16*s2); i += 8)
		{
#define _EXTRACT_BIT(x, n) (((x) >> n) & 1)
			unsigned char *pc = &ch_comp[i];
			c = code + i;
			*pc++ = _EXTRACT_BIT(enc->tree1[c++], 1);
			*pc++ = _EXTRACT_BIT(enc->tree1[c++], 1);
			*pc++ = _EXTRACT_BIT(enc->tree1[c++], 1);
			*pc++ = _EXTRACT_BIT(enc->tree1[c++], 1);
			*pc++ = _EXTRACT_BIT(enc->tree1[c++], 1);
			*pc++ = _EXTRACT_BIT(enc->tree1[c++], 1);
			*pc++ = _EXTRACT_BIT(enc->tree1[c++], 1);
			*pc   = _EXTRACT_BIT(enc->tree1[c], 1);

			pc = &ch_comp[i];

			short tmp = 

			tmp  = ((*pc++) << 7); tmp |= ((*pc++) << 6);
			tmp |= ((*pc++) << 5); tmp |= ((*pc++) << 4);
			tmp |= ((*pc++) << 3); tmp |= ((*pc++) << 2);
			tmp |= ((*pc++) << 1); tmp |= ((*pc++));

			scan_run[e++] = tmp;

		}
		free(ch_comp);
	}

	offsetUV(im,enc,ratio);

	if (uv) count=(4*IM_SIZE+1);
	else    count=(4*IM_SIZE);

	for (j=0;j < s2;)
	{
		for (i=0;i < s4 ;i++)
		{
			unsigned char *dst = &im->im_nhw[count];
			short *src = &pr[j];

			int k;
			for (k = 0; k < 8; k++) {
				*dst = *src++; dst += 2;
			}

			j+= s2;

			src = &pr[j+7];
			// copy mirrored:
			for (k = 0; k < 8; k++) {
				*dst = *src--; dst += 2;
			}

			j+= s2;
			count+=32;
		}

		j -= (IM_SIZE-8);
	}

	free(res256);
	free(resIII);

}

void encode_image(image_buffer *im,encode_state *enc, int ratio)
{
	int res_uv;

	im->im_process=(short*)calloc(4*IM_SIZE,sizeof(short));

	//if (im->setup->quality_setting<=LOW6) block_variance_avg(im);

	if (im->setup->quality_setting<HIGH2) 
	{
		pre_processing(im);
	}

	encode_y(im, enc, ratio);

	im->im_nhw=(unsigned char*)calloc(6*IM_SIZE,sizeof(char));

	scan_run_code(im->im_nhw, im->im_process, enc);

	free(im->im_process);
	// Release original data buffer
	//
	free(im->im_jpeg); // XXX
	
	
////////////////////////////////////////////////////////////////////////////	

	im->im_process=(short*)calloc(IM_SIZE,sizeof(short)); // {
	im->im_jpeg=(short*)calloc(IM_SIZE,sizeof(short)); // Work buffer {

	if (im->setup->quality_setting > LOW3) res_uv=4;
	else                                   res_uv=5;

	encode_uv(im, enc, ratio, res_uv, 0);
	free(im->im_bufferU); // Previously reserved buffer XXX

	encode_uv(im, enc, ratio, res_uv, 1);
	free(im->im_bufferV); // Previously reserved buffer XXX

	free(im->im_jpeg); // Free Work buffer }

	free(im->im_process); // }

	highres_compression(im, enc);

	wavlts2packet(im,enc);

	free(im->im_nhw);

}

int menu(char **argv,image_buffer *im,encode_state *os,int rate)
{
	FILE *im256;
	int ret;
 	const char *str;
 
	// INITS & MEMORY ALLOCATION FOR ENCODING
	//im->setup=(codec_setup*)malloc(sizeof(codec_setup));
	im->setup->colorspace=YUV;
	im->setup->wavelet_type=WVLTS_53;
	im->setup->RES_HIGH=0;
	im->setup->RES_LOW=3;
	im->setup->wvlts_order=2;
	im->im_buffer4=(unsigned char*)calloc(4*3*IM_SIZE,sizeof(char));

	// OPEN IMAGE
	if ((im256 = fopen(argv[1],"rb")) == NULL )
	{
		printf ("\n Could not open file \n");
		exit(-1);
	}

	// READ IMAGE DATA
	ret = read_png(im256, im->im_buffer4, 512); 
	fclose(im256);

	switch (ret) {
		case -1:
			str = "Bad size"; break;
		case -2:
			str = "Bad header"; break;
		default:
			str = "Unknown error";
	}

	if (ret >= 0) {
		downsample_YUV420(im,os,rate);
	} else {
		fprintf(stderr, "Error: %s\n", str);
	}

	return 0 ;
}

int write_compressed_file(image_buffer *im,encode_state *enc,char **argv)
{
	FILE *compressed;
	int len;
	char OutputFile[200];

	len=strlen(argv[1]);
	memset(argv[1]+len-4,0,4);
	sprintf(OutputFile,"%s.nhw",argv[1]);

	int q = im->setup->quality_setting;

	compressed = fopen(OutputFile,"wb");
	if( NULL == compressed )
	{
		printf("Failed to create compressed .nhw file %s\n",OutputFile);
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
	if (q > LOW8) fwrite(&enc->nhw_res1_len,2,1,compressed);
	
	if (q >= LOW1)
	{
		fwrite(&enc->nhw_res3_len,2,1,compressed);
		fwrite(&enc->nhw_res3_bit_len,2,1,compressed);
	}
	if (q > LOW3)
	{
		fwrite(&enc->nhw_res4_len,2,1,compressed);
	}
	
	if (q > LOW8) fwrite(&enc->nhw_res1_bit_len,2,1,compressed);

	if (q >= HIGH1)
	{
		fwrite(&enc->nhw_res5_len,2,1,compressed);
		fwrite(&enc->nhw_res5_bit_len,2,1,compressed);
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
		fwrite(enc->nhw_res1,enc->nhw_res1_len,1,compressed);
		fwrite(enc->nhw_res1_bit,enc->nhw_res1_bit_len,1,compressed);
		fwrite(enc->nhw_res1_word,enc->nhw_res1_word_len,1,compressed);
	}
	
	if (q > LOW3)
	{
		fwrite(enc->nhw_res4,enc->nhw_res4_len,1,compressed);
	}
	
	if (q >=LOW1)
	{
		fwrite(enc->nhw_res3,enc->nhw_res3_len,1,compressed);
		fwrite(enc->nhw_res3_bit,enc->nhw_res3_bit_len,1,compressed);
		fwrite(enc->nhw_res3_word,enc->nhw_res3_word_len,1,compressed);
	}
	
	if (q >= HIGH1)
	{
		fwrite(enc->nhw_res5,enc->nhw_res5_len,1,compressed);
		fwrite(enc->nhw_res5_bit,enc->nhw_res5_bit_len,1,compressed);
		fwrite(enc->nhw_res5_word,enc->nhw_res5_word_len,1,compressed);
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
		fwrite(enc->res_U_64,(IM_DIM<<1),1,compressed);
		fwrite(enc->res_V_64,(IM_DIM<<1),1,compressed);
		fwrite(enc->highres_word,enc->highres_comp_len,1,compressed);
	}
	
	fwrite(enc->ch_res,enc->end_ch_res,1,compressed);
	fwrite(enc->encode,enc->size_data2*4,1,compressed);

	fclose(compressed);

	free(enc->encode);
	
	if (q > LOW3)
	{
		free(enc->nhw_res4);
	}
	
	if (q > LOW8)
	{
		free(enc->nhw_res1);
		free(enc->nhw_res1_bit);
		free(enc->nhw_res1_word);
	}
	
	free(enc->nhw_select_word1);
	free(enc->nhw_select_word2);

	if (q >= LOW1)
	{
		free(enc->nhw_res3);
		free(enc->nhw_res3_bit);
		free(enc->nhw_res3_word);
	}

	if (q >= HIGH1)
	{
		free(enc->nhw_res5);
		free(enc->nhw_res5_bit);
		free(enc->nhw_res5_word);
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
	free(enc->ch_res);

	if (q > LOW5)  	
	{
		free(enc->highres_word);
		free(enc->res_U_64);
		free(enc->res_V_64);
	}
	return 0;
}



