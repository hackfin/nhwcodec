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

char bmp_header[54];

#define CLIP(x) ( (x<0) ? 0 : ((x>255) ? 255 : x) );

// XXX This is for restructuring tests only:
#ifdef SWAPOUT
#define SWAPOUT_FUNCTION(f)  __CONCAT(orig,f)
#else
#define SWAPOUT_FUNCTION(f)  f
#endif

void encode_y(image_buffer *im, encode_state *enc, int ratio);


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
		*argv++;*argv++;arg=*argv++;

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
		*argv--;*argv--;*argv--;

		select=8; //for now...
	}

	menu(argv,&im,&enc,select);

	/* Encode Image */
	encode_image(&im,&enc,select);


	write_compressed_file(&im,&enc,argv);
	return 0;
}


void residual_coding_q2(short *nhw_process, short *res256, int res_uv)
{
	int stage;
	int i, j, count, scan;
	int Y, e;
	
	for (i=0,count=0,Y=0,e=0;i<(IM_SIZE>>1);i+=IM_DIM)
	{
		for (scan=i,j=0;j<(IM_DIM>>1);j++,scan++,count++)
		{
			if ((nhw_process[scan]-res256[count])>3 && (nhw_process[scan]-res256[count])<7)
			{
				if ((nhw_process[scan+1]-res256[count+1])>2 && (nhw_process[scan+1]-res256[count+1])<7)
				{
					if (abs(nhw_process[scan+(IM_DIM>>1)])<8) {nhw_process[scan+(IM_DIM>>1)]=12400;count++;scan++;j++;continue;}
					else if (abs(nhw_process[scan+(IM_SIZE>>1)])<8) {nhw_process[scan+(IM_SIZE>>1)]=12400;count++;scan++;j++;continue;}
					else if (abs(nhw_process[scan+(IM_SIZE>>1)+(IM_DIM>>1)])<8) {nhw_process[scan+(IM_SIZE>>1)+(IM_DIM>>1)]=12400;count++;scan++;j++;continue;}
				}
			}
			else if ((nhw_process[scan]-res256[count])<-3 && (nhw_process[scan]-res256[count])>-7)
			{
				if ((nhw_process[scan+1]-res256[count+1])<-2 && (nhw_process[scan+1]-res256[count+1])>-8)
				{
					if (abs(nhw_process[scan+(IM_DIM>>1)])<8) {nhw_process[scan+(IM_DIM>>1)]=12600;count++;scan++;j++;continue;}
					else if (abs(nhw_process[scan+(IM_SIZE>>1)])<8) {nhw_process[scan+(IM_SIZE>>1)]=12600;count++;scan++;j++;continue;}
					else if (abs(nhw_process[scan+(IM_SIZE>>1)+(IM_DIM>>1)])<8) {nhw_process[scan+(IM_SIZE>>1)+(IM_DIM>>1)]=12600;count++;scan++;j++;continue;}
				}
			}
			
			if (abs(nhw_process[scan]-res256[count])>res_uv) 
			{
				if ((nhw_process[scan]-res256[count])>0)
				{
					if (abs(nhw_process[scan+(IM_DIM>>1)])<8) nhw_process[scan+(IM_DIM>>1)]=12900;
					else if (abs(nhw_process[scan+(IM_SIZE>>1)])<8) nhw_process[scan+(IM_SIZE>>1)]=12900; 
					else if (abs(nhw_process[scan+(IM_SIZE>>1)+(IM_DIM>>1)])<8) nhw_process[scan+(IM_SIZE>>1)+(IM_DIM>>1)]=12900; 
				}
				else if ((nhw_process[scan]-res256[count])==-5)
				{
					if ((nhw_process[scan+1]-res256[count+1])<0)
					{
						if (abs(nhw_process[scan+(IM_DIM>>1)])<8) nhw_process[scan+(IM_DIM>>1)]=13000;
						else if (abs(nhw_process[scan+(IM_SIZE>>1)])<8) nhw_process[scan+(IM_SIZE>>1)]=13000; 
						else if (abs(nhw_process[scan+(IM_SIZE>>1)+(IM_DIM>>1)])<8) nhw_process[scan+(IM_SIZE>>1)+(IM_DIM>>1)]=13000; 
					}
					
				}
				else
				{
					if (abs(nhw_process[scan+(IM_DIM>>1)])<8) nhw_process[scan+(IM_DIM>>1)]=13000;
					else if (abs(nhw_process[scan+(IM_SIZE>>1)])<8) nhw_process[scan+(IM_SIZE>>1)]=13000; 
					else if (abs(nhw_process[scan+(IM_SIZE>>1)+(IM_DIM>>1)])<8) nhw_process[scan+(IM_SIZE>>1)+(IM_DIM>>1)]=13000; 
				}
			}
		}
	}
}

#define CODE_16000 16000
#define CODE_24000 24000


void preprocess14(const short *nhw_process, short *result)
{
	int stage;
	int i, j, count, scan;
	int s = 2 * IM_DIM; // scan line size
	int halfn = IM_DIM;
	int tmp = 2 * halfn + 1;

	for (i=0,count=0;i<(4*IM_SIZE>>1);i+=s,count+=halfn)
	{
		for (scan=i,j=0;j<halfn;j++,scan++) 
		{
			if (i>=IM_SIZE || j>=(halfn>>1))
			{
				stage=nhw_process[scan];

				if (stage<-7)
				{
					if (((-stage)&7)==7)    result[count+j]+=16000;
					else if (!((-stage)&7)) result[count+j]+=16000;
				}
				else if (stage<-4) result[count+j]+=12000;
				else if (stage>=0)
				{
					if (stage>=2 && stage<5) 
					{
						if (scan>=tmp && (i+j)<(2*IM_SIZE-2*halfn-1))
						{
							if (abs(nhw_process[scan-tmp])!=0 || abs(nhw_process[scan+tmp])!=0)
							{
								result[count+j]+=12000;
							}
							//else result[count+j]+=8000;
						}
					}
					else if (!(stage&7)) result[count+j]+=12000;
					else if ((stage&7)==1) result[count+j]+=12000;
					else if (stage>4 && stage<=7) result[count+j]+=16000;
				}
			}
		}
	}
}


void postprocess14(image_buffer *im, short *nhw_process, short *res256)
{
	int i, j, count, scan;
	int a, e;

	for (i=0,count=0;i<(4*IM_SIZE>>1);i+=(2*IM_DIM),count+=IM_DIM)
	{
		for (j=0;j<IM_DIM;j++) 
		{
			if (res256[count+j]>14000) 
			{
				res256[count+j]-=16000;
				if (i<IM_SIZE && j>=(IM_DIM>>1)) nhw_process[(i>>8)+((j-(IM_DIM>>1))<<10)+(2*IM_DIM)]++;
				else if (i>=IM_SIZE && j<(IM_DIM>>1)) nhw_process[((i-IM_SIZE)>>8)+(j<<10)+1]++; 
				else if (i>=IM_SIZE && j>=(IM_DIM>>1)) nhw_process[((i-IM_SIZE)>>8)+((j-(IM_DIM>>1))<<10)+(2*IM_DIM+1)]++; 
			}
			else if (res256[count+j]>10000) 
			{
				res256[count+j]-=12000;
				if (i<IM_SIZE && j>=(IM_DIM>>1)) nhw_process[(i>>8)+((j-(IM_DIM>>1))<<10)+(2*IM_DIM)]--;
				else if (i>=IM_SIZE && j<(IM_DIM>>1)) nhw_process[((i-IM_SIZE)>>8)+(j<<10)+1]--; 
				else if (i>=IM_SIZE && j>=(IM_DIM>>1)) nhw_process[((i-IM_SIZE)>>8)+((j-(IM_DIM>>1))<<10)+(2*IM_DIM+1)]--;
			}
		}
	}

	for (i=0,count=0;i<(4*IM_SIZE>>1);i+=(2*IM_DIM))
	{
		for (e=i,j=0;j<IM_DIM;j++,count++,e++)
		{
			scan=nhw_process[e]-res256[count];

			if(scan>11) {im->im_jpeg[e]=res256[count]-7;nhw_process[e]-=7;}
			else if(scan>7) {im->im_jpeg[e]=res256[count]-4;nhw_process[e]-=4;}
			else if(scan>5) {im->im_jpeg[e]=res256[count]-2;nhw_process[e]-=2;}
			else if(scan>4) {im->im_jpeg[e]=res256[count]-1;nhw_process[e]--;}
			else if (scan<-11) {im->im_jpeg[e]=res256[count]+7;nhw_process[e]+=7;}
			else if (scan<-7) {im->im_jpeg[e]=res256[count]+4;nhw_process[e]+=4;}
			else if (scan<-5) {im->im_jpeg[e]=res256[count]+2;nhw_process[e]+=2;}
			else if (scan<-4) {im->im_jpeg[e]=res256[count]+1;nhw_process[e]++;}
			else if (abs(scan)>1)
			{
				a=(nhw_process[e+1]-res256[count+1]);
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

				a+=(nhw_process[e-1]-res256[count-1]);

				if (scan>=4 && a>=1) {im->im_jpeg[e]=res256[count]-1;nhw_process[e]--;}
				else if (scan<=-4 && a<=-1) {im->im_jpeg[e]=res256[count]+1;nhw_process[e]++;}
				else if (scan==3 && a>=0) {im->im_jpeg[e]=res256[count]-1;nhw_process[e]--;}
				else if (scan==-3 && a<=0) {im->im_jpeg[e]=res256[count]+1;nhw_process[e]++;}
				else if (abs(a)>=3) 
				{
					if (scan>0 && a>0) {im->im_jpeg[e]=res256[count]-1;nhw_process[e]--;}
					else if (scan<0 && a<0) {im->im_jpeg[e]=res256[count]+1;nhw_process[e]++;}
					else if (a>=5) {im->im_jpeg[e]=res256[count]-2;nhw_process[e]-=2;}
					else if (a<=-5) {im->im_jpeg[e]=res256[count]+2;nhw_process[e]+=2;}
					else if (a>=4)
					{
						im->im_jpeg[e]=res256[count]-1;nhw_process[e]--;
					}
					else if (a<=-4)
					{
						im->im_jpeg[e]=res256[count]+1;nhw_process[e]++;
					}
					else im->im_jpeg[e]=res256[count];
				}
				else im->im_jpeg[e]=res256[count];
			}
			else im->im_jpeg[e]=res256[count];
		}
	}
}

