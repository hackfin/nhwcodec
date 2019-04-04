/***************************************************************************
****************************************************************************
*  NHW Image Codec 													       *
*  file: image_processing.c  										       *
*  version: 0.1.3 						     		     				   *
*  last update: $ 06012012 nhw exp $							           *
*																		   *
****************************************************************************
****************************************************************************

****************************************************************************
*  remark: -image processing set										   *
***************************************************************************/

/* Copyright (C) 2007-2013 NHW Project
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
#include <assert.h>

#include "utils.h"
#include "codec.h"
#include "tree.h"

#define ROUND_8(x)  ((x) & ~7)
#define ROUND_2(x)  ((x) & ~1)

// GLOBALS:
//

static unsigned char extra_words1[19]={10,12,14,18,20,22,26,28,30,34,36,38,42,44,46,50,52,54,58};
static unsigned char extra_words2[19]={60,62,66,68,70,74,76,78,82,84,86,90,92,94,98,100,102,106,108};

static char extra_table[109] = {
0,0,0,0,0,0,0,0,0,0,1,0,2,0,3,0,0,0,4,0,5,0,6,0,0,0,7,0,8,0,9,0,0,0,10,0, 
11,0,12,0,0,0,13,0,14,0,15,0,0,0,16,0,17,0,18,0,0,0,19,0, 
-1,0,-2,0,0,0,-3,0,-4,0,-5,0,0,0,-6,0,-7,0,-8,0,0,0,-9,0,-10,0, 
-11,0,0,0,-12,0,-13,0,-14,0,0,0,-15,0,-16,0,-17,0,0,0,-18,0,-19
};

#if 0 // UNUSED CODE
void quantizationUV(image_buffer *im)
{
	int i,j,low,high;
	int step2 = im->fmt.tile_size / 2;
	int im_size = im->fmt.end / 4;

	int quad_half = im_size / 8;

	low=(0xFF)^((1<<im->setup->RES_LOW)-1);
	high=(0xFF)^((1<<im->setup->RES_HIGH)-1);

	for (i=0;i<quad_half;i+=step2)
	{
		for (j=0;j<(step2>>3);j++)
		{
			if (i<quad_half/2 && j<(step2>>4)) im->im_process[j+i]&=high;
			else im->im_process[j+i]&=low;
		}
	}

	for (i=0;i<quad_half;i+=step2)
	{
		for (j=(step2>>3);j<step2;j++) im->im_process[j+i]&=low;
	}

	for (i=quad_half;i<im_size;i++)
	{
		im->im_process[i]&=low;
	}
}


void quantizationY(image_buffer *im)
{
	int i,j,low,high;
	int quad_half = im->fmt.end / 8;

	low=(0xFF)^((1<<im->setup->RES_LOW)-1);
	high=(0xFF)^((1<<im->setup->RES_HIGH)-1);
	int step = im->fmt.tile_size;

	for (i=0;i<quad_half;i+=(step))
	{
		for (j=0;j<((step)>>3);j++)
		{
			if (i<(quad_half / 2) && j<((step)>>4)) im->im_process[j+i]&=high;
			else im->im_process[j+i]&=low;
		}
	}

	for (i=0;i<quad_half;i+=(step))
	{
		for (j=((step)>>3);j<step;j++) im->im_process[j+i]&=low;
	}

	for (i=quad_half;i<im->fmt.end;i++)
	{
		im->im_process[i]&=low;
	}

}

#endif

void offsetUV(image_buffer *im,encode_state *enc,int m2)
{
	int i,exw,a;

	int step = im->fmt.tile_size;
	int step2 = step / 2;
	int im_size = im->fmt.end / 4;

	for (i=0;i < im_size;i++)
	{
		short *p = &im->im_process[i];
		a = p[0];

		//if (a>10000) {im->im_process[i]=124;continue;}
		//else if (a<-6 && a>-10) {im->im_process[i]=134;continue;}

		if (a > 10000)
		{ 
			switch (a) {
				case CODE_12400: {p[0]=124;break;} 
				case CODE_12600: {p[0]=126;break;} 
				case CODE_12900: {p[0]=122;break;}  
				case CODE_13000: {p[0]=130;break;}  
				default:
					assert(0);
			}
		} else {
			// FIXME:
			int cond = (i&255)<(step2-1);
			int cond1 = i < (im_size-1);

			if (a>127) {
				exw = (ROUND_8(a) - 128) >> 3;
				if (exw>18) exw=18;
				p[0]=extra_words1[exw];
			} else if (a<-127) {
				exw = (ROUND_8(-a) - 128) >> 3;
				if (exw>18) exw=18;
				p[0]=extra_words2[exw];
			} else {
				if (a<0) {
					if (a==-7 || a==-8) {
						if (cond && (p[0+1]==-7 || p[0+1]==-8)) {
							p[0]=120;p[0+1]=120;i++;continue;
						}
					}

					a = -a;

					if (cond1 && p[0+1]<0 && p[0+1]>-8) {
						if ((a&7)<6) { a = ROUND_8(a); }
					} else {
						if ((a&7)<7) { a = ROUND_8(a); }
					}
					
					a = -a;
				} else if (a>6 && (a&7)>=6) {
					if (cond && p[0+1]==7) p[0+1]=8;
				}

				//if (a==7 || a==4) {p[0]=132;continue;} 
				if (a < m2 && a > -m2) { p[0]=128; }
				else {
					a += 128;
					p[0]= ROUND_8(a);
				}
			}
		}
	}
}

static inline
void offset_fixup_neighbours(short *p, int i, short p2)
{
	int a;

	if (p[0] > 7 && p[1] > 7) {
		a = p[0];

		if ((a & 7) == 0 && (p[1] & 7) == 0) {
			if (a > 15) {
				// TODO: Verify boundary conditions
				if (i > 0) { // FIXME: boundary case
					if      (p[-1] <= 0)           { p[0]--; }
					else if (p2 <= 0 && p[1] > 15) { p[1]--; }
				}
			} else
				// Special boundary case:
				if (p2 <= 0 && p[1] > 15) {p[1]--; }
		}
	}

}

/* Process all subbands except LL */
void offsetY_non_LL(image_buffer *im)
{
	int i;
	short *p;
	short *pr = im->im_process;
	int step = im->fmt.tile_size;
	int step2 = step / 2;
	// Process in '#':
	
	// - - # #
	// - - # #
	// - - - -
	// - - - -

	p = &pr[0];
	for (i = 0; i < im->fmt.half; i++, p++) {
		if ((i&511) >= step2) { // FIXME: loop variable comparison
			if ((i&511)<(step-1)) {
				short p2 = ((i&511)<(step-2)) ? p[2] : 1;
				offset_fixup_neighbours(p, i, p2);
			}
		}
	}

	// - - - -
	// - - - -
	// # # # #
	// # # # #

	p = &pr[im->fmt.half];
	for (i = im->fmt.half; i < im->fmt.end; i++, p++) {
		if ((i&511)<(step-1)) { // FIXME: loop variable comparison
			short p2 = ((i&511)<(step-2)) ? p[2] : 1;
			offset_fixup_neighbours(p, i, p2);
		}

	}

}