void preprocess_q7(int quality, short *nhw_process, const char *wvlt)
{
	int i, j, count, scan;
	int a, e;


	for (i=0,scan=0;i<(IM_SIZE);i+=(2*IM_DIM)) {
		for (scan=i,j=0;j<(IM_DIM>>1)-4;j++,scan++)
		{
			if (abs(nhw_process[scan+4]-nhw_process[scan])<wvlt[0] &&
				abs(nhw_process[scan+4]-nhw_process[scan+3])<wvlt[0] && abs(nhw_process[scan+1]-nhw_process[scan])<wvlt[0] && 
				abs(nhw_process[scan+3]-nhw_process[scan+1])<wvlt[0] && abs(nhw_process[scan+3]-nhw_process[scan+2])<(wvlt[1]-2))
			{
					
					if ((nhw_process[scan+3]-nhw_process[scan+1])>5 && (nhw_process[scan+2]-nhw_process[scan+3]>=0))
					{
						nhw_process[scan+2]=nhw_process[scan+3];
					}
					else if ((nhw_process[scan+1]-nhw_process[scan+3])>5 && (nhw_process[scan+2]-nhw_process[scan+3]<=0))
					{
						nhw_process[scan+2]=nhw_process[scan+3];
					}
					else if ((nhw_process[scan+1]-nhw_process[scan+3])>5 && (nhw_process[scan+2]-nhw_process[scan+1]>=0))
					{
						nhw_process[scan+2]=nhw_process[scan+1];
					}
					else if ((nhw_process[scan+3]-nhw_process[scan+1])>5 && (nhw_process[scan+2]-nhw_process[scan+1]<=0))
					{
						nhw_process[scan+2]=nhw_process[scan+1];
					}
					else if ((nhw_process[scan+3]-nhw_process[scan+2])>0 && (nhw_process[scan+2]-nhw_process[scan+1]>0))
					{
					}
					else if ((nhw_process[scan+1]-nhw_process[scan+2])>0 && (nhw_process[scan+2]-nhw_process[scan+3]>0))
					{
					}
					else nhw_process[scan+2]=(nhw_process[scan+3]+nhw_process[scan+1])>>1;
						
					for (count=1;count<4;count++)
					{
						if (abs(nhw_process[((scan+count)<<1)+IM_DIM])<wvlt[5]) nhw_process[((scan+count)<<1)+IM_DIM]=0;
						if (abs(nhw_process[((scan+count)<<1)+IM_DIM+1])<wvlt[5]) nhw_process[((scan+count)<<1)+IM_DIM+1]=0;
						if (abs(nhw_process[((scan+count)<<1)+(3*IM_DIM)])<wvlt[5]) nhw_process[((scan+count)<<1)+(3*IM_DIM)]=0;
						if (abs(nhw_process[((scan+count)<<1)+(3*IM_DIM)+1])<wvlt[5]) nhw_process[((scan+count)<<1)+(3*IM_DIM)+1]=0;
						
						if (abs(nhw_process[((scan+count)<<1)+(2*IM_SIZE)])<(wvlt[5]+6)) nhw_process[((scan+count)<<1)+(2*IM_SIZE)]=0;
						if (abs(nhw_process[((scan+count)<<1)+(2*IM_SIZE)+1])<(wvlt[5]+6)) nhw_process[((scan+count)<<1)+(2*IM_SIZE)+1]=0;
						if (abs(nhw_process[((scan+count)<<1)+(2*IM_SIZE)+(2*IM_DIM)])<(wvlt[5]+6)) nhw_process[((scan+count)<<1)+(2*IM_SIZE)+(2*IM_DIM)]=0;
						if (abs(nhw_process[((scan+count)<<1)+(2*IM_SIZE)+(2*IM_DIM)+1])<(wvlt[5]+6)) nhw_process[((scan+count)<<1)+(2*IM_SIZE)+(2*IM_DIM)+1]=0;
						
						e=(2*IM_SIZE)+IM_DIM;
						if (abs(nhw_process[((scan+count)<<1)+e])<wvlt[4]) nhw_process[((scan+count)<<1)+e]=0;
						if (abs(nhw_process[((scan+count)<<1)+e+1])<wvlt[4]) nhw_process[((scan+count)<<1)+e+1]=0;
						if (abs(nhw_process[((scan+count)<<1)+e+(2*IM_DIM)])<wvlt[4]) nhw_process[((scan+count)<<1)+e+(2*IM_DIM)]=0;
						if (abs(nhw_process[((scan+count)<<1)+e+(2*IM_DIM)+1])<wvlt[4]) nhw_process[((scan+count)<<1)+e+(2*IM_DIM)+1]=0;
					}
						
					if (quality<=LOW9)
					{
						for (count=1;count<4;count++)
						{
							if (abs(nhw_process[scan+count+(IM_DIM>>1)])<11) nhw_process[scan+count+(IM_DIM>>1)]=0;
							if (abs(nhw_process[scan+count+IM_SIZE])<12) nhw_process[scan+count+IM_SIZE]=0;
							if (abs(nhw_process[scan+count+IM_SIZE+(IM_DIM>>1)])<13) nhw_process[scan+count+IM_SIZE+(IM_DIM>>1)]=0;
						}
					}
						
			}
			else if (abs(nhw_process[scan+4]-nhw_process[scan])<(wvlt[1]+1) && abs(nhw_process[scan+4]-nhw_process[scan+3])<(wvlt[1]+1) && abs(nhw_process[scan+1]-nhw_process[scan])<(wvlt[1]+1))
			{
				if (abs(nhw_process[scan+3]-nhw_process[scan+1])<(wvlt[1]+6) && abs(nhw_process[scan+3]-nhw_process[scan+2])<(wvlt[1]+6))
				{
					if (((nhw_process[scan+3]-nhw_process[scan+2])>=0 && (nhw_process[scan+2]-nhw_process[scan+1])>=0) ||
						((nhw_process[scan+3]-nhw_process[scan+2])<=0 && (nhw_process[scan+2]-nhw_process[scan+1])<=0)) 
					{
						//nhw_process[scan+2]=(nhw_process[scan+3]+nhw_process[scan+1]+1)>>1;
						
						for (count=1;count<4;count++)
						{
							if (abs(nhw_process[((scan+count)<<1)+IM_DIM])<wvlt[5]) nhw_process[((scan+count)<<1)+IM_DIM]=0;
							if (abs(nhw_process[((scan+count)<<1)+IM_DIM+1])<wvlt[5]) nhw_process[((scan+count)<<1)+IM_DIM+1]=0;
							if (abs(nhw_process[((scan+count)<<1)+(3*IM_DIM)])<wvlt[5]) nhw_process[((scan+count)<<1)+(3*IM_DIM)]=0;
							if (abs(nhw_process[((scan+count)<<1)+(3*IM_DIM)+1])<wvlt[5]) nhw_process[((scan+count)<<1)+(3*IM_DIM)+1]=0;
						
							if (abs(nhw_process[((scan+count)<<1)+(2*IM_SIZE)])<(wvlt[5]+6)) nhw_process[((scan+count)<<1)+(2*IM_SIZE)]=0;
							if (abs(nhw_process[((scan+count)<<1)+(2*IM_SIZE)+1])<(wvlt[5]+6)) nhw_process[((scan+count)<<1)+(2*IM_SIZE)+1]=0;
							if (abs(nhw_process[((scan+count)<<1)+(2*IM_SIZE)+(2*IM_DIM)])<(wvlt[5]+6)) nhw_process[((scan+count)<<1)+(2*IM_SIZE)+(2*IM_DIM)]=0;
							if (abs(nhw_process[((scan+count)<<1)+(2*IM_SIZE)+(2*IM_DIM)+1])<(wvlt[5]+6)) nhw_process[((scan+count)<<1)+(2*IM_SIZE)+(2*IM_DIM)+1]=0;
						
							e=(2*IM_SIZE)+IM_DIM;
							if (abs(nhw_process[((scan+count)<<1)+e])<wvlt[4]) nhw_process[((scan+count)<<1)+e]=0;
							if (abs(nhw_process[((scan+count)<<1)+e+1])<wvlt[4]) nhw_process[((scan+count)<<1)+e+1]=0;
							if (abs(nhw_process[((scan+count)<<1)+e+(2*IM_DIM)])<wvlt[4]) nhw_process[((scan+count)<<1)+e+(2*IM_DIM)]=0;
							if (abs(nhw_process[((scan+count)<<1)+e+(2*IM_DIM)+1])<wvlt[4]) nhw_process[((scan+count)<<1)+e+(2*IM_DIM)+1]=0;
						}
						
						if (quality<=LOW9)
						{
							for (count=1;count<4;count++)
							{
								if (abs(nhw_process[scan+count+(IM_DIM>>1)])<11) nhw_process[scan+count+(IM_DIM>>1)]=0;
								if (abs(nhw_process[scan+count+IM_SIZE])<12) nhw_process[scan+count+IM_SIZE]=0;
								if (abs(nhw_process[scan+count+IM_SIZE+(IM_DIM>>1)])<13) nhw_process[scan+count+IM_SIZE+(IM_DIM>>1)]=0;
							}
						}
					}
				}
			}
		}
	}

	for (i=0,scan=0;i<(IM_SIZE)-(4*IM_DIM);i+=(2*IM_DIM))
	{
		for (scan=i,j=0;j<(IM_DIM>>1)-2;j++,scan++)
		{
			if (abs(nhw_process[scan+1]-nhw_process[scan+(4*IM_DIM)+1])<wvlt[2] && abs(nhw_process[scan+(2*IM_DIM)]-nhw_process[scan+(2*IM_DIM)+2])<wvlt[2])
			{
				if (abs(nhw_process[scan+(2*IM_DIM)+1]-nhw_process[scan+(2*IM_DIM)])<(wvlt[3]-1) && abs(nhw_process[scan+1]-nhw_process[scan+(2*IM_DIM)+1])<wvlt[3])
				{
					e=(nhw_process[scan+1]+nhw_process[scan+(4*IM_DIM)+1]+nhw_process[scan+(2*IM_DIM)]+nhw_process[scan+(2*IM_DIM)+2]+2)>>2;
						
					if (abs(e-nhw_process[scan+(2*IM_DIM)])<5 || abs(e-nhw_process[scan+(2*IM_DIM)+2])<5)
					{
						nhw_process[scan+(2*IM_DIM)+1]=e;
					}
					
					count=scan+(2*IM_DIM)+1;
						
					if (abs(nhw_process[(count<<1)+IM_DIM])<wvlt[5]) nhw_process[(count<<1)+IM_DIM]=0;
					if (abs(nhw_process[(count<<1)+IM_DIM+1])<wvlt[5]) nhw_process[(count<<1)+IM_DIM+1]=0;
					if (abs(nhw_process[(count<<1)+(3*IM_DIM)])<wvlt[5]) nhw_process[(count<<1)+(3*IM_DIM)]=0;
					if (abs(nhw_process[(count<<1)+(3*IM_DIM)+1])<wvlt[5]) nhw_process[(count<<1)+(3*IM_DIM)+1]=0;
						
					if (abs(nhw_process[(count<<1)+(2*IM_SIZE)])<(wvlt[5]+6)) nhw_process[(count<<1)+(2*IM_SIZE)]=0;
					if (abs(nhw_process[(count<<1)+(2*IM_SIZE)+1])<(wvlt[5]+6)) nhw_process[(count<<1)+(2*IM_SIZE)+1]=0;
					if (abs(nhw_process[(count<<1)+(2*IM_SIZE)+(2*IM_DIM)])<(wvlt[5]+6)) nhw_process[(count<<1)+(2*IM_SIZE)+(2*IM_DIM)]=0;
					if (abs(nhw_process[(count<<1)+(2*IM_SIZE)+(2*IM_DIM)+1])<(wvlt[5]+6)) nhw_process[(count<<1)+(2*IM_SIZE)+(2*IM_DIM)+1]=0;
						
					e=(2*IM_SIZE)+IM_DIM;
					if (abs(nhw_process[(count<<1)+e])<32) nhw_process[(count<<1)+e]=0;
					if (abs(nhw_process[(count<<1)+e+1])<32) nhw_process[(count<<1)+e+1]=0;
					if (abs(nhw_process[(count<<1)+e+(2*IM_DIM)])<32) nhw_process[(count<<1)+e+(2*IM_DIM)]=0;
					if (abs(nhw_process[(count<<1)+e+(2*IM_DIM)+1])<32) nhw_process[(count<<1)+e+(2*IM_DIM)+1]=0;
					
					if (quality<=LOW9)
					{
						for (e=0;e<3;e++)
						{
							if (abs(nhw_process[count+e-1+(IM_DIM>>1)])<11) nhw_process[count+e-1+(IM_DIM>>1)]=0;
							if (abs(nhw_process[count+e-1+IM_SIZE])<12) nhw_process[count+e-1+IM_SIZE]=0;
							if (abs(nhw_process[count+e-1+IM_SIZE+(IM_DIM>>1)])<13) nhw_process[count+e-1+IM_SIZE+(IM_DIM>>1)]=0;
						}
					}
				}
			}
		}
	}
	
	for (i=0,scan=0;i<(IM_SIZE)-(4*IM_DIM);i+=(2*IM_DIM))
	{
		for (scan=i,j=0;j<(IM_DIM>>1)-2;j++,scan++)
		{
			if (abs(nhw_process[scan+2]-nhw_process[scan+1])<wvlt[2] && abs(nhw_process[scan+1]-nhw_process[scan])<wvlt[2])
			{
				if (abs(nhw_process[scan]-nhw_process[scan+(2*IM_DIM)])<wvlt[2] && abs(nhw_process[scan+2]-nhw_process[scan+(2*IM_DIM)+2])<wvlt[2])
				{
					if (abs(nhw_process[scan+(4*IM_DIM)+1]-nhw_process[scan+(2*IM_DIM)])<wvlt[2] && abs(nhw_process[scan+(2*IM_DIM)]-nhw_process[scan+(2*IM_DIM)+1])<wvlt[3]) 
					{
						e=(nhw_process[scan+1]+nhw_process[scan+(4*IM_DIM)+1]+nhw_process[scan+(2*IM_DIM)]+nhw_process[scan+(2*IM_DIM)+2]+1)>>2;
						
						if (abs(e-nhw_process[scan+(2*IM_DIM)])<5 || abs(e-nhw_process[scan+(2*IM_DIM)+2])<5)
						{
							nhw_process[scan+(2*IM_DIM)+1]=e;
						}
						
						count=scan+(2*IM_DIM)+1;
						
						if (abs(nhw_process[(count<<1)+IM_DIM])<wvlt[5]) nhw_process[(count<<1)+IM_DIM]=0;
						if (abs(nhw_process[(count<<1)+IM_DIM+1])<wvlt[5]) nhw_process[(count<<1)+IM_DIM+1]=0;
						if (abs(nhw_process[(count<<1)+(3*IM_DIM)])<wvlt[5]) nhw_process[(count<<1)+(3*IM_DIM)]=0;
						if (abs(nhw_process[(count<<1)+(3*IM_DIM)+1])<wvlt[5]) nhw_process[(count<<1)+(3*IM_DIM)+1]=0;
						
						if (abs(nhw_process[(count<<1)+(2*IM_SIZE)])<(wvlt[5]+6)) nhw_process[(count<<1)+(2*IM_SIZE)]=0;
						if (abs(nhw_process[(count<<1)+(2*IM_SIZE)+1])<(wvlt[5]+6)) nhw_process[(count<<1)+(2*IM_SIZE)+1]=0;
						if (abs(nhw_process[(count<<1)+(2*IM_SIZE)+(2*IM_DIM)])<(wvlt[5]+6)) nhw_process[(count<<1)+(2*IM_SIZE)+(2*IM_DIM)]=0;
						if (abs(nhw_process[(count<<1)+(2*IM_SIZE)+(2*IM_DIM)+1])<(wvlt[5]+6)) nhw_process[(count<<1)+(2*IM_SIZE)+(2*IM_DIM)+1]=0;
						
						e=(2*IM_SIZE)+IM_DIM;
						if (abs(nhw_process[(count<<1)+e])<32) nhw_process[(count<<1)+e]=0;
						if (abs(nhw_process[(count<<1)+e+1])<32) nhw_process[(count<<1)+e+1]=0;
						if (abs(nhw_process[(count<<1)+e+(2*IM_DIM)])<32) nhw_process[(count<<1)+e+(2*IM_DIM)]=0;
						if (abs(nhw_process[(count<<1)+e+(2*IM_DIM)+1])<32) nhw_process[(count<<1)+e+(2*IM_DIM)+1]=0;
					}
					
					if (quality<=LOW9)
					{
						for (e=0;e<3;e++)
						{
							if (abs(nhw_process[count+e-1+(IM_DIM>>1)])<11) nhw_process[count+e-1+(IM_DIM>>1)]=0;
							if (abs(nhw_process[count+e-1+IM_SIZE])<12) nhw_process[count+e-1+IM_SIZE]=0;
							if (abs(nhw_process[count+e-1+IM_SIZE+(IM_DIM>>1)])<13) nhw_process[count+e-1+IM_SIZE+(IM_DIM>>1)]=0;
						}
					}
				}
			}
		}
	}
}