void offsetY_LL_q4(image_buffer *im)
{
	int i, a, j;

	short *pr = im->im_process;
	int step = im->fmt.tile_size;

	int step2 = step / 2;
	// # # - -
	// # # - -
	// - - - -
	// - - - -

	for (i=0;i<im->fmt.half;i+=step)
	{
		for (a=i+1,j=1;j<(step2-1);j++,a++)
		{
			short *q = &pr[a];

			if (q[0]>3 && q[0]<8)
			{
				if (q[-1]>3 && q[-1]<=7)
				{
					if (q[1]>3 && q[1]<=7)
					{
						q[0]=MARK_127;q[-1]=MARK_128;j++;a++;//q[1]=MARK_128;
					}
					else if (q[(step-1)]>3 && q[(step-1)]<=7)
					{
						if (q[step]>3 && q[step]<=7)
						{
							q[-1]=MARK_121;q[0]=MARK_128;
							q[(step-1)]=MARK_128;q[step]=MARK_128;
							j++;a++;
						}
					}
				}
			}
			else if (q[0]<-3 && q[0]>-8)
			{
				if (q[-1]<-3 && q[-1]>=-7)
				{
					if (q[1]<-3 && q[1]>=-7)
					{
						q[0]=MARK_129;q[-1]=MARK_128;j++;a++;//q[1]=MARK_128;	 
					}
					else if (q[(step-1)]<-3 && q[(step-1)]>=-7)
					{
						if (q[step]<-3 && q[step]>=-7)
						{
							q[-1]=MARK_122;q[0]=MARK_128;
							q[(step-1)]=MARK_128;q[step]=MARK_128;
							j++;a++;
						}
					}
				}
			}
		}
	}

	// # # - -
	// # # - -
	// - - - -
	// - - - -


	for (i=0;i<im->fmt.half;i+=step) {
		for (a=i,j=0;j<(step2-1);j++,a++) {
			short *q = &pr[a];
			if (q[0]==5 || q[0]==6 || q[0]==7) {
				if (q[1]==5 || q[1]==6 || q[1]==7) {
					//q[0]+=3;
					q[0]=MARK_126;
					j++;a++;
				}
			}
			else if (q[0]==-5 || q[0]==-6 || q[0]==-7)
			{
				if (q[1]==-5 || q[1]==-6 || q[1]==-7)
				{
					//q[0]-=3;
					q[0]=MARK_125;
					j++;a++;
				}
			}
		}
	}

}

void offsetY(image_buffer *im,encode_state *enc, int m1)
{
	int i,exw,a;
	short *pr;
	int step = im->fmt.tile_size;

	pr = (short*) im->im_process;

	offsetY_non_LL(im);
	
	if (im->setup->quality_setting>LOW4) {
		offsetY_LL_q4(im);
	}

	for (i=0;i<im->fmt.end;i++)
	{
		short *q = &pr[i];
		a = q[0];

		switch (a) {

			case MARK_128:  q[0] = 128; break;
			case MARK_127:  q[0] = 127; break;
			case MARK_129:  q[0] = 129; break;
			case MARK_125:  q[0] = 125; break;
			case MARK_126:  q[0] = 126; break;
			case MARK_121:  q[0] = 121; break;
			case MARK_122:  q[0] = 122; break;

			default:
			{
				int cond = (i&511)<(step-1);
				if (a>127) {
					exw = (ROUND_8(a) - 128) >> 3;
					if (exw > 18) exw=18;
					q[0]=extra_words1[exw];
				} else if (a<-127) {
					exw = (ROUND_8(-a) - 128) >> 3;
					if (exw > 18) exw=18;
					q[0]=extra_words2[exw];
				} else {
					if (cond) { // FIXME: Outside loop
						
						if (a < 0) {
							if (a==-7 && q[1] == 8) {q[0] = -8; a = -8;}
							else if (a<-12 && ((-a) & 7) == 6) {
								if (q[1]==-7) q[1]=-9;
							}
							if (((-a) & 7) < 7) { a = -ROUND_8(-a); }
						} else {
							if (a == 8 && q[1] == -7) q[1] = -8;
							else if (a > 12 && (a & 7) >= 6) {
								if (q[1]==7) q[1]=9;
							}
						}
					} else {
						if (a<0) {
							if (((-a) & 7) < 7) { a = -ROUND_8(-a); }
						}
					}

					if (a < m1 && a > -m1)  {
						q[0]=128;
					} else {
						a += 128;
						q[0]= ROUND_8(a);
					}
				}

			}
		}
	}
}

void im_recons_wavelet_band(image_buffer *im)
{
	int i,j,a,r,scan;
	int step = im->fmt.tile_size;

	for (i=0,r=0;i<im->fmt.half;i+=(step))
	{ 
		for (scan=i+step/2,j=0;j<step/2;j++,scan++)
		{
			a = im->im_process[scan];
			short *wlb = &im->im_wavelet_band[r];

			switch (a) {
				case 129:
					wlb[-1]=-5;wlb[0]=-7;wlb[1]=-5;
					r+=2; j++; scan++;
					break;
				case 128:
					r++;
					break;
				case 127:
					wlb[-1]=5;wlb[0]=6;wlb[1]=5;
					r+=2; j++; scan++;
					break;
				default:
					if ((a&7) != 0) {
						if (extra_table[a]>0) {
							wlb[0]=WVLT_ENERGY_NHW+(extra_table[a]<<3);
							r++;
						} else {
							wlb[0]=(extra_table[a]<<3)-WVLT_ENERGY_NHW;
							r++;
						}
					} else {
						if (a>0x80) wlb[0]=(a-125);
						else wlb[0]=(a-131);
						r++;
					}	
			}
		}
	}
}

// This does some pre-sharpening in some quality configurations
//
void pre_processing(image_buffer *im)
{
	int i,j,res,count,e=0,a=0,sharpness,n1;
	short *nhw_process;
	char lower_quality_setting_on;
	int step = im->fmt.tile_size;

	nhw_process=(short*)im->im_process;
	memcpy(im->im_process,im->im_jpeg,im->fmt.end*sizeof(short));

	if (im->setup->quality_setting<=LOW6) lower_quality_setting_on=1;
	else lower_quality_setting_on=0;

	if (im->setup->quality_setting==LOW4) sharpness=82;
	else if (im->setup->quality_setting==LOW5) sharpness=77;
	else if (im->setup->quality_setting==LOW6) sharpness=72;
	else if (im->setup->quality_setting==LOW7) sharpness=67;
	else if (im->setup->quality_setting==LOW8) sharpness=64;
	else if (im->setup->quality_setting==LOW9) sharpness=58;
	else if (im->setup->quality_setting==LOW10) sharpness=40;
	else if (im->setup->quality_setting==LOW11) sharpness=24;
	else if (im->setup->quality_setting==LOW12) sharpness=3;
	else if (im->setup->quality_setting==LOW13) sharpness=0;
	else if (im->setup->quality_setting==LOW14) sharpness=0;
	else if (im->setup->quality_setting==LOW15 || im->setup->quality_setting==LOW16) sharpness=24;
	else if (im->setup->quality_setting==LOW17) sharpness=36;
	else if (im->setup->quality_setting==LOW18) sharpness=45;
	else if (im->setup->quality_setting==LOW19) sharpness=48;
	
	
	if (im->setup->quality_setting>LOW11) n1=36;
	else if (im->setup->quality_setting==LOW11) n1=24;
	else if (im->setup->quality_setting==LOW12) n1=10;
	else if (im->setup->quality_setting==LOW13) n1=6;
	else if (im->setup->quality_setting==LOW14) n1=36;
	else if (im->setup->quality_setting<=LOW15 && im->setup->quality_setting>=LOW17) n1=36;
	else if (im->setup->quality_setting==LOW18) n1=56;
	else if (im->setup->quality_setting==LOW19) n1=60;
	else    /* default */                       n1=36;

	for (i=step;i<(im->fmt.end-step);i+=step)
	{
		short *p = &nhw_process[i + 1];
		short *q = &im->im_jpeg[i + 1];

		for (j=1;j<(step-2);j++, p++, q++)
		{   

			res	   =   (p[0]<<3) -
						p[0-1]-p[0+1]-
						p[0-step]-p[0+step]-
						p[0-(step+1)]-p[0+(step-1)]-
						p[0-(step-1)]-p[0+(step+1)];

			j++;p++; q++;

			count   =  (p[0]<<3) -
						p[0-1]-p[0+1]-
						p[0-step]-p[0+step]-
						p[0-(step+1)]-p[0+(step-1)]-
						p[0-(step-1)]-p[0+(step+1)];
						

#define WEIGHTED_SUM_NEIGHBOURS(p) \
	( 4 * p[0] + p[-1] + p[1] + p[-step] + p[step] + 4)


			if (lower_quality_setting_on)
			{
				if (abs(res)>4 && abs(res)<n1)
				{
					p--; q--;
					
					if (abs(p[-step]-p[0-1])<4
					 && abs(p[-1]-p[0+step])<4
					 && abs(p[step]-p[0+1])<4
					 && abs(p[1]-p[0-step])<4)
					{
						q[0]= WEIGHTED_SUM_NEIGHBOURS(p) >> 3;
					}
					
					p++; q++;
				}
				
				if (abs(count)>4 && abs(count)<n1)
				{
					if (abs(p[0-step]-p[0-1])<4 && abs(p[0-1]-p[0+step])<4 && abs(p[0+step]-p[0+1])<4 && abs(p[0+1]-p[0-step])<4)
					{
						q[0]= WEIGHTED_SUM_NEIGHBOURS(p) >> 3;
					}
				}
			}
		

			if (im->setup->quality_setting>LOW4) 
			{
			if (res>201) {q[0-1]-=2;e=4;}
			else if (res<-201) {q[0-1]+=2;e=3;}
			else if (res>176) {q[0-1]--;e=2;}
			else if (res<-176) {q[0-1]++;e=1;}
			else e=0;

			if (count>201)
			{
				if (!e || e==3) q[0]-=2;
				else if (e!=4) q[0]--;
			}
			else if (count<-201) 
			{
				if (!e || e==4) q[0]+=2;
				else if (e!=3) q[0]++;
			}
			else if (count>176) 
			{
				if (e!=4) q[0]--;
			}
			else if (count<-176) 
			{
				if (e!=3) q[0]++;
			}
			}
			else
			{
				if (abs(res)>sharpness)
				{
					if (res>0) q[0-1]++;else q[0-1]--;
				}

				if (abs(count)>sharpness)
				{
					if (count>0) q[0]++;else q[0]--;
				}

			}

			if (im->setup->quality_setting>LOW6 || (im->setup->quality_setting<=LOW10 && im->setup->quality_setting>LOW13)) 
			{
				if (res<32 && res>10) 
				{
					if (abs(count)>=23) 
					{
						if (res<16)
						{
							if (count>0 && count<32 && res>11) q[0]++;
							q[0-1]++;
							a=0;continue;
						}
						else 
						{
							if (!a) q[0-1]+=2;else q[0-1]++;
							a=0;continue;
						}
					}
				}
				else if (res>-32 && res<-10) 
				{
					if (abs(count)>=23) 
					{
						if (res>-16)
						{
							if (count<0 && count>-32 && res<-11) q[0]--;
							q[0-1]--;
							a=0;continue;
						}
						else 
						{
							if (!a) q[0-1]-=2;else q[0-1]--; 
							a=0;continue;
						}
					}
				}
			
				a=0;

				if (count<32 && count>10) 
				{
					if (abs(res)>=23)
					{
						if (count<16)
						{
							if (res>0 && res<32 && count>11) q[0-1]++;
							q[0]++;
						}
						else {q[0]+=2;a=1;}
					}
				}	
				else if (count>-32 && count<-10) 
				{
					if (abs(res)>=23)
					{
						if (count>-16)
						{
							if (res<0 && res>-32 && count<-11) q[0-1]--;
							q[0]--;
						}
						else {q[0]-=2;a=1;}
					}
				}
			}
		}
	}
}