void preprocess_q9(short *nhw_process, const char *wvlt)
{

	int i, j, count, scan;
	int e;
	for (i=0,scan=0;i<(IM_SIZE);i+=(2*IM_DIM))
	{
		for (scan=i,j=0;j<(IM_DIM>>1)-2;j++,scan++)
		{
			if (abs(nhw_process[scan+2]-nhw_process[scan+1])<wvlt[6] && abs(nhw_process[scan+2]-nhw_process[scan])<wvlt[6]  && abs(nhw_process[scan+1]-nhw_process[scan])<wvlt[6])
			{
				count=scan+1;
					
				if (abs(nhw_process[(count<<1)+IM_DIM])<wvlt[5]) nhw_process[(count<<1)+IM_DIM]=0;
				if (abs(nhw_process[(count<<1)+IM_DIM+1])<wvlt[5]) nhw_process[(count<<1)+IM_DIM+1]=0;
				if (abs(nhw_process[(count<<1)+(3*IM_DIM)])<wvlt[5]) nhw_process[(count<<1)+(3*IM_DIM)]=0;
				if (abs(nhw_process[(count<<1)+(3*IM_DIM)+1])<wvlt[5]) nhw_process[(count<<1)+(3*IM_DIM)+1]=0;
					
				if (abs(nhw_process[(count<<1)+(2*IM_SIZE)])<(wvlt[5]+6)) nhw_process[(count<<1)+(2*IM_SIZE)]=0;
				if (abs(nhw_process[(count<<1)+(2*IM_SIZE)+1])<(wvlt[5]+6)) nhw_process[(count<<1)+(2*IM_SIZE)+1]=0;
				if (abs(nhw_process[(count<<1)+(2*IM_SIZE)+(2*IM_DIM)])<(wvlt[5]+6)) nhw_process[(count<<1)+(2*IM_SIZE)+(2*IM_DIM)]=0;
				if (abs(nhw_process[(count<<1)+(2*IM_SIZE)+(2*IM_DIM)+1])<(wvlt[5]+6)) nhw_process[(count<<1)+(2*IM_SIZE)+(2*IM_DIM)+1]=0;
					
				e=(2*IM_SIZE)+IM_DIM;
				if (abs(nhw_process[(count<<1)+e])<34) nhw_process[(count<<1)+e]=0;
				if (abs(nhw_process[(count<<1)+e+1])<34) nhw_process[(count<<1)+e+1]=0;
				if (abs(nhw_process[(count<<1)+e+(2*IM_DIM)])<34) nhw_process[(count<<1)+e+(2*IM_DIM)]=0;
				if (abs(nhw_process[(count<<1)+e+(2*IM_DIM)+1])<34) nhw_process[(count<<1)+e+(2*IM_DIM)+1]=0;
				
				//for (e=0;e<3;e++)
				//{
						if (abs(nhw_process[count+(IM_DIM>>1)])<11) nhw_process[count+(IM_DIM>>1)]=0;
						if (abs(nhw_process[count+IM_SIZE])<12) nhw_process[count+IM_SIZE]=0;
						if (abs(nhw_process[count+IM_SIZE+(IM_DIM>>1)])<13) nhw_process[count+IM_SIZE+(IM_DIM>>1)]=0;
				//}
			}
		}
	}
}