void pre_processing_UV(image_buffer *im)
{
	int i,j,scan,res;
	short *nhw_process;
	int step = im->fmt.tile_size / 2;
	int im_size = im->fmt.end / 4;

	nhw_process=(short*)im->im_process;
	memcpy(im->im_process,im->im_jpeg,im_size*sizeof(short));

	for (i=step;i<((im_size)-step);i+=step)
	{
		for (scan=i+1,j=1;j<(step-1);j++,scan++)
		{   
			res	   =   (nhw_process[scan]<<3) -
						nhw_process[scan-1]-nhw_process[scan+1]-
						nhw_process[scan-step]-nhw_process[scan+step]-
						nhw_process[scan-(step+1)]-nhw_process[scan+(step-1)]-
						nhw_process[scan-(step-1)]-nhw_process[scan+(step+1)];
				
			if (im->setup->quality_setting<LOW6) 
			{
				if (abs(res)>=14)
				{
					if (res>0) im->im_jpeg[scan]-=2;else im->im_jpeg[scan]+=2;
				}
				else if (abs(res)>5)
				{
					if (res>0) im->im_jpeg[scan]--;else im->im_jpeg[scan]++;
				}
			}
			else
			{
				if (res>5) im->im_jpeg[scan]--;
				else if (res<-5) im->im_jpeg[scan]++;
			}
		}
	}
}

#if 0 // UNUSED
void block_variance_avg(image_buffer *im)
{
	int i,j,e,a,t1,scan,count,avg,variance;
	short *nhw_process,*nhw_process2;
	unsigned char *block_var;
	int step = im->fmt.tile_size;


	memcpy(im->im_process,im->im_jpeg,im->fmt.end*sizeof(short));

	nhw_process=(short*)im->im_jpeg;
	nhw_process2=(short*)im->im_process;

	block_var=(unsigned char*)calloc((im->fmt.end>>6),sizeof(char));

	for (i=0,a=0;i<im->fmt.end;i+=(8*step))
	{
		for (scan=i,j=0;j<step;j+=8,scan+=8)
		{
			avg=0;variance=0;count=0;

			for (e=0;e<(8*step);e+=step)
			{
				avg+=nhw_process[scan+e];
				avg+=nhw_process[scan+1+e];
				avg+=nhw_process[scan+2+e];
				avg+=nhw_process[scan+3+e];
				avg+=nhw_process[scan+4+e];
				avg+=nhw_process[scan+5+e];
				avg+=nhw_process[scan+6+e];
				avg+=nhw_process[scan+7+e];
			}

			avg=(avg+32)>>6;

			for (e=0;e<(8*step);e+=step)
			{
				count=(nhw_process[scan+e]-avg);variance+=count*count;
				count=(nhw_process[scan+1+e]-avg);variance+=count*count;
				count=(nhw_process[scan+2+e]-avg);variance+=count*count;
				count=(nhw_process[scan+3+e]-avg);variance+=count*count;
				count=(nhw_process[scan+4+e]-avg);variance+=count*count;
				count=(nhw_process[scan+5+e]-avg);variance+=count*count;
				count=(nhw_process[scan+6+e]-avg);variance+=count*count;
				count=(nhw_process[scan+7+e]-avg);variance+=count*count;
			}

			if (variance<1500)
			{
				block_var[a++]=1;

				for (e=step;e<(7*step);e+=step)
				{
					for (t1=0;t1<6;t1++)
					{
						scan++;

						nhw_process[scan+e]=((nhw_process2[scan+e]<<3)+
											nhw_process2[scan+e-1]+nhw_process2[scan+e+1]+
											nhw_process2[scan+e-step]+nhw_process2[scan+e+step]+
											nhw_process2[scan+e-1-step]+nhw_process2[scan+e+1-step]+
											nhw_process2[scan+e-1+step]+nhw_process2[scan+e+1+step]+8)>>4;
					}

					scan-=6;
				}
			}
			else a++;
		}
	}

	int step8 = step / 8;

	for (i=0;i<im->fmt.end>>6)-(step8);i+=(step8))
	{
		for (count=i,j=0;j<(step8)-1;j++,count++)
		{
			if (block_var[count])
			{
				if (block_var[count+1])
				{
					scan=(i*8*8)+(j*8);

					for (e=step;e<(7*step);e+=step)
					{
						scan+=7;

						nhw_process[scan+e]=((nhw_process2[scan+e]<<3)+
											nhw_process2[scan+e-1]+nhw_process2[scan+e+1]+
											nhw_process2[scan+e-step]+nhw_process2[scan+e+step]+
											nhw_process2[scan+e-1-step]+nhw_process2[scan+e+1-step]+
											nhw_process2[scan+e-1+step]+nhw_process2[scan+e+1+step]+8)>>4;

						scan++;

						nhw_process[scan+e]=((nhw_process2[scan+e]<<3)+
											nhw_process2[scan+e-1]+nhw_process2[scan+e+1]+
											nhw_process2[scan+e-step]+nhw_process2[scan+e+step]+
											nhw_process2[scan+e-1-step]+nhw_process2[scan+e+1-step]+
											nhw_process2[scan+e-1+step]+nhw_process2[scan+e+1+step]+8)>>4;

						scan-=8;
					}
				}

				if (block_var[count+(step>>2)])
				{
					scan=(i*8*8)+(j*8);

					scan+=(7*step);

					for (e=1;e<7;e++)
					{
						nhw_process[scan+e]=((nhw_process2[scan+e]<<3)+
											nhw_process2[scan+e-1]+nhw_process2[scan+e+1]+
											nhw_process2[scan+e-step]+nhw_process2[scan+e+step]+
											nhw_process2[scan+e-1-step]+nhw_process2[scan+e+1-step]+
											nhw_process2[scan+e-1+step]+nhw_process2[scan+e+1+step]+8)>>4;
					}

					scan+=step;

					for (e=1;e<7;e++)
					{
						nhw_process[scan+e]=((nhw_process2[scan+e]<<3)+
											nhw_process2[scan+e-1]+nhw_process2[scan+e+1]+
											nhw_process2[scan+e-step]+nhw_process2[scan+e+step]+
											nhw_process2[scan+e-1-step]+nhw_process2[scan+e+1-step]+
											nhw_process2[scan+e-1+step]+nhw_process2[scan+e+1+step]+8)>>4;
					}
				}
			}
		}
	}

	free(block_var);
}
#endif

#define IS_ODD(x)  ((x & 1) == 1)

static
void offsetY_recons256_q3(image_buffer *im, short *nhw1, int part)
{
	int i,j;
	int step = im->fmt.tile_size;
	int im_size = im->fmt.end / 4;

#define Q3_CONDITION(n) \
	IS_ODD((n)[0]) && IS_ODD((n)[1]) && IS_ODD((n)[2]) && IS_ODD((n)[3]) && (abs((n)[0]-(n)[3]) > 1)

	if (!part) {
		for (i = 0; i < im_size; i += step) {
			short *p = &nhw1[i];
			for (j = 0;j < ((step/4)-3); j++, p++) {
				if (Q3_CONDITION(p)) {
					p[0] +=16000; p[1]+=16000;
					p[2] +=16000; p[3]+=16000;
					j+=3; p+=3;
				}
			}
		}
	} else {
		for (i = 0; i < im_size; i += step) {
			short *p = &nhw1[i];
			for (j = 0;j < ((step/4)-3); j++, p++) {
				if (Q3_CONDITION(p)) {
					p[0] += 16000; p[2] += 16000;
					j += 3; p += 3;
				}
			}
		}
	}
}