void compress1(int quality, short *nhw_process, const char *wvlt, encode_state *enc)
{
	int i, j, count, scan;
	int Y, stage, res;
	int a, e;

	for (i=0,a=0,e=0,count=0,res=0,Y=0,stage=0;i<((4*IM_SIZE)>>2);i+=(2*IM_DIM))
	{
		for (count=i,j=0;j<((2*IM_DIM)>>2);j++,count++)
		{
			scan=nhw_process[count];

			if (quality>LOW3 && scan>10000) 
			{
				if (scan>20000) 
				{
					scan-=24000;enc->nhw_res4[res++]=j+1;stage++;
				}
				else scan-=16000;
			}
			/*else if (im->setup->quality_setting<LOW3 && j>0 && j<((IM_DIM>>1)-1) && abs(nhw_process[count-1]-nhw_process[count+1])<1 && abs(nhw_process[count-1]-scan)<=5)
		 	{
				scan=nhw_process[count-1];//nhw_process[count+1]=nhw_process[count-1];
			}*/
			else if ((scan&1)==1 && count>i && (nhw_process[count+1]&1)==1 /*&& !(nhw_process[count-1]&1)*/)
			{
				if (j<((IM_DIM>>1)-2) && (nhw_process[count+2]&1)==1 /*&& !(nhw_process[count+3]&1)*/) 
				{
					if (abs(scan-nhw_process[count+2])>1 && quality>LOW3) nhw_process[count+1]++;
				}
				/*else if (j<((IM_DIM>>1)-4) && (nhw_process[count+2]&1)==1 && (nhw_process[count+3]&1)==1
						&& !(nhw_process[count+4]&1)) 
				{
					nhw_process[count+2]++;
				}*/
				else if (i<(IM_SIZE-(2*IM_DIM)-2) && (nhw_process[count+(2*IM_DIM)]&1)==1
							&& (nhw_process[count+(2*IM_DIM+1)]&1)==1 && !(nhw_process[count+(2*IM_DIM+2)]&1))
				{
					if (nhw_process[count+(2*IM_DIM)]<10000 && quality>LOW3) 
					{
						nhw_process[count+(2*IM_DIM)]++;
					}
				}
			}
			else if ((scan&1)==1 && i>=(2*IM_DIM) && i<(IM_SIZE-(6*IM_DIM)))
			{
				if ((nhw_process[count+(2*IM_DIM)]&1)==1 && (nhw_process[count+(2*IM_DIM+1)]&1)==1)
				{
					if ((nhw_process[count+(4*IM_DIM)]&1)==1 && !(nhw_process[count+(6*IM_DIM)]&1)) 
					{
						if (nhw_process[count+(2*IM_DIM)]<10000 && quality>LOW3) 
						{
							nhw_process[count+(2*IM_DIM)]++;
						}
					}
				}
			}

			if (scan>255 && (j>0 || i>0))
			{
				enc->exw_Y[e++]=(i>>9);	enc->exw_Y[e++]=j+128;
				Y=scan-255;if (Y>255) Y=255;enc->exw_Y[e++]=Y;
				enc->tree1[a]=enc->tree1[a-1];enc->ch_res[a]=enc->tree1[a-1];a++;nhw_process[count]=0;

			}
			else if (scan<0 && (j>0 || i>0))  
			{
				enc->exw_Y[e++]=(i>>9);enc->exw_Y[e++]=j;
				if (scan<-255) scan=-255;
				enc->exw_Y[e++]=-scan;
				enc->tree1[a]=enc->tree1[a-1];enc->ch_res[a]=enc->tree1[a-1];a++;nhw_process[count]=0;
			}
			else 
			{
				if (scan>255) scan=255;else if (scan<0) scan=0;
				enc->ch_res[a]=scan;enc->tree1[a++]=scan&254;nhw_process[count]=0;
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

const char quality_special_default[7] = { 16, 28, 11, 8, 5, 0xff, 0xff };
const char quality_special12500[7] = { 19, 31, 13, 9, 6, 0xff, 0xff };
const char quality_special10000[7] = { 18, 30, 12, 8, 6, 0xff, 0xff };
const char quality_special7000[7] = { 17, 29, 11, 8, 5, 0xff, 0xff };

void compress_s2(int quality, short *resIII, short *nhw_process, char *wvlt, encode_state *enc, int ratio)
{
	int i, j, e, scan, count;
	int res = 0;

	if (quality<NORM && quality>LOW5)
	{
		for (i=(2*IM_SIZE);i<(4*IM_SIZE);i+=(2*IM_DIM))
		{
			for (scan=i,j=0;j<IM_DIM;j++,scan++)
			{
				if (abs(nhw_process[scan])>=ratio && abs(nhw_process[scan])<9) 
				{	
					 if (nhw_process[scan]>0) nhw_process[scan]=7;else nhw_process[scan]=-7;	
				}
			}

			for (scan=i+(IM_DIM),j=(IM_DIM);j<(2*IM_DIM);j++,scan++)
			{
				if (abs(nhw_process[scan])>=ratio && abs(nhw_process[scan])<=14) 
				{	
					 if (nhw_process[scan]>0) nhw_process[scan]=7;else nhw_process[scan]=-7;	
				}
			}
		}
	}
	else if (quality<=LOW5 && quality>=LOW6)
	{ 
		wvlt[0] = 11;

		if (quality==LOW5) wvlt[1]=19;
		else if (quality==LOW6) wvlt[1]=20;

		for (i=(2*IM_SIZE);i<(4*IM_SIZE);i+=(2*IM_DIM))
		{
			for (scan=i,j=0;j<(IM_DIM);j++,scan++)
			{		
				if (abs(nhw_process[scan])>=ratio && abs(nhw_process[scan])<wvlt[0]) 
				{	
					nhw_process[scan]=0;			
				}
			}

			for (scan=i+(IM_DIM),j=(IM_DIM);j<(2*IM_DIM);j++,scan++)
			{
				if (abs(nhw_process[scan])>=ratio && abs(nhw_process[scan])<wvlt[1]) 
				{	
					if (nhw_process[scan]>=14) nhw_process[scan]=7;
					else if (nhw_process[scan]<=-14) nhw_process[scan]=-7;	
					else 	nhw_process[scan]=0;	
				}
			}
		}
	}
	else if (quality<LOW6)
	{ 
		/*
		if (quality<=LOW7) {wvlt[0]=15;wvlt[1]=27;wvlt[2]=10;wvlt[3]=6;wvlt[4]=3;}

		else */
		if (quality<=LOW8)
		{
			for (i=(2*IM_SIZE),count=0;i<(4*IM_SIZE);i++)
			{
				if (abs(nhw_process[i])>=12) count++;
			}
			
			//if (count>15000) {wvlt[0]=20;wvlt[1]=32;wvlt[2]=13;wvlt[3]=8;wvlt[4]=5;}
			if (count>12500)      COPY_WVLT(wvlt, quality_special12500)
			else if (count>10000) COPY_WVLT(wvlt, quality_special10000)
			else if (count>=7000) COPY_WVLT(wvlt, quality_special7000)
			else                  COPY_WVLT(wvlt, quality_special_default);
			
			if (quality==LOW9) 
			{
				if (count>12500) 
				{
					wvlt[0]++;wvlt[1]++;wvlt[2]++;wvlt[3]++;wvlt[4]++;
				}
				else wvlt[0]++;
			}
			else if (quality<=LOW10) 
			{
				if (count>12500) 
				{
					wvlt[0]+=3;wvlt[1]+=3;wvlt[2]+=2;wvlt[3]+=3;wvlt[4]+=3;
				}
				else 
				{
					wvlt[0]+=3;wvlt[1]+=2;wvlt[2]+=2;wvlt[3]+=2;wvlt[4]+=2;
				}
			}
		}
		
		for (i=0;i<(2*IM_SIZE);i+=(2*IM_DIM))
		{
			for (scan=i+IM_DIM,j=IM_DIM;j<(2*IM_DIM);j++,scan++)
			{
				if (abs(nhw_process[scan])>=ratio &&  abs(nhw_process[scan])<(wvlt[2]+2)) 
				{	
					if (abs(resIII[(((i>>1)+(j-IM_DIM))>>1)+(IM_DIM>>1)])<wvlt[3]) nhw_process[scan]=0;
					else if (abs(nhw_process[scan]+nhw_process[scan-1])<wvlt[4] && abs(nhw_process[scan+1])<wvlt[4]) 
					{
						nhw_process[scan]=0;nhw_process[scan-1]=0;
					}
					else if (abs(nhw_process[scan]+nhw_process[scan+1])<wvlt[4] && abs(nhw_process[scan-1])<wvlt[4]) 
					{
						nhw_process[scan]=0;nhw_process[scan+1]=0;
					}
				}
				
				if (abs(nhw_process[scan])>=ratio &&  abs(nhw_process[scan])<wvlt[2]) 
				{	
					if (abs(nhw_process[scan-1])<ratio && abs(nhw_process[scan+1])<ratio) 
					{
						nhw_process[scan]=0;
					}
				}
			}
		}

		for (i=(2*IM_SIZE);i<(4*IM_SIZE);i+=(2*IM_DIM))
		{
			for (scan=i,j=0;j<(IM_DIM);j++,scan++)
			{
				if (abs(nhw_process[scan])>=ratio &&  abs(nhw_process[scan])<(wvlt[0]+2)) 
				{	
					if (abs(resIII[((((i-(2*IM_SIZE))>>1)+j)>>1)+(IM_SIZE>>1)])<wvlt[3]) nhw_process[scan]=0;
					else if (abs(nhw_process[scan]+nhw_process[scan-1])<wvlt[4] && abs(nhw_process[scan+1])<wvlt[4]) 
					{
						nhw_process[scan]=0;nhw_process[scan-1]=0;
					}
					else if (abs(nhw_process[scan]+nhw_process[scan+1])<wvlt[4] && abs(nhw_process[scan-1])<wvlt[4]) 
					{
						nhw_process[scan]=0;nhw_process[scan+1]=0;
					}
				}
				
				if (abs(nhw_process[scan])>=ratio && abs(nhw_process[scan])<wvlt[0]) 
				{	
					if (abs(nhw_process[scan-1])<(ratio) && abs(nhw_process[scan+1])<(ratio)) 
					{
						nhw_process[scan]=0;		
					}	
					else if (abs(nhw_process[scan])<(wvlt[0]-4)) 
					{
						nhw_process[scan]=0;
					}
				}
			}

			for (scan=i+(IM_DIM),j=(IM_DIM);j<((2*IM_DIM)-1);j++,scan++)
			{
				if (abs(nhw_process[scan])>=ratio &&  abs(nhw_process[scan])<(wvlt[1]+1)) 
				{	
					if (abs(resIII[((((i-(2*IM_SIZE))>>1)+(j-IM_DIM))>>1)+((IM_SIZE>>1)+(IM_DIM>>1))])<(wvlt[3]+1)) nhw_process[scan]=0;
					else if (abs(nhw_process[scan]+nhw_process[scan-1])<wvlt[4] && abs(nhw_process[scan+1])<wvlt[4]) 
					{
						nhw_process[scan]=0;nhw_process[scan-1]=0;
					}
					else if (abs(nhw_process[scan]+nhw_process[scan+1])<wvlt[4] && abs(nhw_process[scan-1])<wvlt[4]) 
					{
						nhw_process[scan]=0;nhw_process[scan+1]=0;
					}
				}
				
				if (abs(nhw_process[scan])>=ratio && abs(nhw_process[scan])<wvlt[1]) 
				{	
					if (abs(nhw_process[scan-1])<ratio && abs(nhw_process[scan+1])<ratio) 
					{
						if (quality>LOW10)
						{
							if (nhw_process[scan]>=16) nhw_process[scan]=7;
							else if (nhw_process[scan]<=-16) nhw_process[scan]=-7;	
							else nhw_process[scan]=0;
						}
						else nhw_process[scan]=0;
					}
					else if (abs(nhw_process[scan])<(wvlt[1]-5))
					{
						if (quality>LOW10)
						{
							if (nhw_process[scan]>=16) nhw_process[scan]=7;
							else if (nhw_process[scan]<=-16) nhw_process[scan]=-7;
							else nhw_process[scan]=0;							
						}
						else nhw_process[scan]=0;
					}
				}
			}
		}
	}

	if (quality>LOW4)
	{ 
		for (i=(2*IM_DIM),count=0,res=0;i<((2*IM_SIZE)-(2*IM_DIM));i+=(2*IM_DIM))
		{
			for (scan=i+(IM_DIM+1),j=(IM_DIM+1);j<((2*IM_DIM)-1);j++,scan++)
			{
				if (nhw_process[scan]>4 && nhw_process[scan]<8)
				{
					if (nhw_process[scan-1]>3 && nhw_process[scan-1]<=7)
					{
						if (nhw_process[scan+1]>3 && nhw_process[scan+1]<=7)
						{
							nhw_process[scan]=12700;nhw_process[scan-1]=10100;nhw_process[scan+1]=10100;
						}
					}
				}
				else if (nhw_process[scan]<-4 && nhw_process[scan]>-8)
				{
					if (nhw_process[scan-1]<-3 && nhw_process[scan-1]>=-7)
					{
						if (nhw_process[scan+1]<-3 && nhw_process[scan+1]>=-7)
						{
							nhw_process[scan]=12900;nhw_process[scan-1]=10100;nhw_process[scan+1]=10100;	 
						}
					}
				}
				else if ((nhw_process[scan]==-7) && (nhw_process[scan+1]==-6 || nhw_process[scan+1]==-7))
				{
					nhw_process[scan]=10204;nhw_process[scan+1]=10100;
				}
				else if (nhw_process[scan]==7 && nhw_process[scan+1]==7)
				{
					nhw_process[scan]=10300;nhw_process[scan+1]=10100;
				}
				else if (nhw_process[scan]==8)
				{
					if ((nhw_process[scan-1]&65534)==6 || (nhw_process[scan+1]&65534)==6) nhw_process[scan]=10;
					else if (nhw_process[scan+1]==8) {nhw_process[scan]=9;nhw_process[scan+1]=9;}
				}
				else if (nhw_process[scan]==-8)
				{
					if (((-nhw_process[scan-1])&65534)==6 || ((-nhw_process[scan+1])&65534)==6) nhw_process[scan]=-9;
					else if (nhw_process[scan+1]==-8) {nhw_process[scan]=-9;nhw_process[scan+1]=-9;}
				}
			}
		}

		for (i=((2*IM_SIZE)+(2*IM_DIM));i<((4*IM_SIZE)-(2*IM_DIM));i+=(2*IM_DIM))
		{
			for (scan=i+1,j=1;j<(IM_DIM-1);j++,scan++)
			{
				if (nhw_process[scan]>4 && nhw_process[scan]<8)
				{
					if (nhw_process[scan-1]>3 && nhw_process[scan-1]<=7)
					{
						if (nhw_process[scan+1]>3 && nhw_process[scan+1]<=7)
						{
							nhw_process[scan]=12700;nhw_process[scan-1]=10100;nhw_process[scan+1]=10100;
						}
					}
				}
				else if (nhw_process[scan]<-4 && nhw_process[scan]>-8)
				{
					if (nhw_process[scan-1]<-3 && nhw_process[scan-1]>=-7)
					{
						if (nhw_process[scan+1]<-3 && nhw_process[scan+1]>=-7)
						{
							nhw_process[scan]=12900;nhw_process[scan-1]=10100;nhw_process[scan+1]=10100;
						}
					}
				}
				else if (nhw_process[scan]==-6 || nhw_process[scan]==-7)
				{
					if (nhw_process[scan+1]==-7)
					{
						nhw_process[scan]=10204;nhw_process[scan+1]=10100;
					}
					else if (nhw_process[scan-(2*IM_DIM)]==-7)
					{
						if (abs(nhw_process[scan+IM_DIM])<8) nhw_process[scan+IM_DIM]=10204;nhw_process[scan]=10100;
					}
					
				}
				else if (nhw_process[scan]==7)
				{
					if (nhw_process[scan+1]==7)
					{
						nhw_process[scan]=10300;nhw_process[scan+1]=10100;
					}
					else if (nhw_process[scan-(2*IM_DIM)]==7)
					{
						if (abs(nhw_process[scan+IM_DIM])<8) nhw_process[scan+IM_DIM]=10300;nhw_process[scan]=10100;
					}
				}
				else if (nhw_process[scan]==8)
				{
					if ((nhw_process[scan-1]&65534)==6 || (nhw_process[scan+1]&65534)==6) nhw_process[scan]=10;
				}
				else if (nhw_process[scan]==-8)
				{
					if (((-nhw_process[scan-1])&65534)==6 || ((-nhw_process[scan+1])&65534)==6) nhw_process[scan]=-9;
				}
			}
		}
	}
}

void process_res_q8(int quality, short *nhw_process, short *res256, encode_state *enc)
{
	int i, j, count, res, stage, e, scan, Y, a;
	int res_setting;

	if (quality>=NORM) res_setting=3;
	else if (quality>=LOW2) res_setting=4;
	else if (quality>=LOW5) res_setting=6;
	else if (quality>=LOW7) res_setting=8;

	for (j=0,count=0,res=0,stage=0,e=0;j<IM_DIM;j++)
	{
		for (scan=j,count=j,i=0;i<((2*IM_SIZE)-(2*IM_DIM));i+=(2*IM_DIM),scan+=(2*IM_DIM),count+=IM_DIM)
		{
			res=nhw_process[scan]-res256[count];a=nhw_process[scan+(2*IM_DIM)]-res256[count+IM_DIM];

			if (res==2 && a==2 && (nhw_process[scan+(4*IM_DIM)]-res256[count+(2*IM_DIM)])>=2)
			{
				if ((nhw_process[scan+(4*IM_DIM)]-res256[count+(2*IM_DIM)])<5 || (nhw_process[scan+(4*IM_DIM)]-res256[count+(2*IM_DIM)])>6)
				{
					res256[count]=12400;nhw_process[scan+(2*IM_DIM)]-=2;nhw_process[scan+(4*IM_DIM)]-=2;
				}
			}
			else if (((res==2 && a==3) || (res==3 && a==2)) && (nhw_process[scan+(4*IM_DIM)]-res256[count+(2*IM_DIM)])>1 && (nhw_process[scan+(4*IM_DIM)]-res256[count+(2*IM_DIM)])<6)
			{
				res256[count]=12400;nhw_process[scan+(2*IM_DIM)]-=2;nhw_process[scan+(4*IM_DIM)]-=2;
			}
			else if ((res==3 && a==3))
			{
				if ((nhw_process[scan+(4*IM_DIM)]-res256[count+(2*IM_DIM)])>0 && (nhw_process[scan+(4*IM_DIM)]-res256[count+(2*IM_DIM)])<6)
				{
					res256[count]=12400;nhw_process[scan+(2*IM_DIM)]-=2;nhw_process[scan+(4*IM_DIM)]-=2;
				}
				else if (quality>=LOW1)
				{
					res256[count]=12100;nhw_process[scan+(2*IM_DIM)]=res256[count+IM_DIM];
				}
			}
			else if (a==-4 && (res==2 || res==3) && ((nhw_process[scan+(4*IM_DIM)]-res256[count+(2*IM_DIM)])==2 || (nhw_process[scan+(4*IM_DIM)]-res256[count+(2*IM_DIM)])==3))
			{
				if (res==2 && (nhw_process[scan+(4*IM_DIM)]-res256[count+(2*IM_DIM)])==2) nhw_process[scan+(2*IM_DIM)]++;
				else {res256[count]=12400;nhw_process[scan+(2*IM_DIM)]-=2;nhw_process[scan+(4*IM_DIM)]-=2;}
			}
			else if (res==1 && a==3 && (nhw_process[scan+(4*IM_DIM)]-res256[count+(2*IM_DIM)])==2)
			{
				if (i>0) 
				{
					if ((nhw_process[scan-(2*IM_DIM)]-res256[count-IM_DIM])>=0) 
					{
						res256[count]=12400;nhw_process[scan+(2*IM_DIM)]-=2;nhw_process[scan+(4*IM_DIM)]-=2;
					}
				}
			}
			else if ((res==3 || res==4 || res==5 || res>6) && ((nhw_process[scan+(2*IM_DIM)]-res256[count+IM_DIM])==3 || ((nhw_process[scan+(2*IM_DIM)]-res256[count+IM_DIM])&65534)==4))
			{
				if ((res)>6) {res256[count]=12500;nhw_process[scan+(2*IM_DIM)]=res256[count+IM_DIM];}
				else if (quality>=LOW1)
				{
					res256[count]=12100;nhw_process[scan+(2*IM_DIM)]=res256[count+IM_DIM];
				}
				else if (quality==LOW2)
				{
					if (res<5 && a==5) res256[count+IM_DIM]=14100;
					else if (res>=5) res256[count]=14100;
					else if (res==3 && a>=4) res256[count+IM_DIM]=14100;
					
					nhw_process[scan+(2*IM_DIM)]=res256[count+IM_DIM];
				}
			}
			else if ((res==2 || res==3) && (a==2 || a==3))
			{ 
				if (!(nhw_process[scan+(4*IM_DIM)]-res256[count+(2*IM_DIM)]) || (nhw_process[scan+(4*IM_DIM)]-res256[count+(2*IM_DIM)])==1)
				{
					if ((nhw_process[scan+1]-res256[count+1])==2 || (nhw_process[scan+1]-res256[count+1])==3)
					{
						if ((nhw_process[scan+(2*IM_DIM+1)]-res256[count+(IM_DIM+1)])==2 || (nhw_process[scan+(2*IM_DIM+1)]-res256[count+(IM_DIM+1)])==3)
						{
							if ((nhw_process[scan+(4*IM_DIM+1)]-res256[count+(2*IM_DIM+1)])>0)
							{
								res256[count]=12400;nhw_process[scan+(2*IM_DIM)]-=2;nhw_process[scan+(4*IM_DIM)]-=2;
							}
						}
					}
				}
			}
			else if (a==4 && (res==-2 || res==-3) && ((res256[count+(2*IM_DIM)]-nhw_process[scan+(4*IM_DIM)])==2 || (res256[count+(2*IM_DIM)]-nhw_process[scan+(4*IM_DIM)])==3))
			{
				if (res==-2 && (res256[count+(2*IM_DIM)]-nhw_process[scan+(4*IM_DIM)])==2) nhw_process[scan+(2*IM_DIM)]--;
				else {res256[count]=12300;nhw_process[scan+(2*IM_DIM)]+=2;nhw_process[scan+(4*IM_DIM)]+=2;}
			}
			else if ((res==-3 || res==-4 || res==-5 || res<-7) && ((nhw_process[scan+(2*IM_DIM)]-res256[count+IM_DIM])==-3 || (nhw_process[scan+(2*IM_DIM)]-res256[count+IM_DIM])==-4 || (nhw_process[scan+(2*IM_DIM)]-res256[count+IM_DIM])==-5))
			{
				if (res<-7) 
				{
					res256[count]=12600;nhw_process[scan+(2*IM_DIM)]=res256[count+IM_DIM];
				}
				else if (quality>=LOW1)
				{
					res256[count]=12200;nhw_process[scan+(2*IM_DIM)]=res256[count+IM_DIM];
				}
				else if (quality==LOW2)
				{
					if (res>-5 && a==-5) res256[count+IM_DIM]=14000;
					else if (res<=-5) res256[count]=14000;
					else if (res==-3 && a<=-4) res256[count+IM_DIM]=14000;
					
					nhw_process[scan+(2*IM_DIM)]=res256[count+IM_DIM];
				}
			}
			else if (a==-2 || a==-3)
			{
				if (res==-2 || res==-3)
				{
					if((res256[count+(2*IM_DIM)]-nhw_process[scan+(4*IM_DIM)])>0)
					{
						res256[count]=12300;
						nhw_process[scan+(2*IM_DIM)]+=2;nhw_process[scan+(4*IM_DIM)]+=2;
					}
					else if (res==-3 && quality>=HIGH1)
					{
						res256[count]=14500;
					}
					else if (!(res256[count+(2*IM_DIM)]-nhw_process[scan+(4*IM_DIM)]))
					{
						if ((nhw_process[scan+1]-res256[count+1])==-2 || (nhw_process[scan+1]-res256[count+1])==-3)
						{
							if ((nhw_process[scan+(2*IM_DIM+1)]-res256[count+(IM_DIM+1)])==-2 || (nhw_process[scan+(2*IM_DIM+1)]-res256[count+(IM_DIM+1)])==-3)
							{
								if ((nhw_process[scan+(4*IM_DIM+1)]-res256[count+(2*IM_DIM+1)])<0)
								{
									res256[count]=12300;
									nhw_process[scan+(2*IM_DIM)]+=2;nhw_process[scan+(4*IM_DIM)]+=2;
								}
							}
						}
					}
					else if (res==-2) goto L_W2;
					else goto L_W3;
				}
				else if (res==-1 && a==-3 && (nhw_process[scan+(4*IM_DIM)]-res256[count+(2*IM_DIM)])==-2)
				{
					if (i>0) 
					{
						if ((nhw_process[scan-(2*IM_DIM)]-res256[count-IM_DIM])<=0) 
						{
							res256[count]=12300;
							nhw_process[scan+(2*IM_DIM)]+=2;nhw_process[scan+(4*IM_DIM)]+=2;
						}
					}
				}
				else if (res==-1)
				{
					if ((res256[count+(2*IM_DIM)]-nhw_process[scan+(4*IM_DIM)])==3)
					{
							res256[count]=12300;nhw_process[scan+(2*IM_DIM)]+=2;nhw_process[scan+(4*IM_DIM)]+=2;
					}
					else goto L_W1;

				}
				else if (res==-4)
				{
					if ((res256[count+(2*IM_DIM)]-nhw_process[scan+(4*IM_DIM)])>1)
					{
						if((res256[count+(2*IM_DIM)]-nhw_process[scan+(4*IM_DIM)])<4)
						{
							res256[count]=12300;
							nhw_process[scan+(2*IM_DIM)]+=2;nhw_process[scan+(4*IM_DIM)]+=2;
						}
						else goto L_W5;
					}
					else goto L_W5;
				}
			}
			else if (!res || res==-1)
			{
L_W1:			stage = (j<<9)+(i>>9)+(IM_DIM);

				if (nhw_process[stage]==7)
				{
					if (nhw_process[stage-1]>=0 && nhw_process[stage-1]<8) nhw_process[stage]+=2;
				}
				else if (nhw_process[stage]==8)
				{
					if (nhw_process[stage-1]>=-2 && nhw_process[stage-1]<8) nhw_process[stage]+=2;
				}
			}
			else if (res==-2)
			{
L_W2:			stage = (j<<9)+(i>>9)+(IM_DIM);

				if (nhw_process[stage]<-14)
				{
					if (!((-nhw_process[stage])&7) || ((-nhw_process[stage])&7)==7) nhw_process[stage]++;
				}
				else if (nhw_process[stage]==7 || (nhw_process[stage]&65534)==8)
				{
					if (nhw_process[stage-1]>=-2) nhw_process[stage]+=3;
				}
			}
			else if (res==-3) 
			{
L_W3:			if (quality>=HIGH1) {res256[count]=14500;}
				else if (nhw_process[(j<<9)+(i>>9)+(IM_DIM)]<-14)
				{
					if (!((-nhw_process[(j<<9)+(i>>9)+(IM_DIM)])&7) || ((-nhw_process[(j<<9)+(i>>9)+(IM_DIM)])&7)==7)
					{
						nhw_process[(j<<9)+(i>>9)+(IM_DIM)]++;
					}
				}
				else if (nhw_process[(j<<9)+(i>>9)+(IM_DIM)]>=0 && ((nhw_process[(j<<9)+(i>>9)+(IM_DIM)]+2)&65532)==8)
				{
					if (nhw_process[(j<<9)+(i>>9)+(IM_DIM-1)]>=-2) nhw_process[(j<<9)+(i>>9)+(IM_DIM)]=10;
				}
				else if (nhw_process[(j<<9)+(i>>9)+(IM_DIM)]>14 && (nhw_process[(j<<9)+(i>>9)+(IM_DIM)]&7)==7)
				{
					nhw_process[(j<<9)+(i>>9)+(IM_DIM)]++;
				}
			}
			else if (res<(-res_setting))
			{
L_W5:			res256[count]=14000;

				if (res==-4)
				{
					stage = (j<<9)+(i>>9)+(IM_DIM);

					if (nhw_process[stage]==-7 || nhw_process[stage]==-8) 
					{
						if (nhw_process[stage-1]<2 && nhw_process[stage-1]>-8) nhw_process[stage]=-9;
					}
				}
				else if (res<-6)
				{
					if (res<-7 && quality>=HIGH1) {res256[count]=14900;}
					else
					{

						stage = (j<<9)+(i>>9)+(IM_DIM);

						if (nhw_process[stage]<-14)
						{
							if (!((-nhw_process[stage])&7) || ((-nhw_process[stage])&7)==7) nhw_process[stage]++;
						}
						else if (nhw_process[stage]==7 || nhw_process[stage]==8)
						{
							if (nhw_process[stage-1]>=-1 && nhw_process[stage-1]<8) nhw_process[stage]+=3;
						}
					}
				}
			}
		}	
	}

	enc->nhw_res1_word_len=0;enc->nhw_res3_word_len=0;enc->nhw_res5_word_len=0;Y=0;

	for (i=0,count=0,e=0,stage=0,res=0,a=0;i<((4*IM_SIZE)>>1);i+=(2*IM_DIM))
	{
		for (scan=i,j=0;j<IM_DIM;j++,scan++,count++)
		{
			if (res256[count]<12000)
			{
				res= nhw_process[scan]-res256[count];res256[count]=0;

				if (!res || res==1)
				{
					stage = (j<<9)+(i>>9)+(IM_DIM);

					if (nhw_process[stage]==-7 || nhw_process[stage]==-8) 
					{
						if (nhw_process[stage-1]<2 && nhw_process[stage-1]>-8) nhw_process[stage]=-9;
					}
				}
				else if (res==2)
				{
					stage = (j<<9)+(i>>9)+(IM_DIM);

					if (nhw_process[stage]>15 && !(nhw_process[stage]&7)) nhw_process[stage]--;
					else if (nhw_process[stage]==-7 || nhw_process[stage]==-8) 
					{
						if (nhw_process[stage-1]<=1) nhw_process[stage]=-9;
					}
					else if (nhw_process[stage]==-6) 
					{
						if (nhw_process[stage-1]<=-1 && nhw_process[stage-1]>-8) nhw_process[stage]=-9;
					}
				}
				else if (res==3)
				{
					if (quality>=HIGH1) {res256[count]=144;enc->nhw_res5_word_len++;}
					else
					{
						stage = (j<<9)+(i>>9)+(IM_DIM);

						if (nhw_process[stage]>15 && !(nhw_process[stage]&7)) nhw_process[stage]--;
						else if (nhw_process[stage]<=0 && ((((-nhw_process[stage])+2)&65532)==8)) 
						{
							if (nhw_process[stage-1]<=2) nhw_process[stage]=-10;
						}
					}
				}
				else if (res>res_setting) 
				{
					res256[count]=141;enc->nhw_res1_word_len++;

					if (res==4)
					{
						stage = (j<<9)+(i>>9)+(IM_DIM);

						if (nhw_process[stage]==7 || (nhw_process[stage]&65534)==8)
						{
							if (nhw_process[stage-1]>=0 && nhw_process[stage-1]<8) nhw_process[stage]+=2;
						}
					}
					else if (res>6) 
					{
						if (res>7 && quality>=HIGH1) 
						{
							res256[count]=148;enc->nhw_res5_word_len++;enc->nhw_res1_word_len++;
						}
						else
						{
							stage = (j<<9)+(i>>9)+(IM_DIM);

							if (nhw_process[stage]>15 && !(nhw_process[stage]&7)) nhw_process[stage]--;
							else if (nhw_process[stage]==-6 || nhw_process[stage]==-7 || nhw_process[stage]==-8) 
							{
								if (nhw_process[stage-1]<0 && nhw_process[stage-1]>-8) nhw_process[stage]=-9;
							}
						}
					}
				}
			}
			else 
			{
				if (res256[count]==14000) {res256[count]=140;enc->nhw_res1_word_len++;} 
				else if (res256[count]==14500) {res256[count]=145;enc->nhw_res5_word_len++;}
				else if (res256[count]==12200) {res256[count]=122;enc->nhw_res3_word_len++;} 
				else if (res256[count]==12100) {res256[count]=121;enc->nhw_res3_word_len++;}
				else if (res256[count]==12300) {res256[count]=123;enc->nhw_res3_word_len++;} 
				else if (res256[count]==12400) {res256[count]=124;enc->nhw_res3_word_len++;}
				else if (res256[count]==14100) {res256[count]=141;enc->nhw_res1_word_len++;} 
				else if (res256[count]==12500) {res256[count]=125;enc->nhw_res3_word_len++;enc->nhw_res1_word_len++;}
				else if (res256[count]==12600) {res256[count]=126;enc->nhw_res3_word_len++;enc->nhw_res1_word_len++;}
				else if (res256[count]==14900) {res256[count]=149;enc->nhw_res5_word_len++;enc->nhw_res1_word_len++;}
			}
		}	
	}
}

void process_res3_q1(unsigned char *highres, short *res256, encode_state *enc)
{
	int i, j, scan, e, Y, stage, count;
	int res;
	unsigned char *ch_comp;
	unsigned char *scan_run;
	unsigned char *nhw_res3I_word;
	nhw_res3I_word = (unsigned char*) malloc(enc->nhw_res3_word_len*sizeof(char));

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
	enc->nhw_res3=(unsigned char*)malloc((enc->nhw_res3_len)*sizeof(char));

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

	enc->nhw_res3_bit=(unsigned char*)malloc(enc->nhw_res3_bit_len*sizeof(char));

	Y=stage>>3;

	for (i=0,stage=0;i<((Y<<3)+8);i+=8)
	{
		enc->nhw_res3_bit[stage++]=((scan_run[i]&1)<<7)|((scan_run[i+1]&1)<<6)|
								   ((scan_run[i+2]&1)<<5)|((scan_run[i+3]&1)<<4)|
								   ((scan_run[i+4]&1)<<3)|((scan_run[i+5]&1)<<2)|
								   ((scan_run[i+6]&1)<<1)|((scan_run[i+7]&1));
	}

	enc->nhw_res3_len=count;

	Y=enc->nhw_res3_word_len>>3;
	free(scan_run);

	scan_run=(unsigned char*)nhw_res3I_word;
	enc->nhw_res3_word=(unsigned char*)malloc((enc->nhw_res3_bit_len<<1)*sizeof(char));

	for (i=0,stage=0;i<((Y<<3)+8);i+=8)
	{
		enc->nhw_res3_word[stage++]=((scan_run[i]&3)<<6)|((scan_run[i+1]&3)<<4)|
								   ((scan_run[i+2]&3)<<2)|(scan_run[i+3]&3);

		enc->nhw_res3_word[stage++]=((scan_run[i+4]&3)<<6)|((scan_run[i+5]&3)<<4)|
								   ((scan_run[i+6]&3)<<2)|(scan_run[i+7]&3);
	}

	enc->nhw_res3_word_len=stage;

	for (i=0;i<count;i++) enc->nhw_res3[i]=highres[i];

	
	free(nhw_res3I_word);
}

void process_res5_q1(unsigned char *highres, short *res256, encode_state *enc)
{
	int i, j, scan, e, Y, stage, count;
	int res;
	unsigned char *ch_comp;
	unsigned char *nhw_res5I_word;
	unsigned char *scan_run;

	nhw_res5I_word=(unsigned char*)malloc(enc->nhw_res5_word_len*sizeof(char));

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
	enc->nhw_res5=(unsigned char*)malloc((enc->nhw_res5_len)*sizeof(char));

	for (i=0;i<enc->nhw_res5_len;i++) enc->nhw_res5[i]=highres[i];

	scan_run=(unsigned char*)malloc((enc->nhw_res5_len+8)*sizeof(char));

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

	enc->nhw_res5_bit_len=((stage>>3)+1);

	enc->nhw_res5_bit=(unsigned char*)malloc(enc->nhw_res5_bit_len*sizeof(char));

	Y=stage>>3;

	for (i=0,stage=0;i<((Y<<3)+8);i+=8)
	{
		enc->nhw_res5_bit[stage++]=((scan_run[i]&1)<<7)|((scan_run[i+1]&1)<<6)|
								   ((scan_run[i+2]&1)<<5)|((scan_run[i+3]&1)<<4)|
								   ((scan_run[i+4]&1)<<3)|((scan_run[i+5]&1)<<2)|
								   ((scan_run[i+6]&1)<<1)|((scan_run[i+7]&1));
	}

	enc->nhw_res5_len=count;

	Y=enc->nhw_res5_word_len>>3;
	free(scan_run);
	scan_run=(unsigned char*)nhw_res5I_word;
	enc->nhw_res5_word=(unsigned char*)malloc((enc->nhw_res5_bit_len<<1)*sizeof(char));

	for (i=0,stage=0;i<((Y<<3)+8);i+=8)
	{
		enc->nhw_res5_word[stage++]=((scan_run[i]&1)<<7)|((scan_run[i+1]&1)<<6)|
								   ((scan_run[i+2]&1)<<5)|((scan_run[i+3]&1)<<4)|
								   ((scan_run[i+4]&1)<<3)|((scan_run[i+5]&1)<<2)|
								   ((scan_run[i+6]&1)<<1)|((scan_run[i+7]&1));
	}

	enc->nhw_res5_word_len=stage;

	for (i=0;i<count;i++) enc->nhw_res5[i]=highres[i];

	
	free(nhw_res5I_word);


}




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

const char quality_table[][7] = {
	{ 11, 15, 10, 15, 36, 20, 21  },  // LOW20
	{ 11, 15, 10, 15, 36, 20, 21  },
	{ 11, 15, 10, 15, 36, 19, 20  },
	{ 11, 15, 10, 15, 36, 18, 18  },
	{ 11, 15, 10, 15, 36, 17, 17 },
	{ 11, 15, 10, 15, 36, 17, 17 },
	{ 11, 15, 10, 15, 36, 17, 17 },
	{ 10, 15, 9, 14, 36, 17, 17   },
	{  8, 13, 6, 11, 34, 15, 15   },
	{  8, 13, 6, 11, 34, 15, 15   },
	{  8, 13, 6, 11, 34, 15, 15   },
	{  8, 13, 6, 11, 34, 15, 15   },
	{  8, 13, 6, 11, 34, 14, 0xff }, // LOW8
	{ 15, 27, 10, 6, 3, 0xff, 0xff }, // LOW7
	{ 16, 28, 11, 8, 5, 0xff, 0xff }, // LOW6
	
};



int configure_wvlt(int quality, char *wvlt)
{
	const char *w;

	if (quality < 0) quality = 0;
	else if (quality > LOW6) quality = LOW6;

	w = quality_table[quality];
	COPY_WVLT(wvlt, w)
	return 0;
}

void scan_run_code(unsigned char *s, const short *nhw_process, encode_state *enc)
{
	int i, j, e, count, stage;
	int res;

	for (j=0,count=0,stage=0,e=0;j<(IM_DIM<<1);)
	{
		for (i=0;i<IM_DIM;i++)
		{
			s[count]=nhw_process[j];
			s[count+1]=nhw_process[j+1];
			s[count+2]=nhw_process[j+2];
			s[count+3]=nhw_process[j+3];
	
			j+=(2*IM_DIM);

			s[count+4]=nhw_process[j+3];
			s[count+5]=nhw_process[j+2];
			s[count+6]=nhw_process[j+1];
			s[count+7]=nhw_process[j];

			j+=(2*IM_DIM);
			count+=8;
		}

		j-=((4*IM_SIZE)-4);
	}


	for (i=0;i<4*IM_SIZE-4;i++)
	{
		if (s[i]!=128 && s[i+1]==128)
		{
			if (s[i+2]==128)
			{
				if  (s[i+3]==128)
				{
					if (s[i]==136 && s[i+4]==136) {s[i]=132;s[i+4]=201;i+=4;}
					else if (s[i]==136 && s[i+4]==120) {s[i]=133;s[i+4]=201;i+=4;}
					else if (s[i]==120 && s[i+4]==136) {s[i]=134;s[i+4]=201;i+=4;}
					else if (s[i]==120 && s[i+4]==120) {s[i]=135;s[i+4]=201;i+=4;}
					//else if (s[i]==136 && s[i+4]==112) {s[i]=127;i+=4;}
					//else if (s[i]==112 && s[i+4]==136) {s[i]=126;i+=4;}
					//else if (s[i]==136 && s[i+4]==144) {s[i]=125;i+=4;}
					//else if (s[i]==144 && s[i+4]==136) {s[i]=123;i+=4;}
					//else if (s[i]==120 && s[i+4]==112) {s[i]=121;i+=4;}
					//else if (s[i]==112 && s[i+4]==120) {s[i]=122;i+=4;}
					else i+=3;
				}
				else i+=2;
			}
			else i++;
		}
	}

	s[0]=128;s[1]=128;s[2]=128;s[3]=128;
	s[(4*IM_SIZE)-4]=128;s[(4*IM_SIZE)-3]=128;s[(4*IM_SIZE)-2]=128;s[(4*IM_SIZE)-1]=128;

	for (i=4,enc->nhw_select1=0,enc->nhw_select2=0,count=0,res=0;i<((4*IM_SIZE)-4);i++)
	{
		if (s[i]==136)
		{
			if (s[i+2]==128 && (s[i+1]==120 || s[i+1]==136)  && s[i-1]==128 && s[i-2]==128 && s[i-3]==128 && s[i-4]==128)
			{
				if (s[i+1]==120) s[i+1]=157;
				else s[i+1]=159;

				enc->nhw_select2++;
			}
			else if (s[i-1]==128 && (s[i+1]==120 || s[i+1]==136) && s[i+2]==128 && s[i+3]==128 && s[i+4]==128 && s[i+5]==128)
			{
				if (s[i+1]==120) s[i+1]=157;
				else s[i+1]=159;

				enc->nhw_select2++;
			}
			else if (s[i-1]==128 && s[i-2]==128 && s[i-3]==128 && s[i-4]==128 && s[i+1]==128)
			{
				s[i]=153;enc->nhw_select1++;
			}
			else if (s[i-1]==128 && s[i+1]==128 && s[i+2]==128 && s[i+3]==128 && s[i+4]==128)
			{
				s[i]=153;enc->nhw_select1++;
			}
		}
		else if (s[i]==120)
		{
			if (s[i+2]==128 && (s[i+1]==120 || s[i+1]==136)  && s[i-1]==128 && s[i-2]==128 && s[i-3]==128 && s[i-4]==128)
			{
				if (s[i+1]==120) s[i+1]=157;
				else s[i+1]=159;

				enc->nhw_select2++;
			}
			else if (s[i-1]==128 && (s[i+1]==120 || s[i+1]==136) && s[i+2]==128 && s[i+3]==128 && s[i+4]==128 && s[i+5]==128)
			{
				if (s[i+1]==120) s[i+1]=157;
				else s[i+1]=159;

				enc->nhw_select2++;
			}
			else if (s[i-1]==128 && s[i-2]==128 && s[i-3]==128 && s[i-4]==128 && s[i+1]==128)
			{
				s[i]=155;enc->nhw_select1++;
			}
			else if (s[i-1]==128 && s[i+1]==128 && s[i+2]==128 && s[i+3]==128 && s[i+4]==128)
			{
				s[i]=155;enc->nhw_select1++;
			}
		}
	}


	for (i=0,count=0;i<(4*IM_SIZE);i++)
	{
		while (s[i]==128 && s[i+1]==128)   
		{
			count++;

			if (count>255)
			{
				if (s[i]==153) s[i]=124;
				else if (s[i]==155) s[i]=123;

				if (s[i+1]==153) s[i+1]=124;
				else if (s[i+1]==155) s[i+1]=123;

				if (s[i+2]==153) s[i+2]=124;
				else if (s[i+2]==155) s[i+2]=123;

				if (s[i+3]==153) s[i+3]=124;
				else if (s[i+3]==155) s[i+3]=123;

				i--;count=0;
			}
			else i++;
		}
		 
		if (count>=252)
		{
			if (s[i+1]==153) s[i+1]=124;
			else if (s[i+1]==155) s[i+1]=123;
		}

		count=0;
	}
}

void do_y_wavelet(int quality, short *nhw_process, int ratio)
{
	int y_wavelet, y_wavelet2;
	int i, j, count, scan;
	int a, e;

	if (quality>HIGH2) {
		y_wavelet=8;y_wavelet2=4;
	} else {
		y_wavelet=9;y_wavelet2=9;
	}

	for (i=(2*IM_DIM),count=0,scan=0;i<((4*IM_SIZE>>1)-(2*IM_DIM));i+=(2*IM_DIM))
	{
		for (j=(IM_DIM+1);j<(2*IM_DIM-1);j++)
		{
			if (abs(nhw_process[i+j])>=(ratio-2)) 
			{
				a=i+j;

				if (abs(nhw_process[a])<y_wavelet2)
				{
					if (((abs(nhw_process[a-1]))+2)>=8) scan++;
					if (((abs(nhw_process[a+1]))+2)>=8) scan++;
					if (((abs(nhw_process[a-(2*IM_DIM)]))+2)>=8) scan++;
					if (((abs(nhw_process[a+(2*IM_DIM)]))+2)>=8) scan++;

					if (scan<3 && nhw_process[a]<y_wavelet && nhw_process[a]>-y_wavelet) 
					{
						//printf("%d %d %d\n",nhw_process[a-1],nhw_process[a],nhw_process[a+1]);
						if (nhw_process[a]<-6) nhw_process[a]=-7;
						else if (nhw_process[a]>6) nhw_process[a]=7;
					}
					/*else if (!scan && abs(nhw_process[a])<9) 
					{
						if (nhw_process[a]<-6) nhw_process[a]=-7;
						else if (nhw_process[a]>6) nhw_process[a]=7;
					}*/

					scan=0;
				}
			}
			else nhw_process[i+j]=0;

			//if (abs(nhw_process[i+j])<9) nhw_process[i+j]=0;
			if (abs(nhw_process[i+j])>6)
			{
			e=nhw_process[i+j];

			if (e>=8 && (e&7)<2) 
			{
				if (nhw_process[i+j+1]>7 && nhw_process[i+j+1]<10000) nhw_process[i+j+1]--;
				//if (nhw_process[i+j+(2*IM_DIM)]>8) nhw_process[i+j+(2*IM_DIM)]--;
			}
			else if (e==-7 && nhw_process[i+j+1]==8) nhw_process[i+j]=-8;
			else if (e==8 && nhw_process[i+j+1]==-7) nhw_process[i+j+1]=-8;
			else if (e<-7 && ((-e)&7)<2)
			{
				if (nhw_process[i+j+1]<-14 && nhw_process[i+j+1]<10000)
				{
					if (((-nhw_process[i+j+1])&7)==7) nhw_process[i+j+1]++;
					else if (((-nhw_process[i+j+1])&7)<2 && j<((2*IM_DIM)-2) && nhw_process[i+j+2]<=0) nhw_process[i+j+1]++;
				}
			}
			}
		}
	}

	

	if (quality>HIGH2) 
	{
		y_wavelet=8;y_wavelet2=4;
	}
	else if (quality>LOW3)
	{
		y_wavelet=8;y_wavelet2=9;
	}
	else 
	{
		y_wavelet=9;y_wavelet2=9;
	}

	for (i=((4*IM_SIZE)>>1),scan=0;i<(4*IM_SIZE-(2*IM_DIM));i+=(2*IM_DIM))
	{
		for (j=1;j<(IM_DIM);j++)
		{
			if (abs(nhw_process[i+j])>=(ratio-2)) 
			{	
				a=i+j;

				if (abs(nhw_process[a])<y_wavelet2)
				{
					if (((abs(nhw_process[a-1]))+2)>=8) scan++;
					if (((abs(nhw_process[a+1]))+2)>=8) scan++;
					if (((abs(nhw_process[a-(2*IM_DIM)]))+2)>=8) scan++;
					if (((abs(nhw_process[a+(2*IM_DIM)]))+2)>=8) scan++;

					if (scan<3 && nhw_process[a]<y_wavelet && nhw_process[a]>(-y_wavelet))
					{
						if (nhw_process[a]<0) nhw_process[a]=-7;else nhw_process[a]=7;
					}
					else if (!scan && abs(nhw_process[a])<y_wavelet2)
					{
						if (nhw_process[a]<0) nhw_process[a]=-7;else nhw_process[a]=7;
					}

					scan=0;
				}
			}
			else nhw_process[i+j]=0;

			if (abs(nhw_process[i+j])>6)
			{
				e=nhw_process[i+j];

				if (e>=8 && (e&7)<2) 
				{
					if (nhw_process[i+j+1]>7 && nhw_process[i+j+1]<10000) nhw_process[i+j+1]--;
					//else if (nhw_process[i+j+(2*IM_DIM)]>8 && nhw_process[i+j+(2*IM_DIM)]<10000) nhw_process[i+j+(2*IM_DIM)]--;
				}
				else if (e==-7 && nhw_process[i+j+1]==8) nhw_process[i+j]=-8;
				else if (e==8 && nhw_process[i+j+1]==-7) nhw_process[i+j+1]=-8;
				else if (e<-7 && ((-e)&7)<2)
				{
					if (nhw_process[i+j+1]<-14 && nhw_process[i+j+1]<10000)
					{
						if (((-nhw_process[i+j+1])&7)==7) nhw_process[i+j+1]++;
						else if (((-nhw_process[i+j+1])&7)<2 && j<(IM_DIM-2) && nhw_process[i+j+2]<=0) nhw_process[i+j+1]++;
					}
				}
			}
		}
	}

	if (quality>HIGH2) y_wavelet=8;
	else y_wavelet=11;

	for (i=((4*IM_SIZE)>>1),scan=0,count=0;i<(4*IM_SIZE-(2*IM_DIM));i+=(2*IM_DIM))
	{
		for (j=(IM_DIM+1);j<(2*IM_DIM-1);j++)
		{
			if (abs(nhw_process[i+j])>=(ratio-1)) 
			{	
				a=i+j;

				if (abs(nhw_process[a])<y_wavelet)
				{
					if (((abs(nhw_process[a-1]))+2)>=8) scan++;
					if (((abs(nhw_process[a+1]))+2)>=8) scan++;
					if (((abs(nhw_process[a-(2*IM_DIM)]))+2)>=8) scan++;
					if (((abs(nhw_process[a+(2*IM_DIM)]))+2)>=8) scan++;

					if (scan<3 && nhw_process[a]<y_wavelet && nhw_process[a]>-y_wavelet)
					{
						if (nhw_process[a]<0) nhw_process[a]=-7;else nhw_process[a]=7;
					}
					/*else if (!scan && abs(nhw_process[a])<11) 
					{
						if (nhw_process[a]<0) nhw_process[a]=-7;else nhw_process[a]=7;
					}*/

					scan=0;
				}
			}
			else nhw_process[i+j]=0;

			if (abs(nhw_process[i+j])>6)
			{
				e=nhw_process[i+j];

				if (e>=8 && (e&7)<2) 
				{
					if (nhw_process[i+j+1]>7 && nhw_process[i+j+1]<10000) nhw_process[i+j+1]--;
					//else if (nhw_process[i+j+(2*IM_DIM)]>7 && nhw_process[i+j+(2*IM_DIM)]<10000) nhw_process[i+j+(2*IM_DIM)]--;
				}
				else if (e==-7 && nhw_process[i+j+1]==8) nhw_process[i+j]=-8;
				else if (e==8 && nhw_process[i+j+1]==-7) nhw_process[i+j+1]=-8;
				else if (e<-7 && ((-e)&7)<2)
				{
					if (nhw_process[i+j+1]<-14 && nhw_process[i+j+1]<10000)
					{
						if (((-nhw_process[i+j+1])&7)==7) nhw_process[i+j+1]++;
						else if (((-nhw_process[i+j+1])&7)<2 && j<((2*IM_DIM)-2) && nhw_process[i+j+2]<=0) nhw_process[i+j+1]++;
					}
				}
			}
		}
	}
}

void SWAPOUT_FUNCTION(encode_y)(image_buffer *im, encode_state *enc, int ratio)
{
	int quality = im->setup->quality_setting;
	int res;
	int i, j, scan, count, stage, Y, a, e;
	int wavelet_order, end_transform;
	short *res256, *resIII;
	unsigned char *nhw_res1I_word, *highres, *ch_comp, *scan_run;
	short *nhw_process;
	unsigned char wvlt[7];
	nhw_process=(short*)im->im_process;

	end_transform=0;
	wavelet_order=im->setup->wvlts_order;
	//for (stage=0;stage<wavelet_order;stage++) wavelet_analysis(im,(2*IM_DIM)>>stage,end_transform++,1); 

	wavelet_analysis(im,(2*IM_DIM),end_transform++,1);

	res256=(short*)malloc(IM_SIZE*sizeof(short));
	resIII=(short*)malloc(IM_SIZE*sizeof(short));

	for (i=0,count=0;i<(4*IM_SIZE>>1);i+=(2*IM_DIM))
	{
		for (scan=i,j=0;j<IM_DIM;j++) 
		{
			res256[count++]=im->im_jpeg[scan++];
		}
	}

	im->setup->RES_HIGH=0;

	wavelet_analysis(im,(2*IM_DIM)>>1,end_transform,1);

	if (quality>LOW14) // Better quality than LOW14?
	{
		preprocess14(nhw_process, res256);

		offsetY_recons256(im,enc,ratio,1);

		wavelet_synthesis(im,(2*IM_DIM)>>1,end_transform-1,1);

		postprocess14(im, nhw_process, res256);

		wavelet_analysis(im,(2*IM_DIM)>>1,end_transform,1);
		
	}
	
	if (quality<=LOW9) // Worse than LOW9?
	{
		if (quality>LOW14) wvlt[0]=10;else wvlt[0]=11;
			
		for (i=IM_SIZE;i<(2*IM_SIZE);i+=(2*IM_DIM))
		{
			for (scan=i,j=0;j<(IM_DIM);j++,scan++)
			{
				if (abs(nhw_process[scan])>=ratio &&  abs(nhw_process[scan])<wvlt[0]) 
				{	
					if (abs(nhw_process[scan-1])<ratio && abs(nhw_process[scan+1])<ratio) 
					{
						nhw_process[scan]=0;
					}
					else if (abs(nhw_process[scan])==ratio)
					{
						if (abs(nhw_process[scan-1])<ratio || abs(nhw_process[scan+1])<ratio) 
						{
							nhw_process[scan]=0;
						}
					}
				}
			}
		}
	}

	configure_wvlt(quality, wvlt);

	if (quality<LOW7) {
		preprocess_q7(quality, nhw_process, wvlt);
		if (quality<=LOW9)
		{
			preprocess_q9(nhw_process, wvlt);
		}
	}	
	
	
	for (i=0,count=0;i<(2*IM_SIZE);i+=(2*IM_DIM))
	{
		for (scan=i,j=0;j<IM_DIM;j++)
		{
			resIII[count++]=nhw_process[scan++];
		}
	}

	enc->tree1=(unsigned char*)malloc(((96*IM_DIM)+1)*sizeof(char));
	enc->exw_Y=(unsigned char*)malloc(32*IM_DIM*sizeof(short));
	
	if (quality>LOW3)
	{
		for (i=0,count=0,res=0,e=0,stage=0;i<((4*IM_SIZE)>>2);i+=(2*IM_DIM))
		{
			for (count=i,j=0;j<(((2*IM_DIM)>>2)-3);j++,count++)
			{
				if ((nhw_process[count]&1)==1 && (nhw_process[count+1]&1)==1 && (nhw_process[count+2]&1)==1 && (nhw_process[count+3]&1)==1 && abs(nhw_process[count]-nhw_process[count+3])>1)
				{
					nhw_process[count]+=24000;nhw_process[count+1]+=16000;//printf("\n %d ",count);
					nhw_process[count+2]+=16000;nhw_process[count+3]+=16000;

					res++;stage++;j+=3;count+=3;
				}
			}

			if (!stage) res++;
			stage=0;
		}

		enc->nhw_res4_len=res;
		enc->nhw_res4=(unsigned char*)malloc(enc->nhw_res4_len*sizeof(char));
	}

	enc->ch_res=(unsigned char*)malloc((IM_SIZE>>2)*sizeof(char));

	compress1(quality, nhw_process, wvlt, enc);

	Y_highres_compression(im,enc);

	free(enc->ch_res);

	for (i=0,count=0;i<(2*IM_SIZE);i+=(2*IM_DIM))
	{
		for (scan=i,j=0;j<IM_DIM;j++)
		{ 
			im->im_process[scan++]=resIII[count++];
		}
	}

	
	if (quality>LOW8) { // Better than LOW8?

		offsetY_recons256(im,enc,ratio,0);

		wavelet_synthesis(im,(2*IM_DIM)>>1,end_transform-1,1);

		if (quality>HIGH1) {
			im->im_wavelet_first_order=(short*)malloc(IM_SIZE*sizeof(short));

			for (i=0,count=0;i<(2*IM_SIZE);i+=(2*IM_DIM))
			{
				for (scan=i,j=0;j<IM_DIM;j++) 
				{
					im->im_wavelet_first_order[count++]=im->im_jpeg[scan++];
				}
			}
		}
	
	}

	// After last call of offsetY_recons256() we can free:
	free(enc->highres_mem);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	compress_s2(quality, resIII, nhw_process, wvlt, enc, ratio);

	
	if (quality > LOW8) {
		process_res_q8(quality, nhw_process, res256, enc);
	}

	highres=(unsigned char*)malloc(((96*IM_DIM)+1)*sizeof(char));

	if (quality > HIGH1) {
		process_res_hq(quality, im->im_wavelet_first_order, res256);
	}
	
	if (quality > LOW8)
	{
		
		nhw_res1I_word=(unsigned char*)malloc(enc->nhw_res1_word_len*sizeof(char));
		
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
				else if (res256[scan]!=0)
				{
					if (res256[scan]==141) 
					{
						highres[count++]=j;
						res256[scan]=0;nhw_res1I_word[e++]=1;
					}
					else if (res256[scan]==140) 
					{
						highres[count++]=j;
						res256[scan]=0;nhw_res1I_word[e++]=0;
					}
					else if (res256[scan]==126) 
					{
						highres[count++]=j;
						res256[scan]=122;nhw_res1I_word[e++]=0;
					}
					else if (res256[scan]==125) 
					{
						highres[count++]=j;
						res256[scan]=121;nhw_res1I_word[e++]=1;
					}
					else if (res256[scan]==148) 
					{
						highres[count++]=j;
						res256[scan]=144;nhw_res1I_word[e++]=1;
					}
					else if (res256[scan]==149) 
					{
						highres[count++]=j;
						res256[scan]=145;nhw_res1I_word[e++]=0;
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
		enc->nhw_res1=(unsigned char*)malloc((enc->nhw_res1_len)*sizeof(char));

		for (i=0;i<enc->nhw_res1_len;i++) enc->nhw_res1[i]=highres[i];

		scan_run=(unsigned char*)malloc((enc->nhw_res1_len+8)*sizeof(char));

		for (i=0;i<enc->nhw_res1_len;i++) scan_run[i]=enc->nhw_res1[i]>>1;

		highres[0]=scan_run[0];

		for (i=1,count=1;i<enc->nhw_res1_len-1;i++)
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

		for (i=0,stage=0;i<enc->nhw_res1_len;i++) 
		{
			if (enc->nhw_res1[i]!=254) scan_run[stage++]=enc->nhw_res1[i];
		}

		for (i=stage;i<stage+8;i++) scan_run[i]=0;

		enc->nhw_res1_bit_len=((stage>>3)+1);

		enc->nhw_res1_bit=(unsigned char*)malloc(enc->nhw_res1_bit_len*sizeof(char));

		Y=stage>>3;

		for (i=0,stage=0;i<((Y<<3)+8);i+=8)
		{
			enc->nhw_res1_bit[stage++]=((scan_run[i]&1)<<7)|((scan_run[i+1]&1)<<6)|
									   ((scan_run[i+2]&1)<<5)|((scan_run[i+3]&1)<<4)|
									   ((scan_run[i+4]&1)<<3)|((scan_run[i+5]&1)<<2)|
									   ((scan_run[i+6]&1)<<1)|((scan_run[i+7]&1));
		}


		enc->nhw_res1_len=count;

		Y=enc->nhw_res1_word_len>>3;
		free(scan_run);
		scan_run=(unsigned char*)nhw_res1I_word;
		enc->nhw_res1_word=(unsigned char*)malloc((Y+1)*sizeof(char));

		for (i=0,stage=0;i<((Y<<3)+8);i+=8)
		{
			enc->nhw_res1_word[stage++]=((scan_run[i]&1)<<7)|((scan_run[i+1]&1)<<6)|
									   ((scan_run[i+2]&1)<<5)|((scan_run[i+3]&1)<<4)|
									   ((scan_run[i+4]&1)<<3)|((scan_run[i+5]&1)<<2)|
									   ((scan_run[i+6]&1)<<1)|((scan_run[i+7]&1));
		}

		enc->nhw_res1_word_len=stage;

		for (i=0;i<count;i++) enc->nhw_res1[i]=highres[i];

		free(nhw_res1I_word);
		
	}

	// Further residual processing:
	if (quality>=LOW1) {
		process_res3_q1(highres, res256, enc);
	}
	
	if (quality>=HIGH1)
	{
		process_res5_q1(highres, res256, enc);
	}

	free(highres);
	free(res256);

	for (i=0,count=0,stage=0;i<(2*IM_SIZE);i+=(2*IM_DIM))
	{
		for (scan=i,j=0;j<IM_DIM;j++)
		{ 
			if (j<(IM_DIM>>1) && i<IM_SIZE) 
			{
				if (resIII[count]>8000) 
				{
					im->im_process[scan++]=resIII[count++];
				}
				else 
				{
					im->im_process[scan++]=0;count++;
				}
			}
			else im->im_process[scan++]=resIII[count++];
		}
	}

	free(resIII);

////////////////////////////////////////////////////////////////////////////	

	do_y_wavelet(quality, nhw_process, ratio);

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


void encode_uv(image_buffer *im, encode_state *enc, int ratio, int res_uv, int uv)
{
	int i, j, a, e, Y, scan, count;
	int res;
	int end_transform;
	unsigned char wvlt[7];
	unsigned char *ch_comp;
	unsigned char *scan_run;
	short *res256;
	short *resIII;
	short *nhw_process;
	const unsigned char *buf;

	if (uv) {
		buf = im->im_bufferV;
	} else {
		buf = im->im_bufferU;
	}
	for (i=0;i<IM_SIZE;i++) im->im_jpeg[i]=buf[i];

	nhw_process=(short*)im->im_process;
	
	if (im->setup->quality_setting<=LOW6) pre_processing_UV(im);

	end_transform=0;im->setup->RES_HIGH=0;

	wavelet_analysis(im,IM_DIM,end_transform++,0);

	res256=(short*)malloc((IM_SIZE>>2)*sizeof(short));
	resIII=(short*)malloc((IM_SIZE>>2)*sizeof(short));

	for (i=0,count=0;i<(IM_SIZE>>1);i+=(IM_DIM))
	{
		for (scan=i,j=0;j<(IM_DIM>>1);j++) res256[count++]=im->im_jpeg[scan++];
	}
	
	if (im->setup->quality_setting<=LOW4)
	{
		for (i=0;i<(IM_SIZE>>1);i+=(IM_DIM))
		{
			for (scan=i+(IM_DIM>>1),j=(IM_DIM>>1);j<(IM_DIM);j++,scan++)
			{
				if (abs(nhw_process[scan])>=ratio && abs(nhw_process[scan])<24) 
				{	
					 nhw_process[scan]=0;	
				}
			}
		}	
			
		for (i=(IM_SIZE>>1);i<(IM_SIZE);i+=(IM_DIM))
		{
			for (scan=i,j=0;j<(IM_DIM>>1);j++,scan++)
			{
				if (abs(nhw_process[scan])>=ratio && abs(nhw_process[scan])<32) 
				{	
					 nhw_process[scan]=0;	
				}
			}

			for (scan=i+(IM_DIM>>1),j=(IM_DIM>>1);j<(IM_DIM);j++,scan++)
			{
				if (abs(nhw_process[scan])>=ratio && abs(nhw_process[scan])<48) 
				{	
					 nhw_process[scan]=0;		
				}
			}
		}
	}

	wavelet_analysis(im,IM_DIM>>1,end_transform,0); 

	offsetUV_recons256(im,ratio,1);

	wavelet_synthesis(im,IM_DIM>>1,end_transform-1,0); 

	int t0, t1;

	if (uv) {
		t0 = 1; t1 = -1;
	} else {
		t0 = 0; t1 = 0;
	}

	for (i=0,count=0;i<(IM_SIZE>>1);i+=IM_DIM)
	{
		for (e=i,j=0;j<(IM_DIM>>1);j++,count++,e++)
		{
			scan=nhw_process[e]-res256[count];
	
			if(scan>10) {im->im_jpeg[e]=res256[count]-6;}
			else if(scan>7) {im->im_jpeg[e]=res256[count]-3;}
			else if(scan>4) {im->im_jpeg[e]=res256[count]-2;}
			else if(scan>3) im->im_jpeg[e]=res256[count]-1;
			else if(scan>2 && (nhw_process[e+1]-res256[count+1])>=t0) im->im_jpeg[e]=res256[count]-1;
			else if (scan<-10) {im->im_jpeg[e]=res256[count]+6;}
			else if (scan<-7) {im->im_jpeg[e]=res256[count]+3;}
			else if (scan<-4) {im->im_jpeg[e]=res256[count]+2;}
			else if (scan<-3) im->im_jpeg[e]=res256[count]+1;
			else if(scan<-2 && (nhw_process[e+1]-res256[count+1])<=t1) im->im_jpeg[e]=res256[count]+1;
			else im->im_jpeg[e]=res256[count];
		}
	}

	wavelet_analysis(im,IM_DIM>>1,end_transform,0);

	for (i=0,count=0;i<(IM_SIZE>>1);i+=IM_DIM)
	{
		for (scan=i,j=0;j<(IM_DIM>>1);j++)
		{
			resIII[count++]=nhw_process[scan++];
		}
	}

	offsetUV_recons256(im,ratio,0);

	wavelet_synthesis(im,IM_DIM>>1,end_transform-1,0);

	if (im->setup->quality_setting>=LOW2)
	{ 
		residual_coding_q2(nhw_process, res256, res_uv);
	}

	for (i=0,count=0;i<(IM_SIZE>>1);i+=IM_DIM)
	{
		for (scan=i,j=0;j<(IM_DIM>>1);j++)
		{
			nhw_process[scan++]=resIII[count++];
		}
	}

	if (im->setup->quality_setting<=LOW9)
	{
		//if (im->setup->quality_setting==LOW9 || im->setup->quality_setting==LOW10)
		//{
			wvlt[0]=2;
			wvlt[1]=3;
			wvlt[2]=5;
			wvlt[3]=8;
		//}
		
		for (i=0,scan=0;i<(IM_SIZE>>2)-(2*IM_DIM);i+=(IM_DIM))
		{
			for (scan=i,j=0;j<(IM_DIM>>2)-2;j++,scan++)
			{
				if (abs(nhw_process[scan+1]-nhw_process[scan+(2*IM_DIM)+1])<wvlt[2] && abs(nhw_process[scan+(IM_DIM)]-nhw_process[scan+(IM_DIM)+2])<wvlt[2])
				{
					if (abs(nhw_process[scan+(IM_DIM)+1]-nhw_process[scan+(IM_DIM)])<(wvlt[3]-1) && abs(nhw_process[scan+1]-nhw_process[scan+(IM_DIM)+1])<wvlt[3])
					{
						nhw_process[scan+(IM_DIM)+1]=(nhw_process[scan+1]+nhw_process[scan+(2*IM_DIM)+1]+nhw_process[scan+(IM_DIM)]+nhw_process[scan+(IM_DIM)+2]+2)>>2;
					}
				}
			}
		}
		
		for (i=0,scan=0;i<(IM_SIZE>>2)-(2*IM_DIM);i+=(IM_DIM))
		{
			for (scan=i,j=0;j<(IM_DIM>>2)-2;j++,scan++)
			{
				if (abs(nhw_process[scan+2]-nhw_process[scan+1])<wvlt[2] && abs(nhw_process[scan+1]-nhw_process[scan])<wvlt[2])
				{
					if (abs(nhw_process[scan]-nhw_process[scan+(IM_DIM)])<wvlt[2] && abs(nhw_process[scan+2]-nhw_process[scan+(IM_DIM)+2])<wvlt[2])
					{
						if (abs(nhw_process[scan+(2*IM_DIM)+1]-nhw_process[scan+(IM_DIM)])<wvlt[2] && abs(nhw_process[scan+(IM_DIM)]-nhw_process[scan+(IM_DIM)+1])<wvlt[3]) 
						{
							nhw_process[scan+(IM_DIM)+1]=(nhw_process[scan+1]+nhw_process[scan+(2*IM_DIM)+1]+nhw_process[scan+(IM_DIM)]+nhw_process[scan+(IM_DIM)+2]+1)>>2;
						}
					}
				}
			}
		}
	}

	enc->exw_Y[enc->exw_Y_end++]=0;enc->exw_Y[enc->exw_Y_end++]=0;



	if (uv) {
		a = (IM_SIZE>>2)+(IM_SIZE>>4);
	} else {
		a = IM_SIZE >> 2;
	}

	for (i=0; i<((IM_SIZE)>>2);i+=(IM_DIM))
	{
		for (j=0;j<((IM_DIM)>>2);j++)
		{
			scan=nhw_process[j+i];

			if (scan>255 && (j>0 || i>0)) 
			{
				enc->exw_Y[enc->exw_Y_end++]=(i>>8);enc->exw_Y[enc->exw_Y_end++]=j+128;
				Y=scan-255;if (Y>255) Y=255;enc->exw_Y[enc->exw_Y_end++]=Y;
				enc->tree1[a]=enc->tree1[a-1];a++;nhw_process[j+i]=0;
			}
			else if (scan<0 && (j>0 || i>0)) 
			{
				enc->exw_Y[enc->exw_Y_end++]=(i>>8);enc->exw_Y[enc->exw_Y_end++]=j;
				if (scan<-255) scan=-255;
				enc->exw_Y[enc->exw_Y_end++]=-scan;enc->tree1[a]=enc->tree1[a-1];a++;nhw_process[j+i]=0;
			}
			else 
			{
				if (scan>255) { scan=255; }
				else if (scan<0) { scan=0; }
				enc->tree1[a++]=scan&254;
				nhw_process[j+i]=0;
			}
		}
	}

	if (im->setup->quality_setting>LOW5) 
	{
		int code;
		if (uv) {
			enc->res_V_64=(unsigned char*)malloc((IM_DIM<<1)*sizeof(char));
			scan_run=(unsigned char*)enc->res_V_64;
			code = 20480;
		} else {		
			enc->res_U_64=(unsigned char*)malloc((IM_DIM<<1)*sizeof(char));
			scan_run=(unsigned char*)enc->res_U_64;
			code = 16384;
		}

		ch_comp=(unsigned char*)malloc((16*IM_DIM)*sizeof(char));

		int c;

		for (i=0,e=0;i<(16*IM_DIM);i+=8)
		{
			c = code + i;
			ch_comp[i]=((enc->tree1[c++])>>1)&1;
			ch_comp[i+1]=((enc->tree1[c++])>>1)&1;
			ch_comp[i+2]=((enc->tree1[c++])>>1)&1;
			ch_comp[i+3]=((enc->tree1[c++])>>1)&1;
			ch_comp[i+4]=((enc->tree1[c++])>>1)&1;
			ch_comp[i+5]=((enc->tree1[c++])>>1)&1;
			ch_comp[i+6]=((enc->tree1[c++])>>1)&1;
			ch_comp[i+7]=((enc->tree1[c])>>1)&1;

			scan_run[e++]=(ch_comp[i]<<7)|(ch_comp[i+1]<<6)|(ch_comp[i+2]<<5)|(ch_comp[i+3]<<4)|(ch_comp[i+4]<<3)|
					  (ch_comp[i+5]<<2)|(ch_comp[i+6]<<1)|ch_comp[i+7];
		}
		free(ch_comp);
	}

	offsetUV(im,enc,ratio);

	if (uv) count=(4*IM_SIZE+1);
	else    count=(4*IM_SIZE);

	for (j=0;j<(IM_DIM);)
	{
		for (i=0;i<(IM_DIM>>1);i++)
		{
			im->im_nhw[count]=nhw_process[j];
			im->im_nhw[count+2]=nhw_process[j+1];
			im->im_nhw[count+4]=nhw_process[j+2];
			im->im_nhw[count+6]=nhw_process[j+3];
			im->im_nhw[count+8]=nhw_process[j+4];
			im->im_nhw[count+10]=nhw_process[j+5];
			im->im_nhw[count+12]=nhw_process[j+6];
			im->im_nhw[count+14]=nhw_process[j+7];
	
			j+=(IM_DIM);
			im->im_nhw[count+16]=nhw_process[j+7];
			im->im_nhw[count+18]=nhw_process[j+6];
			im->im_nhw[count+20]=nhw_process[j+5];
			im->im_nhw[count+22]=nhw_process[j+4];
			im->im_nhw[count+24]=nhw_process[j+3];
			im->im_nhw[count+26]=nhw_process[j+2];
			im->im_nhw[count+28]=nhw_process[j+1];
			im->im_nhw[count+30]=nhw_process[j];

			j+=(IM_DIM);
			count+=32;
		}

		j-=(IM_SIZE-8);
	}

	free(res256);
	free(resIII);

}

void encode_image(image_buffer *im,encode_state *enc, int ratio)
{
	int res_uv;

	im->im_process=(short*)malloc(4*IM_SIZE*sizeof(short));

	//if (im->setup->quality_setting<=LOW6) block_variance_avg(im);

	// Reserve encoder state member buffers here:


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

	im->im_process=(short*)malloc(IM_SIZE*sizeof(short)); // {
	im->im_jpeg=(short*)malloc(IM_SIZE*sizeof(short)); // Work buffer {

	if (im->setup->quality_setting>LOW3) res_uv=4;else res_uv=5;

	encode_uv(im, enc, ratio, res_uv, 0);
	free(im->im_bufferU); // Previously reserved buffer XXX

	encode_uv(im, enc, ratio, res_uv, 1);
	free(im->im_bufferV); // Previously reserved buffer XXX

	free(im->im_jpeg); // Free Work buffer }

	free(im->im_process); // }


	highres_compression(im, enc);

	free(enc->highres_comp);

	wavlts2packet(im,enc);

	free(im->im_nhw);
}


void abort_(const char * s, ...)
{
	va_list args;
	va_start(args, s);
	vfprintf(stderr, s, args);
	fprintf(stderr, "\n");
	va_end(args);
	abort();
}


int menu(char **argv,image_buffer *im,encode_state *os,int rate)
{
	int i;
	FILE *im256;
	unsigned char *im4;
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
	int len,i;
	char OutputFile[200];

	len=strlen(argv[1]);
	memset(argv[1]+len-4,0,4);
	sprintf(OutputFile,"%s.nhw",argv[1]);

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
	if (im->setup->quality_setting>LOW8) fwrite(&enc->nhw_res1_len,2,1,compressed);
	
	if (im->setup->quality_setting>=LOW1)
	{
		fwrite(&enc->nhw_res3_len,2,1,compressed);
		fwrite(&enc->nhw_res3_bit_len,2,1,compressed);
	}
	if (im->setup->quality_setting>LOW3)
	{
		fwrite(&enc->nhw_res4_len,2,1,compressed);
	}
	
	if (im->setup->quality_setting>LOW8) fwrite(&enc->nhw_res1_bit_len,2,1,compressed);

	if (im->setup->quality_setting>=HIGH1)
	{
		fwrite(&enc->nhw_res5_len,2,1,compressed);
		fwrite(&enc->nhw_res5_bit_len,2,1,compressed);
	}

	if (im->setup->quality_setting>HIGH1)
	{
		fwrite(&enc->nhw_res6_len,4,1,compressed);
		fwrite(&enc->nhw_res6_bit_len,2,1,compressed);
		fwrite(&enc->nhw_char_res1_len,2,1,compressed);
	}

	if (im->setup->quality_setting>HIGH2)
	{
		fwrite(&enc->qsetting3_len,2,1,compressed);
	}

	fwrite(&enc->nhw_select1,2,1,compressed);
	fwrite(&enc->nhw_select2,2,1,compressed);
	
	if (im->setup->quality_setting>LOW5)
	{
		fwrite(&enc->highres_comp_len,2,1,compressed);
	}
	
	fwrite(&enc->end_ch_res,2,1,compressed);
	fwrite(enc->tree1,enc->size_tree1,1,compressed);
	fwrite(enc->tree2,enc->size_tree2,1,compressed);
	fwrite(enc->exw_Y,enc->exw_Y_end,1,compressed);
	
	if (im->setup->quality_setting>LOW8)
	{
		fwrite(enc->nhw_res1,enc->nhw_res1_len,1,compressed);
		fwrite(enc->nhw_res1_bit,enc->nhw_res1_bit_len,1,compressed);
		fwrite(enc->nhw_res1_word,enc->nhw_res1_word_len,1,compressed);
	}
	
	if (im->setup->quality_setting>LOW3)
	{
		fwrite(enc->nhw_res4,enc->nhw_res4_len,1,compressed);
	}
	
	if (im->setup->quality_setting>=LOW1)
	{
		fwrite(enc->nhw_res3,enc->nhw_res3_len,1,compressed);
		fwrite(enc->nhw_res3_bit,enc->nhw_res3_bit_len,1,compressed);
		fwrite(enc->nhw_res3_word,enc->nhw_res3_word_len,1,compressed);
	}
	
	if (im->setup->quality_setting>=HIGH1)
	{
		fwrite(enc->nhw_res5,enc->nhw_res5_len,1,compressed);
		fwrite(enc->nhw_res5_bit,enc->nhw_res5_bit_len,1,compressed);
		fwrite(enc->nhw_res5_word,enc->nhw_res5_word_len,1,compressed);
	}

	if (im->setup->quality_setting>HIGH1)
	{
		fwrite(enc->nhw_res6,enc->nhw_res6_len,1,compressed);
		fwrite(enc->nhw_res6_bit,enc->nhw_res6_bit_len,1,compressed);
		fwrite(enc->nhw_res6_word,enc->nhw_res6_word_len,1,compressed);
		fwrite(enc->nhw_char_res1,enc->nhw_char_res1_len,2,compressed);
	}

	if (im->setup->quality_setting>HIGH2)
	{
		fwrite(enc->high_qsetting3,enc->qsetting3_len,4,compressed);

	}

	fwrite(enc->nhw_select_word1,enc->nhw_select1,1,compressed);
	fwrite(enc->nhw_select_word2,enc->nhw_select2,1,compressed);

	if (im->setup->quality_setting>LOW5) 
	{
		fwrite(enc->res_U_64,(IM_DIM<<1),1,compressed);
		fwrite(enc->res_V_64,(IM_DIM<<1),1,compressed);
		fwrite(enc->highres_word,enc->highres_comp_len,1,compressed);
	}
	
	fwrite(enc->ch_res,enc->end_ch_res,1,compressed);
	fwrite(enc->encode,enc->size_data2*4,1,compressed);

	fclose(compressed);

	free(enc->encode);
	
	if (im->setup->quality_setting>LOW3)
	{
		free(enc->nhw_res4);
	}
	
	if (im->setup->quality_setting>LOW8)
	{
	free(enc->nhw_res1);
	free(enc->nhw_res1_bit);
	free(enc->nhw_res1_word);
	}
	
	free(enc->nhw_select_word1);
	free(enc->nhw_select_word2);

	if (im->setup->quality_setting>=LOW1)
	{
		free(enc->nhw_res3);
		free(enc->nhw_res3_bit);
		free(enc->nhw_res3_word);
	}

	if (im->setup->quality_setting>=HIGH1)
	{
		free(enc->nhw_res5);
		free(enc->nhw_res5_bit);
		free(enc->nhw_res5_word);
	}

	if (im->setup->quality_setting>HIGH1)
	{
		free(enc->nhw_res6);
		free(enc->nhw_res6_bit);
		free(enc->nhw_res6_word);
		free(enc->nhw_char_res1);
	}

	if (im->setup->quality_setting>HIGH2)
	{
		free(enc->high_qsetting3);
	}

	free(enc->exw_Y);
	free(enc->ch_res);

	if (im->setup->quality_setting>LOW5)  	
	{
		free(enc->highres_word);
		free(enc->res_U_64);
		free(enc->res_V_64);
	}
}