static inline
int evaluate_neighbours(short *p, short *q, int step)
{
	int skip = 0;

	if         (p[0]  >3 && p[0]  < 8) {
		if     (p[-1] >3 && p[-1] <=7) {

			if (p[1]  >3 && p[1]  <=7) {
				p[-1] = CODE_15300;
				p[0] = 0; q[0] = 5; q[1] = 5;
				skip = 1;
			} else
			if     ((p[(step-1)] > 3 && p[(step-1)] <=7)
				&&  (p[(step)]   > 3 && p[(step)]   <=7)) {
					p[-1] =       CODE_15500;  q[0]      = 5;
					p[(step-1)] = CODE_15500;  q[(step)] = 5;
					p[(step)] = 0;
					skip = 1;
			}
		}
	} else if (p[0] <- 3 && p[0] > -8) {
		if (p[-1]    <-3 && p[-1] >=-7) {
			if (p[1] <-3 && p[1]  >=-7) {
				p[-1] = CODE_15400;  p[0] = 0;
				q[0]  = -6;     q[1] = -5; skip = 1;
			}
			else if (p[(step-1)] < -3 && p[(step-1)] >= -7) {
				if (p[(step)] <    -3 && p[(step)]   >= -7) {
					p[-1]      = CODE_15600;  q[0]      = -5;
					p[(step-1)]= CODE_15600;  q[(step)] = -5;
					p[(step)]=0;
					skip = 1;
				}
			}
		}
	}
	return skip;
}

int tag_thresh_neighbour(short *p)
{
	if (p[0]==5 || p[0]==6 || p[0]==7) {
		if (p[1]==5 || p[1]==6 || p[1]==7) {
			//p[0]+=3;
			p[0]=CODE_15700;
			return 1;
		}
	} else if (p[0]==-5 || p[0]==-6 || p[0]==-7) {
		if (p[1]==-5 || p[1]==-6 || p[1]==-7) {
			//p[0]-=3;
			p[0]=CODE_15800;
			return 1;
		}
	}
	return 0;
}

void offsetY_recons256_q4(short *pr, short *jp, int size, int step, int part)
{
	int i, j;
	int a;

	for (i=0;i<(size);i+=(step))
	{
		a = i + (step/4) + 1;
		short *p = &pr[a];
		short *q = &jp[a];

		for (j=(step/4)+1;j<(step/2-1);j++,p++, q++)
		{
			if (evaluate_neighbours(p, q, step)) { j++; p++; q++; }
		}
	}

	for (i=(size);i<((2*size)-(step));i+=(step))
	{
		short *p = &pr[i+1];
		short *q = &jp[i+1];

		for (j=1;j<(step/2-1);j++,p++, q++)
		{
			if (evaluate_neighbours(p, q, step)) { j++; p++; q++; }
		}
	}

	if (!part)
	{

	// Operate on:
	// <-  s2 ->
	// +---+---+...
	// |   | A |
	// +---+---+   <- size
	// | B | B |
	// +---+---+
	// .
	//

		// A
		for (i=0;i<(size);i+=(step)) {
			short *p = &pr[i + step/4];
			for (j=(step/4);j<(step/2-1);j++,p++) {
				// If one got tagged, skip next.
				if (tag_thresh_neighbour(p)) { j++; p++; }
			}
		}
		// B
		for (i=size;i<(2*size);i+=(step)) {
			short *p = &pr[i];
			for (j=0;j<(step/2-1);j++,p++) {
				if (tag_thresh_neighbour(p)) { j++; p++; }
			}
		}
	}


}

static inline
void round_offset_limit_m1(short a, short *q, int m1)
{
	if (a < m1 && a > (-m1)) {
		q[0] = 0;
	} else {
		a += 128;

		if (a<0) a = -(ROUND_8(-a));
		else     a = ROUND_8(a);

		if (a > 128) q[0] = a - 125;
		else         q[0] = a - 131;
	}
}


static inline
int offsetY_subbands_H4(short *p, short *q, int m1, int end, int part)
{

	int a = p[0];

	if (a < 0) {
	// 	if (a > -m1) {
	// 		q[0] = 0;
	// 	} else
		if (a<-12 /*&& !part*/ && ((-a)&7)==6) {
			if (end && p[1]==-7) p[1]=-8;
		} else
		if (a==-7 && end && p[1]==8) {p[0]=-8;a=-8;}

		if (((-a) & 7) < 7) { a = -ROUND_8(-a); }

	} else {
		switch (a) {
			// Note: all these conditions are only effective
			// when quality better than 4: {
			case CODE_15300: q[0] =  5;          return 2;
			case CODE_15400: q[0] = -5;          return 2;
			case CODE_15500: q[0] =  5;          return 1;
			case CODE_15600: q[0] = -5;          return 1;
			case CODE_15700: q[0] =  6; q[1]= 6; return 1;
			case CODE_15800: q[0] = -6; q[1]=-6; return 1;
			// }
			case 8:
				if (end && p[1]==-7) p[1]=-8;
				break;
			default:
				// if (a < m1) {
				// 	q[0]=0;
				// } else
				if (a > 12 && !part && (a & 7) >= 6) {
					if (end && p[1]==7)     p[1]=8;
				}
		}
	}
	round_offset_limit_m1(a, q, m1);
	return 0;
}

// Remarks:
//
// Dangerous:
// enc->highres_mem might be uninitalized, when this function is called before
// Y_highres_compression(). This is only safe when 'part' != 0.
//

static
void offsetY_recons256_generic_part0(image_buffer *im, encode_state *enc,
	short thresh[2])
{
	int i, a, j;

	int im_size = im->fmt.end / 4;
	int step = im->fmt.tile_size;
	
	// Process LL:
	for (i=0,a=0;i<im_size;i+=step)
	{
		short *p = &im->im_process[i];
		short *q = &im->im_jpeg[i];

		for (a=i,j=0;j<(step/4);j++,a++, p++, q++) 
		{

			if (p[0]>10000) 
			{
				q[0]=p[0];
				continue;
			}
			else if ((p[0]&1)==1 && a>i && (p[1]&1)==1 /*&& !(p[-1]&1)*/)
			{
				if (j<((step/4)-2) && (p[2]&1)==1 /*&& !(im->im_process[a+3]&1)*/)
				{
					if (abs(p[0]-p[2])> thresh[1]) p[1]++;
				}
				/*else if (j<((step/4)-4) && (p[2]&1)==1 && (p[3]&1)==1
						&& !(p[4]&1))
				{
					p[2]++;
				}*/
				else if (i<(im_size-step-2) && (p[step]&1)==1 
							&& (p[(step+1)]&1)==1 && !(p[(step+2)]&1))
				{
					if (p[step]<thresh[0]) p[step]++;
				}
			}
			else if ((p[0]&1)==1 && i>=step && i<(im_size-(3*step)))
			{
				if ((p[step]&1)==1 && (p[(step+1)]&1)==1)
				{
					if ((p[(2*step)]&1)==1 && !(p[(3*step)]&1)) 
					{
						if (p[step]<thresh[0]) p[step]++;
					}
				}
			}
		}
	}
}

static
void offsetY_recons256_generic_part1(image_buffer *im, encode_state *enc,
	short thresh[2])
{
	int i, a, j;

	int im_size = im->fmt.end / 4;
	int step = im->fmt.tile_size;
	
	// Process LL:
	for (i=0,a=0;i<im_size;i+=step)
	{
		short *p = &im->im_process[i];
		short *q = &im->im_jpeg[i];

		for (a=i,j=0;j<(step/4);j++,a++, p++, q++) 
		{

			if (p[0]>10000) 
			{
				p[0]-=16000;q[0]=p[0];
				if (p[1]>0 && p[1]<256) q[1]=ROUND_2(p[1]);
				else q[1]=p[1];
				j++;a++; p++; q++;

				continue;
			}
			else if ((p[0]&1)==1 && a>i && (p[1]&1)==1 /*&& !(p[-1]&1)*/)
			{
				if (j<((step/4)-2) && (p[2]&1)==1 /*&& !(im->im_process[a+3]&1)*/)
				{
					if (abs(p[0]-p[2])> thresh[1]) p[1]++;
				}
				/*else if (j<((step/4)-4) && (p[2]&1)==1 && (p[3]&1)==1
						&& !(p[4]&1))
				{
					p[2]++;
				}*/
				else if (i<(im_size-step-2) && (p[step]&1)==1 
							&& (p[(step+1)]&1)==1 && !(p[(step+2)]&1))
				{
					if (p[step]<thresh[0]) p[step]++;
				}
			}
			else if ((p[0]&1)==1 && i>=step && i<(im_size-(3*step)))
			{
				if ((p[step]&1)==1 && (p[(step+1)]&1)==1)
				{
					if ((p[(2*step)]&1)==1 && !(p[(3*step)]&1)) 
					{
						if (p[step]<thresh[0]) p[step]++;
					}
				}
			}

			if (p[0]>0 && p[0]<256) q[0]=ROUND_2(p[0]);
			else q[0]=p[0];
		}
	}
}

static
void fixup_highres(image_buffer *im, short *highres_tmp)
{
	int i, j;
	int step = im->fmt.tile_size;
	int im_size = im->fmt.end / 4;

	for (i = 0; i < im_size; i += step)
	{
		short *p = &im->im_process[i];
		short *q = &im->im_jpeg[i];
		for (j = 0; j < (step / 4); j++, p++, q++) 
		{
			if (p[0] < 10000) {
				*highres_tmp = p[0];
				if (p[0]>=0 && p[0]<256) q[0]=ROUND_2(p[0]);
				else q[0]=p[0]; 
			} else {
				p[0]-=16000;
				*highres_tmp = p[0];
				q[0]=p[0];
			}
			highres_tmp++;
		}
	}
}

void offsetY_recons256(image_buffer *im, encode_state *enc, int m1, int part)
{
	int i,j,a;
	short *nhw1,*highres_tmp;
	int step = im->fmt.tile_size;
	int im_size = im->fmt.end / 4;
	short thresh[2];

	nhw1=(short*)im->im_process;

	if (im->setup->quality_setting>LOW3) {
		offsetY_recons256_q3(im, nhw1, part);
		thresh[0] = 10000;
		thresh[1] = 1;
	} else {
		thresh[0] = -32768;
		thresh[1] = 32767;
	}

	if (part) {
		offsetY_recons256_generic_part1(im, enc, thresh);
	} else {
		offsetY_recons256_generic_part0(im, enc, thresh);
		highres_tmp=(short*)malloc((im_size>>2)*sizeof(short));

		fixup_highres(im, highres_tmp);

		if (im->setup->quality_setting>LOW5)
		{
			for (i=0;i<enc->highres_mem_len;i++)
			{
				j=(enc->highres_mem[i]>>7);
				a=enc->highres_mem[i]&127;

				im->im_jpeg[(j<<im->fmt.tile_power)+a]=highres_tmp[enc->highres_mem[i]];
			}

			// XXX Don't free this here.
			// free(enc->highres_mem);
		}
		free(highres_tmp);
	}

	if (im->setup->quality_setting>LOW4)
	{
		// Tags pixels according to neighbour properties
		// Only this function is tagging by CODE_15xxx
		offsetY_recons256_q4(im->im_process, im->im_jpeg, im->fmt.end / 4, step, part);
	}

	for (i=0;i< im_size;i+=(step))
	{
		for (j=(step/4);j<step/2;j++) 
		{
			int skip;
			short *p = &im->im_process[i+j]; // FIXME: optimize
			short *q = &im->im_jpeg[i+j];
			skip = offsetY_subbands_H4(p, q, m1, j < step/2-1, part);
			j += skip;
		}
	}

	for (i=im_size;i<(2*im_size);i+=(step))
	{
		for (j=0;j<step/2;j++) 
		{
			int skip;
			short *p = &im->im_process[i+j];
			short *q = &im->im_jpeg[i+j];

			skip = offsetY_subbands_H4(p, q, m1, j < step/2-1, part);
			j += skip;


		}
	}

	if (!part)
	{
		// Run through all subbands except LL:
		for (i=(step);i<((2*im_size)-(step));i+=(step))
		{
			short *q = &im->im_jpeg[i + 1];

			for (j=1;j<step/2-1;j++,q++) 
			{

				if (abs(q[0])>=8)
				{
					if (abs(q[-(step+1)])>=8) continue;
					if (abs(q[-(step)])>=8) continue;
					if (abs(q[-(step-1)])>=8) continue;
					if (abs(q[-1])>=8) continue;
					if (abs(q[1])>=8) continue;
					if (abs(q[(step-1)])>=8) continue;
					if (abs(q[(step)])>=8) continue;
					if (abs(q[(step+1)])>=8) continue;

					if (i>=im_size || j>=(step/4))
					{
						if (q[0]>0) q[0]--;
						else q[0]++;
					}
				}
			}
		}
	}
}

static
void offsetUV_recons256_q5(short *dst, const short *src, int size, int step)
{
	int i;

#warning "Tiling incompatible"

	for (i=0;i<(size>>2);i++)
	{
		// TODO: Check boundary conditions
		if ((i&255)<(step))
		{
			if (!(i>>8)) { // FIXME
				*dst++=*src++;
				*dst++=ROUND_2(*src++);
			} else {
				*dst++=ROUND_2(*src++);
				*dst++=(*src++);
			}

			i++;
		} else { dst++; src++; }
	}
}

static inline
int offsetUV_fixup(short *p, short *q, int is_last, int comp, int m1)
{
	int a = p[0];

	if (a < 0) {
		if ((a==-7 || a==-8) && !comp)
		{
			if (!is_last && (p[1]==-7 || p[1]==-8)) 
			{
				q[0]=-11; q[1]=-11; return 1;
			}
		}
		// FIXME: Simplify
		a = -a;

		if (p[1] < 0 && p[1] > -8) {
			if ((a & 7) < 6) { a = ROUND_8(a); }
		} else {
			if ((a & 7) < 7) { a = ROUND_8(a); }
		}

		a = -a;
	}
	round_offset_limit_m1(a, q, m1);
	return 0;
}

void offsetUV_recons256(image_buffer *im, int m1, int comp)
{
	int i,j;

	int step = im->fmt.tile_size;
	int step2 = step / 2;
	int step4 = step / 4;
	int step8 = step / 8;
	int im_size = im->fmt.end / 4;
	int quad_size = im_size / 4;

	if (comp)
	{	
		if (im->setup->quality_setting>LOW5) 
		{
			offsetUV_recons256_q5(im->im_jpeg, im->im_process, im_size, step8);
		} else {
			short *p = im->im_process;
			short *q = im->im_jpeg;

			for (i=0;i<quad_size;i++) {
				if ((i & 255) < step8) { // FIXME
					*q =(*p & ~3) + 1;
				}
				p++; q++;
			}	
		}
	} else {
		short *p = im->im_process;
		short *q = im->im_jpeg;

		for (i=0;i<quad_size;i++) {
			if ((i & 255)<(step8)) {
				if (*p > 0 && *p < 256)
					*q = ROUND_2(*p);
				else
					*q = *p;
			}
			p++; q++;
		}
	}

	// Could be further optimized. For later..

	for (i = 0; i < quad_size; i += step2)
	{
		short *p = &im->im_process[i + step8];
		short *q = &im->im_jpeg[i + step8];

		for (j = (step8);j < (step4-1); j++) 
		{
			//if (a>10000) {im->im_jpeg[i+j]=7;im->im_jpeg[i+j+1]=7;continue;}
			//else if (a<-6 && a>-10) {im->im_jpeg[i+j]=-8;continue;}

			int skip;
			skip = offsetUV_fixup(p, q, 0, comp, m1);
			j += skip;
			skip++; p += skip; q += skip;
		}
		offsetUV_fixup(p, q, 1, comp, m1);
	}

	for (i = quad_size; i< 2*quad_size; i += step2)
	{
		short *p = &im->im_process[i];
		short *q = &im->im_jpeg[i];
		int skip;

		for (j = 0;j < (step4-1); j++) 
		{
			//if (a>10000) {im->im_jpeg[i+j]=7;im->im_jpeg[i+j+1]=7;continue;}
			//else if (a<-6 && a>-10) {im->im_jpeg[i+j]=-8;continue;}
			skip = offsetUV_fixup(p, q, 0, comp, m1);
			j += skip;
			skip++; p += skip; q += skip;
		}
		offsetUV_fixup(p, q, 1, comp, m1);
	}
}
