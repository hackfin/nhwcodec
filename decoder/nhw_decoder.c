/***************************************************************************
****************************************************************************
*  NHW Image Codec 													       *
*  file: nhw_decoder.c  										           *
*  version: 0.1.6 						     		     				   *
*  last update: $ 02102019 nhw exp $							           *
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
#include <assert.h>

#include "imgio.h"
#include "codec.h"
#include "compression.h"

#define CLIP(x) ( (x<0) ? 0 : ((x>255) ? 255 : x) );

#define FREAD ret = fread

// Generates an index for the full size image
// from coordinates x, y
#define PACK_COORDINATES(x, y)  ( (y)<<1) | (x)
#define UNPACK_COORDINATES(p)  p

int reverse_offset_correction_coding(short v);

void decode_res1(decode_state *os, NhwIndex *nhwres1, NhwIndex *nhwres2);
void decode_res3(decode_state *os, NhwIndex *nhwres3, NhwIndex *nhwres4, NhwIndex *nhwres5,
	NhwIndex *nhwres6);
void decode_res4(image_buffer *im, decode_state *os);
void decode_res5(decode_state *os, NhwIndex *nhwresH1, NhwIndex *nhwresH2);
void decode_res6(image_buffer *im, decode_state *os);
void residual_compensation(image_buffer *im, decode_state *os,
	NhwIndex *nhwresH1, NhwIndex *nhwresH2,
	NhwIndex *nhwres1, NhwIndex *nhwres2,
	NhwIndex *nhwres3, NhwIndex *nhwres4, NhwIndex *nhwres5, NhwIndex *nhwres6
);
int decode_exwY(image_buffer *im, decode_state *os);

// Ready for API/header:
void SWAPOUT_FUNCTION(imgbuf_init)(image_buffer *im, int tile_power)
{
	im->fmt.tile_power = tile_power;
	im->fmt.tile_size = 1 << tile_power;
	im->fmt.end = im->fmt.tile_size * im->fmt.tile_size;
	im->fmt.half = im->fmt.end / 2;
}

void yuv_to_rgb(image_buffer *im)
{
	int i,Y,U,V,R,G,B,m,t;
	unsigned char *icolorY,*icolorU,*icolorV,*iNHW;
	float Y_q_setting,Y_inv;

	iNHW = im->im_buffer4;
    icolorY = (unsigned char*) im->im_bufferY;
	icolorU = (unsigned char*) im->im_bufferU;
	icolorV = (unsigned char*) im->im_bufferV;

	int IM_SIZE = im->fmt.end / 4;
	int quality = im->setup->quality_setting;

	if (quality>=NORM)
	{
		for (m=0;m<4;m++)
		{
			for (i=m*IM_SIZE;i<(m+1)*IM_SIZE;i++)
			{
				//Y = icolorY[i]*298;
				Y = icolorY[i];
				U = icolorU[i]-128;
				V = icolorV[i]-128;

				//Matrix  YCbCr (or YUV) to RGB
				/*R = ((Y         + 409*V + R_COMP)>>8); 
				G = ((Y - 100*U - 208*V + G_COMP)>>8);  
				B = ((Y + 516*U         + B_COMP)>>8);*/
				R = (int)(Y  + 1.402*V +0.5f);   
				G = (int)(Y  -0.34414*U -0.71414*V +0.5f); 
				B = (int)(Y  +1.772*U +0.5f);

				//Clip RGB Values
				if ((R>>8)!=0) R = ( (R<0) ? 0 : 255 );
				if ((G>>8)!=0) G =( (G<0) ? 0 : 255 );
				if ((B>>8)!=0) B =( (B<0) ? 0 : 255 );

				*iNHW++=R; *iNHW++=G; *iNHW++=B;
			}

		}
	}
	else if (quality==LOW1 || quality==LOW2)
	{
		if (quality==LOW1) Y_inv=1.025641; // 1/0.975
		else if (quality==LOW2) Y_inv=1.075269; // 1/0.93

		for (m=0;m<4;m++)
		{
			for (i=m*IM_SIZE,t=0;i<(m+1)*IM_SIZE;i++,t+=3)
			{
				//Y = icolorY[i]*298;
				Y_q_setting =(float)(icolorY[i]*Y_inv);
				U = icolorU[i]-128;
				V = icolorV[i]-128;

				//Matrix  YCbCr (or YUV) to RGB
				/*R = ((Y         + 409*V + R_COMP)>>8); 
				G = ((Y - 100*U - 208*V + G_COMP)>>8);  
				B = ((Y + 516*U         + B_COMP)>>8);*/
				R = (int)(Y_q_setting  + 1.402*V +0.5f);   
				G = (int)(Y_q_setting  -0.34414*U -0.71414*V +0.5f); 
				B = (int)(Y_q_setting  +1.772*U +0.5f);

				//Clip RGB Values
				if ((R>>8)!=0) R = ( (R<0) ? 0 : 255 );
				if ((G>>8)!=0) G =( (G<0) ? 0 : 255 );
				if ((B>>8)!=0) B =( (B<0) ? 0 : 255 );

				*iNHW++=R; *iNHW++=G; *iNHW++=B;
			}

		}
	}
	else if (quality==LOW3) 
	{
		Y_inv=1.063830; // 1/0.94

		for (m=0;m<4;m++)
		{
			for (i=m*IM_SIZE,t=0;i<(m+1)*IM_SIZE;i++,t+=3)
			{
				//Y = icolorY[i]*298;
				Y = icolorY[i];
				U = icolorU[i]-128;
				V = icolorV[i]-128;

				//Matrix  YCbCr (or YUV) to RGB
				/*R = ((Y         + 409*V + R_COMP)>>8); 
				G = ((Y - 100*U - 208*V + G_COMP)>>8);  
				B = ((Y + 516*U         + B_COMP)>>8);*/
				R = (int)((Y + 1.402*V)*Y_inv +0.5f);   
				G = (int)((Y -0.34414*U -0.71414*V)*Y_inv +0.5f); 
				B = (int)((Y +1.772*U)*Y_inv +0.5f);

				//Clip RGB Values
				if ((R>>8)!=0) R = ( (R<0) ? 0 : 255 );
				if ((G>>8)!=0) G =( (G<0) ? 0 : 255 );
				if ((B>>8)!=0) B =( (B<0) ? 0 : 255 );

				*iNHW++=R; *iNHW++=G; *iNHW++=B;
			}

		}
	}
	else if (quality<LOW3) 
	{
		if (quality==LOW4) Y_inv=1.012139; // 1/0.94
		else if (quality==LOW5) Y_inv=1.048174; // 1/0.906
		else if (quality==LOW6) Y_inv=1.138331; // 1/0.8
		else if (quality==LOW7) Y_inv=1.186945; 
		else if (quality==LOW8) Y_inv=1.177434;
		else if (quality==LOW9) Y_inv=1.190611; 
		else if (quality==LOW10) Y_inv=1.281502; 
		else if (quality==LOW11) Y_inv=1.392014;
		else if (quality==LOW12) Y_inv=1.521263;
		else if (quality==LOW13) Y_inv=1.587597;
		else if (quality==LOW14) Y_inv=1.665887;
		else if (quality==LOW15) Y_inv=1.741126;
		else if (quality==LOW16) Y_inv=1.820444;
		else if (quality==LOW17) Y_inv=1.916257;
		else if (quality==LOW18) Y_inv=1.985939;
		else if (quality==LOW19) Y_inv=2.060881;


		for (m=0;m<4;m++)
		{
			for (i=m*IM_SIZE,t=0;i<(m+1)*IM_SIZE;i++,t+=3)
			{
				Y = icolorY[i]*298;
				U = icolorU[i];
				V = icolorV[i];

				//Matrix  YCbCr (or YUV) to RGB
				R =(((int)((Y         + 409*V + R_COMP)*Y_inv +128.5f))>>8); 
				G =(((int)((Y - 100*U - 208*V + G_COMP)*Y_inv +128.5f)) >>8);  
				B = (((int)((Y + 516*U         + B_COMP)*Y_inv +128.5f)) >>8);

				//Clip RGB Values
				if ((R>>8)!=0) R = ( (R<0) ? 0 : 255 );
				if ((G>>8)!=0) G =( (G<0) ? 0 : 255 );
				if ((B>>8)!=0) B =( (B<0) ? 0 : 255 );

				*iNHW++=R; *iNHW++=G; *iNHW++=B;
			}

		}
	}

}


int SWAPOUT_FUNCTION(main)(int argc, char **argv)
{
	image_buffer im;
	decode_state dec;
	FILE *res_image;
	char OutputFile[200];
	int len;
	codec_setup setup;

	im.setup = &setup;

	if (argv[1]==NULL || argv[1]==0)
	{
		printf("\n Copyright (C) 2007-2013 NHW Project (Raphael C.)\n");
		printf("\n-> nhw_decoder.exe filename.nhw\n");
		exit(-1);
	}


	imgbuf_init(&im, 9);

	im.im_process=(short*)calloc(im.fmt.end,sizeof(short));

	memset(&dec, 0, sizeof(dec));
	parse_file(&im, &dec,argv);
	process_hrcomp(&im, &dec);

	/* Decode Image */
	decode_image(&im, &dec, 0);

	free(dec.packet1);
	free(dec.packet2);

	im.im_buffer4=(unsigned char*)malloc(3*im.fmt.end*sizeof(char));

	yuv_to_rgb(&im);

	// Create the Output Decompressed .BMP File
	len=strlen(argv[1]);
	memset(argv[1]+len-4,0,4);
	sprintf(OutputFile,"%sDEC.png",argv[1]);

	res_image = fopen(OutputFile,"wb");

	if( NULL == res_image )
	{
		printf("Failed to open output decompressed file %s\n",OutputFile);
		return -1;
	}

	int s = im.fmt.tile_size;

	write_png(res_image, im.im_buffer4, s, s, 1, 1);

	fclose(res_image);
	free(im.im_bufferY);
	free(im.im_bufferU);
	free(im.im_bufferV);
	free(im.im_buffer4);
	return 0;
}

int process_hrcomp(image_buffer *imd, decode_state *os)
{
	int i,j,ch,e,a=0,run,nhw;
	char uv_small_dc_offset[8][2]={{0,4},{0,-4},{4,0},{-4,0},{4,4},{4,-4},{-4,4},{-4,-4}};

	int IM_SIZE = imd->fmt.end / 4;
	int IM_DIM = imd->fmt.tile_size / 2;

	os->res_comp[0]=os->res_ch[0];

	if ((imd->setup->RES_HIGH&3)==1) goto L6; 
	if ((imd->setup->RES_HIGH&3)==2) goto L8;

	for (j=1,i=1,a=0;j<(IM_SIZE>>2);i++)
	{
		if (os->res_ch[i]>=128)
		{
			if (imd->setup->quality_setting>LOW5) os->res_comp[j++]=os->highres_comp[a++];
			os->res_comp[j++]=((os->res_ch[i]-128)<<1);
		}
		else
		{
			if (os->res_ch[i]<16)
			{
				run=(os->res_ch[i]>>3)&1;
				nhw=os->res_comp[j-1];
				for (e=0;e<(run+2);e++) os->res_comp[j++]=nhw;
				if ((os->res_ch[i]&7)==0) continue;
				else if ((os->res_ch[i]&7)==1) {os->res_comp[j]=os->res_comp[j-1]+2;j++;}
				else if ((os->res_ch[i]&7)==2) 
				{
					os->res_comp[j]=os->res_comp[j-1]+2;j++;
					os->res_comp[j]=os->res_comp[j-1]-2;j++;
				}
				else if ((os->res_ch[i]&7)==3) 
				{
					os->res_comp[j]=os->res_comp[j-1]+2;j++;
					os->res_comp[j]=os->res_comp[j-1];j++;
					//os->res_comp[j]=os->res_comp[j-1];j++;
				}
				else if ((os->res_ch[i]&7)==4) 
				{
					os->res_comp[j]=os->res_comp[j-1]-2;j++;
					os->res_comp[j]=os->res_comp[j-1]+2;j++;
					//os->res_comp[j]=os->res_comp[j-1];j++;
				}
				else if ((os->res_ch[i]&7)==5) 
				{
					os->res_comp[j]=os->res_comp[j-1]-2;j++;
					os->res_comp[j]=os->res_comp[j-1];j++;
				}
				else if ((os->res_ch[i]&7)==6) 
				{
					os->res_comp[j]=os->res_comp[j-1]-2;j++;
				}
				else if ((os->res_ch[i]&7)==7) 
				{
					os->res_comp[j]=os->res_comp[j-1]+4;j++;
				}
			}
			else if (os->res_ch[i]<32)
			{
				if (os->res_ch[i]>=24)
				{
					os->res_comp[j]=os->res_comp[j-1]+4;
					j++;

					ch = ((os->res_ch[i])&7);
					ch <<=1;
					os->res_comp[j]= (ch-8)+ os->res_comp[j-1];
					j++;
				}
				else
				{
					os->res_comp[j]=os->res_comp[j-1]+2;
					j++;

					ch = ((os->res_ch[i])&7);
					ch <<=1;
					os->res_comp[j]= (ch-8)+ os->res_comp[j-1];
					j++;
				}
			}
			else if (os->res_ch[i]<64)
			{
				os->res_ch[i]-=32;
				ch = (os->res_ch[i])>>3;
				ch <<=1;
				os->res_comp[j]= (ch-6)+ os->res_comp[j-1];
				j++;

				ch = ((os->res_ch[i])&7);
				ch <<=1;
				os->res_comp[j]= (ch-8)+ os->res_comp[j-1];
				j++;
			}
			else
			{
				os->res_ch[i]-=64;
				ch = ((os->res_ch[i])>>1)&31;
				ch <<=1;
				os->res_comp[j]= (ch-32)+ os->res_comp[j-1];
				j++;

				ch = ((os->res_ch[i])&1);
				ch<<=3;
				i++;
				ch |= ((os->res_ch[i])>>5);
				ch <<=1;
				os->res_comp[j]= (ch-16)+ os->res_comp[j-1];
				j++;
				
				ch = ((os->res_ch[i])&31);
				ch <<=1;
				os->res_comp[j]= (ch-32)+ os->res_comp[j-1];
				j++;
			}
		}
	}

	goto L7;

L6: for (j=1,i=1,a=0;j<(IM_SIZE>>2);i++)
	{
		if (os->res_ch[i]>=128)
		{
			if (imd->setup->quality_setting>LOW5) os->res_comp[j++]=os->highres_comp[a++];
			os->res_comp[j++]=((os->res_ch[i]-128)<<1);
		}
		else
		{
			if (os->res_ch[i]<32)
			{
				run=(os->res_ch[i]>>2)&7;
				nhw=os->res_comp[j-1];
				for (e=0;e<(run+2);e++) os->res_comp[j++]=nhw;
				if ((os->res_ch[i]&3)==0) continue;
				else if ((os->res_ch[i]&3)==1) {os->res_comp[j]=os->res_comp[j-1]+2;j++;}
				else if ((os->res_ch[i]&3)==2) {os->res_comp[j]=os->res_comp[j-1]-2;j++;}
				else if ((os->res_ch[i]&3)==3) {os->res_comp[j]=os->res_comp[j-1];j++;}
			}
			else if (os->res_ch[i]<64)
			{
				os->res_ch[i]-=32;
				ch = (os->res_ch[i])>>3;
				ch <<=1;
				os->res_comp[j]= (ch-4)+ os->res_comp[j-1];
				j++;

				ch = ((os->res_ch[i])&7);
				ch <<=1;
				os->res_comp[j]= (ch-8)+ os->res_comp[j-1];
				j++;
			}
			else
			{
				os->res_ch[i]-=64;
				ch = ((os->res_ch[i])>>1)&31;
				ch <<=1;
				os->res_comp[j]= (ch-32)+ os->res_comp[j-1];
				j++;

				ch = ((os->res_ch[i])&1);
				ch<<=3;
				i++;
				ch |= ((os->res_ch[i])>>5);
				ch <<=1;
				os->res_comp[j]= (ch-16)+ os->res_comp[j-1];
				j++;
				
				ch = ((os->res_ch[i])&31);
				ch <<=1;
				os->res_comp[j]= (ch-32)+ os->res_comp[j-1];
				j++;
			}
		}
	}

	goto L7;

L8: for (j=1,i=1,a=0;j<(IM_SIZE>>2);i++)
	{
		if (os->res_ch[i]>=128)
		{
			if (imd->setup->quality_setting>LOW5) os->res_comp[j++]=os->highres_comp[a++];
			os->res_comp[j++]=((os->res_ch[i]-128)<<1);
		}
		else
		{
			if (os->res_ch[i]<64)
			{
				run=os->res_ch[i]&63;
				nhw=os->res_comp[j-1];
				for (e=0;e<(run+2);e++) os->res_comp[j++]=nhw;
			}
			else
			{
				os->res_ch[i]-=64;
				ch = ((os->res_ch[i])>>1)&31;
				ch <<=1;
				os->res_comp[j]= (ch-32)+ os->res_comp[j-1];
				j++;

				ch = ((os->res_ch[i])&1);
				ch<<=3;
				i++;
				ch |= ((os->res_ch[i])>>5);
				ch <<=1;
				os->res_comp[j]= (ch-16)+ os->res_comp[j-1];
				j++;
				
				ch = ((os->res_ch[i])&31);
				ch <<=1;
				os->res_comp[j]= (ch-32)+ os->res_comp[j-1];
				j++;
			}
		}
	}

L7:	os->res_comp[(IM_SIZE>>2)]=os->res_ch[i++];

	if (imd->setup->quality_setting>LOW5) free(os->highres_comp);

	//for (i=0;i<1200;i+=4) printf("%d %d %d %d\n",os->res_comp[i],os->res_comp[i+1],os->res_comp[i+2],os->res_comp[i+3]);

	for (j=((IM_SIZE>>2)+1);j<((IM_SIZE>>2)+(IM_SIZE>>3));i++)
	{
		if (os->res_ch[i]>=192)
		{
			os->res_ch[i]-=192;

			ch = (os->res_ch[i])>>2;
			os->res_comp[j]= uv_small_dc_offset[ch][0]+ os->res_comp[j-1];
			j++;
			os->res_comp[j]= uv_small_dc_offset[ch][1]+ os->res_comp[j-1];
			j++;

			ch=os->res_ch[i]&3;

			if (!ch)
			{
				os->res_comp[j]=os->res_comp[j-1];j++;
			}
			else if (ch==1)
			{
				os->res_comp[j]=os->res_comp[j-1]+4;j++;
			}
			else if (ch==2)
			{
				os->res_comp[j]=os->res_comp[j-1]-4;j++;
			}
			else
			{
				os->res_comp[j]=os->res_comp[j-1]+8;j++;
			}
		}
		else if (os->res_ch[i]>=128)
		{
			os->res_comp[j++]=(os->res_ch[i]-128)<<2;
		}
		else
		{
			if (os->res_ch[i]>=64)
			{
				run=(os->res_ch[i]>>3)&7;
				nhw=os->res_comp[j-1];
				if (run==7)
				{
					run=(os->res_ch[i]&7)+7;
					for (e=0;e<(run+2);e++) os->res_comp[j++]=nhw;
				}
				else
				{
					for (e=0;e<(run+2);e++) os->res_comp[j++]=nhw;
					if ((os->res_ch[i]&7)==0) continue;
					else if ((os->res_ch[i]&7)==1) {os->res_comp[j]=os->res_comp[j-1]+4;j++;}
					else if ((os->res_ch[i]&7)==2) 
					{
						os->res_comp[j]=os->res_comp[j-1]+4;j++;
						os->res_comp[j]=os->res_comp[j-1]-4;j++;
					}
					else if ((os->res_ch[i]&7)==3) 
					{
						os->res_comp[j]=os->res_comp[j-1]+4;j++;
						os->res_comp[j]=os->res_comp[j-1]-4;j++;
						os->res_comp[j]=os->res_comp[j-1];j++;
					}
					else if ((os->res_ch[i]&7)==4) 
					{
						os->res_comp[j]=os->res_comp[j-1]-4;j++;
						os->res_comp[j]=os->res_comp[j-1]+4;j++;
						os->res_comp[j]=os->res_comp[j-1];j++;
					}
					else if ((os->res_ch[i]&7)==5) 
					{
						os->res_comp[j]=os->res_comp[j-1]-4;j++;
						os->res_comp[j]=os->res_comp[j-1]+4;j++;
					}
					else if ((os->res_ch[i]&7)==6) 
					{
						os->res_comp[j]=os->res_comp[j-1]-4;j++;
					}
					else if ((os->res_ch[i]&7)==7) 
					{
						os->res_comp[j]=os->res_comp[j-1]+8;j++;
					}
				}
			}
			else
			{
				ch = (os->res_ch[i])>>3;
				ch <<=2;
				os->res_comp[j]= (ch-16)+ os->res_comp[j-1];
				j++;

				ch = ((os->res_ch[i])&7);
				ch <<=2;
				os->res_comp[j]= (ch-16)+ os->res_comp[j-1];
				j++;
				//if (os->res_ch[i]&1) { os->res_comp[j]=os->res_comp[j-1];j++;}
			}
		}
	}

	free(os->res_ch);

	if (imd->setup->quality_setting>LOW5)
	{
		for (i=0,e=(IM_SIZE>>2);i<(IM_DIM<<1);i++)
		{
			ch=(os->res_U_64[i]>>7);os->res_comp[e++]+=(ch<<1);

			ch=((os->res_U_64[i]>>6)&1);os->res_comp[e++]+=(ch<<1);

			ch=((os->res_U_64[i]>>5)&1);os->res_comp[e++]+=(ch<<1);

			ch=((os->res_U_64[i]>>4)&1);os->res_comp[e++]+=(ch<<1);

			ch=((os->res_U_64[i]>>3)&1);os->res_comp[e++]+=(ch<<1);

			ch=((os->res_U_64[i]>>2)&1);os->res_comp[e++]+=(ch<<1);

			ch=((os->res_U_64[i]>>1)&1);os->res_comp[e++]+=(ch<<1);

			ch=((os->res_U_64[i])&1);os->res_comp[e++]+=(ch<<1);
		}

		free(os->res_U_64);

		for (i=0,e=((IM_SIZE>>2)+(IM_SIZE>>4));i<(IM_DIM<<1);i++)
		{
			ch=(os->res_V_64[i]>>7);os->res_comp[e++]+=(ch<<1);

			ch=((os->res_V_64[i]>>6)&1);os->res_comp[e++]+=(ch<<1);

			ch=((os->res_V_64[i]>>5)&1);os->res_comp[e++]+=(ch<<1);

			ch=((os->res_V_64[i]>>4)&1);os->res_comp[e++]+=(ch<<1);

			ch=((os->res_V_64[i]>>3)&1);os->res_comp[e++]+=(ch<<1);

			ch=((os->res_V_64[i]>>2)&1);os->res_comp[e++]+=(ch<<1);

			ch=((os->res_V_64[i]>>1)&1);os->res_comp[e++]+=(ch<<1);

			ch=((os->res_V_64[i])&1);os->res_comp[e++]+=(ch<<1);
		}

		free(os->res_V_64);
	}

	return(imd->setup->wvlts_order);

}

void process_nhwcodes(image_buffer *im, short *im_nhw, int IM_SIZE, int IM_DIM)
{
	int scan;
	int count = 0;
	int i, j;

	for (i=0;i<(2*IM_SIZE);i+=(2*IM_DIM))
	{
		for (scan=i,j=0;j<(2*IM_DIM);j++,scan++)
		{
			if (IS_NHW_CODE(im_nhw[scan]))
			{
				if (im_nhw[scan]==CODE_NHW_NB_1)
				{
					im->im_jpeg[scan-1]=5;im->im_jpeg[scan+1]=5;
					if (j<IM_DIM) im->im_jpeg[scan]=5;else im->im_jpeg[scan]=6;
				}
				else if (im_nhw[scan]==CODE_NHW_NB_2)
				{
					im->im_jpeg[scan-1]=-5;im->im_jpeg[scan+1]=-5;
					if (j<IM_DIM) im->im_jpeg[scan]=-6;else im->im_jpeg[scan]=-7;
				}
				else if (im_nhw[scan]==CODE_NHW_NB_3)
				{
					im->im_jpeg[scan]=5;im->im_jpeg[scan+1]=5;
					im->im_jpeg[scan+(2*IM_DIM)]=5;im->im_jpeg[scan+(2*IM_DIM+1)]=5;
				}
				else if (im_nhw[scan]==CODE_NHW_NB_4)
				{
					im->im_jpeg[scan]=-5;im->im_jpeg[scan+1]=-5;
					im->im_jpeg[scan+(2*IM_DIM)]=-5;im->im_jpeg[scan+(2*IM_DIM+1)]=-5;
				}
				else if (im_nhw[scan]==CODE_NHW_NB_5)
				{
					im->im_jpeg[scan]=-6;im->im_jpeg[scan+1]=-6;
				}
				else if (im_nhw[scan]==CODE_NHW_NB_6)
				{
					im->im_jpeg[scan]=6;im->im_jpeg[scan+1]=6;
				}
			}
		}
	}

	int mask = im->fmt.tile_size - 1;

	for (i=(2*IM_SIZE);i<(4*IM_SIZE);i+=(2*IM_DIM))
	{
		for (scan=i,j=0;j<(IM_DIM);j++,scan++)
		{
			if (IS_NHW_CODE(im_nhw[scan]))
			{
				if (im_nhw[scan]==CODE_NHW_NB_1)
				{
					im->im_jpeg[scan-1]=5;im->im_jpeg[scan]=6;im->im_jpeg[scan+1]=5;
				}
				else if (im_nhw[scan]==CODE_NHW_NB_2)
				{
					im->im_jpeg[scan-1]=-5;im->im_jpeg[scan]=-7;im->im_jpeg[scan+1]=-5;
				}
				else if (im_nhw[scan]==CODE_NHW_NB_5)
				{
					if (scan<(2*IM_SIZE)) {im->im_jpeg[scan]=-7;im->im_jpeg[scan+1]=-7;}
					else if ((scan&mask)<IM_DIM) {im->im_jpeg[scan]=-7;im->im_jpeg[scan+1]=-7;}
					else {im->im_jpeg[scan-IM_DIM]=-7;im->im_jpeg[scan-(3*IM_DIM)]=-7;im->im_jpeg[scan]=0;}
				}
				else if (im_nhw[scan]==CODE_NHW_NB_6)
				{
					if (scan<(2*IM_SIZE)) {im->im_jpeg[scan]=7;im->im_jpeg[scan+1]=7;}
					else if ((scan&mask)<IM_DIM) {im->im_jpeg[scan]=7;im->im_jpeg[scan+1]=7;}
					else {im->im_jpeg[scan-IM_DIM]=7;im->im_jpeg[scan-(3*IM_DIM)]=7;im->im_jpeg[scan]=0;}
				}
			}
		}
	}

	for (i=(2*IM_SIZE);i<(4*IM_SIZE);i+=(2*IM_DIM))
	{
		for (scan=i+IM_DIM,j=IM_DIM;j<(2*IM_DIM);j++,scan++)
		{
			if (IS_NHW_CODE(im_nhw[scan]))
			{
				if (im_nhw[scan]==CODE_NHW_NB_1)
				{
					im->im_jpeg[scan-1]=5;im->im_jpeg[scan]=6;im->im_jpeg[scan+1]=5;
				}
				else if (im_nhw[scan]==CODE_NHW_NB_2)
				{
					im->im_jpeg[scan-1]=-5;im->im_jpeg[scan]=-7;im->im_jpeg[scan+1]=-5;
				}
				else if (im_nhw[scan]==CODE_NHW_NB_5)
				{
					if (scan<(2*IM_SIZE)) {im->im_jpeg[scan]=-7;im->im_jpeg[scan+1]=-7;}
					else if ((scan&mask)<IM_DIM) {im->im_jpeg[scan]=-7;im->im_jpeg[scan+1]=-7;}
					else {im->im_jpeg[scan-IM_DIM]=-7;im->im_jpeg[scan-(3*IM_DIM)]=-7;im->im_jpeg[scan]=0;}
				}
				else if (im_nhw[scan]==CODE_NHW_NB_6)
				{
					if (scan<(2*IM_SIZE)) {im->im_jpeg[scan]=7;im->im_jpeg[scan+1]=7;}
					else if ((scan&mask)<IM_DIM) {im->im_jpeg[scan]=7;im->im_jpeg[scan+1]=7;}
					else {im->im_jpeg[scan-IM_DIM]=7;im->im_jpeg[scan-(3*IM_DIM)]=7;im->im_jpeg[scan]=0;}
				}
			}
			else if (abs(im_nhw[scan])>8 && abs(im_nhw[scan])<16 && im->setup->quality_setting<HIGH3)
			{
				if (j>IM_DIM && j<((2*IM_DIM)-1))
				{
					if (abs(im_nhw[scan-1])<8) count++;
					if (abs(im_nhw[scan+1])<8) count++;
					if (abs(im_nhw[scan-(2*IM_DIM)])<8) count++;
					if (i < (4*IM_SIZE)-(2*IM_DIM)) { // XXX HACK
						if (abs(im_nhw[scan+(2*IM_DIM)])<8) count++;
					}

					if (count>=2)
					{
						if (im_nhw[scan]>0) im_nhw[scan]++;
						else im_nhw[scan]--;
					}

					count=0;
				}
			}
		}
	}

}


int decode_u(image_buffer *im, decode_state *os, int exw1)
{
	int res, stage;
	unsigned char *nhw_scale;
	int i, j, count, scan, nhw;
	short *pr = im->im_process;
	short *im_nhw;
	int end_transform;
	unsigned char *nhw_chr;
	int IM_SIZE = im->fmt.end / 4;
	int IM_DIM = im->fmt.tile_size / 2;


	im->im_jpeg=(short*)malloc(IM_SIZE*sizeof(short));
	im_nhw=(short*)im->im_jpeg;
	pr=(short*)im->im_nhw3;

	for (j=0,count=0;j<(IM_DIM);)
	{
		for (i=0;i<(IM_DIM>>1);i++)
		{
			im_nhw[j]=pr[count];
			im_nhw[j+1]=pr[count+2];
			im_nhw[j+2]=pr[count+4];
			im_nhw[j+3]=pr[count+6];
			im_nhw[j+4]=pr[count+8];
			im_nhw[j+5]=pr[count+10];
			im_nhw[j+6]=pr[count+12];
			im_nhw[j+7]=pr[count+14];
	
			j+=(IM_DIM);
			im_nhw[j+7]=pr[count+16];
			im_nhw[j+6]=pr[count+18];
			im_nhw[j+5]=pr[count+20];
			im_nhw[j+4]=pr[count+22];
			im_nhw[j+3]=pr[count+24];
			im_nhw[j+2]=pr[count+26];
			im_nhw[j+1]=pr[count+28];
			im_nhw[j]=pr[count+30];

			j+=(IM_DIM);
			count+=32;
		}

		j-=((IM_SIZE)-8);
	}

	/*for (i=0;i<(IM_SIZE>>1);i+=(IM_DIM))
	{
		for (scan=i,j=0;j<(IM_DIM>>1);j++,scan++) 
		{
			if (im->im_jpeg[scan]>0) im->im_jpeg[scan]--;
			else if (im->im_jpeg[scan]<0) im->im_jpeg[scan]++;
		}
	}*/

	if (im->setup->quality_setting>LOW5)
	{
		for (i=0,nhw=(IM_SIZE>>2);i<(IM_SIZE>>2);i+=IM_DIM)
		{
			for (j=0;j<(IM_DIM>>2);j++) 
			{
				im->im_jpeg[j+i]=os->res_comp[nhw++];
			}
		}
	}
	else
	{	
		for (i=0,nhw=(IM_SIZE>>2);i<(IM_SIZE>>2);i+=IM_DIM)
		{
			for (j=0;j<(IM_DIM>>2);j++) 
			{
				im->im_jpeg[j+i]=os->res_comp[nhw++]+1;
			}
		}

	}

	exw1+=2;
	i=exw1;

	for (;i<os->exw_Y_end;i+=3,exw1+=3)
	{
		if (!os->exw_Y[i] && !os->exw_Y[i+1]) break;

		if (os->exw_Y[i+1]>=128) {scan=os->exw_Y[i+2]+255;os->exw_Y[i+1]-=128;}
		else {scan=-os->exw_Y[i+2];}

		count=(os->exw_Y[i]<<im->fmt.tile_power)+os->exw_Y[i+1];
		assert(count < im->fmt.end / 4);

		im->im_jpeg[count]=scan;

	}

	im->im_process=(short*)malloc(IM_SIZE*sizeof(short));

	end_transform=0;
	//for (stage=wavelet_order-1;stage>=0;stage--) dec_wavelet_synthesis(im,IM_DIM>>stage,end_transform++,0);

	dec_wavelet_synthesis(im,(IM_DIM>>1),end_transform++,0);

	pr=(short*)im->im_process;
	im_nhw=(short*)im->im_jpeg;

	for (i=0;i<(IM_SIZE>>1);i+=IM_DIM)
	{
		for (scan=i+(IM_DIM>>1),j=(IM_DIM>>1);j<IM_DIM;j++,scan++)
		{
			if (im_nhw[scan]>5000)
			{
				if (im_nhw[scan]==5005)
				{
					pr[scan-(IM_DIM>>1)]-=4;pr[scan-(IM_DIM>>1)+1]-=4;im_nhw[scan]=0;
				}
				else if (im_nhw[scan]==5006)
				{
					pr[scan-(IM_DIM>>1)]+=4;pr[scan-(IM_DIM>>1)+1]+=4;im_nhw[scan]=0;
				}
				else if (im_nhw[scan]==5003) 
				{
					pr[scan-(IM_DIM>>1)]-=6;im_nhw[scan]=0;
				}
				else if (im_nhw[scan]==5004)
				{
					pr[scan-(IM_DIM>>1)]+=6;im_nhw[scan]=0;
				}
			}
		}
	}

	for (i=(IM_SIZE>>1);i<(IM_SIZE);i+=IM_DIM)
	{
		for (scan=i,j=0;j<(IM_DIM);j++,scan++)
		{
			if (im_nhw[scan]>5000)
			{
				if (im_nhw[scan]==5005)
				{
					if (j<(IM_DIM>>1))
					{
						pr[scan-(IM_SIZE>>1)]-=4;pr[scan-(IM_SIZE>>1)+1]-=4;im_nhw[scan]=0;
					}
					else
					{
						pr[scan-(IM_SIZE>>1)-(IM_DIM>>1)]-=4;pr[scan-(IM_SIZE>>1)-(IM_DIM>>1)+1]-=4;im_nhw[scan]=0;
					}
				}
				else if (im_nhw[scan]==5006)
				{
					if (j<(IM_DIM>>1))
					{
						pr[scan-(IM_SIZE>>1)]+=4;pr[scan-(IM_SIZE>>1)+1]+=4;im_nhw[scan]=0;
					}
					else
					{
						pr[scan-(IM_SIZE>>1)-(IM_DIM>>1)]+=4;pr[scan-(IM_SIZE>>1)-(IM_DIM>>1)+1]+=4;im_nhw[scan]=0;
					}
				}
				else if (im_nhw[scan]==5003) 
				{
					if (j<(IM_DIM>>1))
					{
						pr[scan-(IM_SIZE>>1)]-=6;im_nhw[scan]=0;
					}
					else
					{
						pr[scan-(IM_SIZE>>1)-(IM_DIM>>1)]-=6;im_nhw[scan]=0;
					}
				}
				else if (im_nhw[scan]==5004)
				{
					if (j<(IM_DIM>>1))
					{
						pr[scan-(IM_SIZE>>1)]+=6;im_nhw[scan]=0;
					}
					else 
					{
						pr[scan-(IM_SIZE>>1)-(IM_DIM>>1)]+=6;im_nhw[scan]=0;
					}
				}
			}
		}
	}

	for (i=0;i<(IM_DIM>>1);i++,im_nhw+=(IM_DIM))
	{
		for (scan=i,j=0;j<(IM_DIM>>1);j++,scan+=(IM_DIM)) im_nhw[j]=pr[scan];
	}

	dec_wavelet_synthesis(im,IM_DIM,end_transform,0);

	free(im->im_jpeg);

	im_nhw=(short*)im->im_process;
	
	if (im->setup->quality_setting<=LOW6) count=35;
	else count=60;

	for (i=IM_DIM;i<IM_SIZE-IM_DIM;i+=(IM_DIM))
	{
		for (scan=i+1,j=1;j<(IM_DIM-1);j++,scan++)
		{
			res	   =   (im_nhw[scan]<<3) -
						im_nhw[scan-1]-im_nhw[scan+1]-
						im_nhw[scan-(IM_DIM)]-im_nhw[scan+(IM_DIM)]-
						im_nhw[scan-(IM_DIM+1)]-im_nhw[scan+(IM_DIM-1)]-
						im_nhw[scan-(IM_DIM-1)]-im_nhw[scan+(IM_DIM+1)];

			if (abs(res)>count)
			{
				if (res>0)
				{
					if (res>160) im_nhw[scan]+=3;
					else if (res>count) im_nhw[scan]+=2;
				}
				else 
				{
					if (res<-160) im_nhw[scan]-=3;
					else if (res<-count) im_nhw[scan]-=2;
				}
			}
		}
	}

	for (i=0;i<IM_SIZE;i++) 
	{
		if ((im_nhw[i]>>8)!=0) 
		{
			if (im_nhw[i]<0) im_nhw[i]=0;
			else if (im_nhw[i]>255) im_nhw[i]=255;
		}
	}

	im->scale=(unsigned char*)malloc(2*IM_SIZE*sizeof(char));
	nhw_scale=(unsigned char*)im->scale;

	/*for (j=0;j<IM_DIM;j++)
	{
		for (i=0;i<(IM_SIZE-IM_DIM);i+=(IM_DIM))
		{
			im->scale[j+(2*i)]=im->im_process[j+i];
			im->scale[j+(2*i)+IM_DIM]=((im->im_process[j+i]+im->im_process[j+i+IM_DIM]+1)>>1);
		}

		im->scale[j+(2*IM_SIZE-2*IM_DIM)]=im->im_process[j+(IM_SIZE-IM_DIM)];
		im->scale[j+(2*IM_SIZE-IM_DIM)]=im->im_process[j+(IM_SIZE-IM_DIM)];

	}*/

	// faster version
	for (j=0;j<IM_DIM;j++)
	{
		stage=j;count=j;
		for (;count<(IM_SIZE-IM_DIM);count+=IM_DIM,stage+=(2*IM_DIM)) 
		{
			nhw_scale[stage]=(im_nhw[count]);
			nhw_scale[stage+IM_DIM]=((im_nhw[count]+im_nhw[count+IM_DIM]+1)>>1);
		}

		nhw_scale[j+(2*IM_SIZE-2*IM_DIM)]=im_nhw[j+(IM_SIZE-IM_DIM)];
		nhw_scale[j+(2*IM_SIZE-IM_DIM)]=im_nhw[j+(IM_SIZE-IM_DIM)];

	}

	free(im->im_process);
	im->im_bufferU=(unsigned char*)malloc(4*IM_SIZE*sizeof(char));
	nhw_chr=(unsigned char*)im->im_bufferU;

	/*for (i=0,e=0;i<(4*IM_SIZE);i+=(IM_DIM*2),e+=IM_DIM)
	{
		for (j=0;j<IM_DIM-1;j++)
		{
			im->im_bufferU[(j*2)+i]=im->scale[j+e];
			im->im_bufferU[(j*2)+1+i]=(im->scale[j+e]+im->scale[j+e+1]+1)>>1;
		}

		im->im_bufferU[(IM_DIM*2)-2+i]=im->scale[IM_DIM-1+e];
		im->im_bufferU[(IM_DIM*2)-1+i]=im->scale[IM_DIM-1+e];
	}*/

	//faster version
	for (count=0,stage=0;count<(4*IM_SIZE);count+=(IM_DIM*2),stage+=IM_DIM)
	{
		for (j=0;j<IM_DIM-1;j++,stage++) 
		{
			nhw_chr[count++]=nhw_scale[stage];
			nhw_chr[count++]=(nhw_scale[stage]+nhw_scale[stage+1]+1)>>1;
		}

		im->im_bufferU[count++]=nhw_scale[stage];
		im->im_bufferU[count]=nhw_scale[stage];

		count-=(2*IM_DIM-1);
		stage-=(IM_DIM-1);
	}

	free(im->scale);

	im->im_jpeg=(short*)malloc(IM_SIZE*sizeof(short));

	return exw1;
}

static
void process_nhw_offset_codes(short *pr, short *im_nhw, int IM_DIM, int IM_SIZE)
{
	int i, j, scan;
	int n = IM_DIM / 2;
	int half = IM_SIZE / 2;
	
	// <-  n  ->
	//  LL   HL   **   **
	//  LH   HH   **   **    <- half
	//  ..   ..   ..   ..
	//  ..   ..   ..   ..
 
	for (i=0;i<half;i+=IM_DIM)
	{
		short *nhw = &im_nhw[i+n];
		short *p = &pr[i];
		for (j=n;j<IM_DIM;j++,nhw++,p++) {
			if (*nhw>5000) {
				switch (*nhw) {
					case 5005: *p-=4; p[1]-=4;*nhw=0; break;
					case 5006: *p+=4; p[1]+=4;*nhw=0; break;
					case 5003: *p-=6; *nhw=0; break;
					case 5004: *p+=6; *nhw=0; break;
				}
			}
		}
	}

	// <-  n  ->
	//  LL   HL   ..   ..
	//  LH   HH   ..   ..    <- half
	//  ..   ..   ..   ..
	//  ..   ..   ..   ..

	for (i=half;i<(IM_SIZE);i+=IM_DIM)
	{
		for (scan=i,j=0;j<(IM_DIM);j++,scan++)
		{
			if (im_nhw[scan]>5000)
			{
				switch (im_nhw[scan]) {
				case 5005:
					if (j<n) {
						pr[scan-half]-=4;pr[scan-half+1]-=4;im_nhw[scan]=0;
					} else {
						pr[scan-half-n]-=4;pr[scan-half-n+1]-=4;im_nhw[scan]=0;
					}
					break;
				case 5006:
					if (j<n) {
						pr[scan-half]+=4;pr[scan-half+1]+=4;im_nhw[scan]=0;
					} else {
						pr[scan-half-n]+=4;pr[scan-half-n+1]+=4;im_nhw[scan]=0;
					}
					break;
				case 5003: 
					if (j<n) {
						pr[scan-half]-=6;im_nhw[scan]=0;
					} else {
						pr[scan-half-n]-=6;im_nhw[scan]=0;
					}
					break;
				case 5004:
					if (j<n) {
						pr[scan-half]+=6;im_nhw[scan]=0;
					} else {
						pr[scan-half-n]+=6;im_nhw[scan]=0;
					}
					break;
				}
			}
		}
	}

	for (i=0;i<n;i++,im_nhw+=(IM_DIM))
	{
		for (scan=i,j=0;j<(IM_DIM>>1);j++,scan+=(IM_DIM)) im_nhw[j]=pr[scan];
	}
}

void decode_v(image_buffer *im, decode_state *os, int exw1)
{
	int res, stage;
	unsigned char *nhw_scale;
	int i, j, count, scan, nhw;
	short *pr = im->im_process;
	short *im_nhw;
	int end_transform;
	unsigned char *nhw_chr;
	int IM_SIZE = im->fmt.end / 4;
	int IM_DIM = im->fmt.tile_size / 2;

	im_nhw=(short*)im->im_jpeg;
	pr=(short*)im->im_nhw3;

	for (j=0,count=1;j<(IM_DIM);)
	{
		for (i=0;i<(IM_DIM>>1);i++)
		{
			im_nhw[j]=pr[count];
			im_nhw[j+1]=pr[count+2];
			im_nhw[j+2]=pr[count+4];
			im_nhw[j+3]=pr[count+6];
			im_nhw[j+4]=pr[count+8];
			im_nhw[j+5]=pr[count+10];
			im_nhw[j+6]=pr[count+12];
			im_nhw[j+7]=pr[count+14];
	
			j+=(IM_DIM);
			im_nhw[j+7]=pr[count+16];
			im_nhw[j+6]=pr[count+18];
			im_nhw[j+5]=pr[count+20];
			im_nhw[j+4]=pr[count+22];
			im_nhw[j+3]=pr[count+24];
			im_nhw[j+2]=pr[count+26];
			im_nhw[j+1]=pr[count+28];
			im_nhw[j]=pr[count+30];

			j+=(IM_DIM);
			count+=32;
		}

		j-=((IM_SIZE)-8);
	}


	/*for (i=0;i<(IM_SIZE>>1);i+=(IM_DIM))
	{
		for (scan=i,j=0;j<(IM_DIM>>1);j++,scan++) 
		{
			if (im->im_jpeg[scan]>0) im->im_jpeg[scan]--;
			else if (im->im_jpeg[scan]<0) im->im_jpeg[scan]++;
		}
	}*/

	if (im->setup->quality_setting>LOW5)
	{
		for (i=0,nhw=(IM_SIZE>>2)+(IM_SIZE>>4);i<(IM_SIZE>>2);i+=IM_DIM)
		{
			for (j=0;j<(IM_DIM>>2);j++) 
			{
				im->im_jpeg[j+i]=os->res_comp[nhw++];
			}
		}
	}
	else
	{	
		for (i=0,nhw=(IM_SIZE>>2)+(IM_SIZE>>4);i<(IM_SIZE>>2);i+=IM_DIM)
		{
			for (j=0;j<(IM_DIM>>2);j++) 
			{
				im->im_jpeg[j+i]=os->res_comp[nhw++]+1;
			}
		}

	}
	
	free(os->res_comp);

	exw1+=2;
	i=exw1;

	for (;i<os->exw_Y_end;i+=3)
	{
		if (os->exw_Y[i+1]>=128) {scan=os->exw_Y[i+2]+255;os->exw_Y[i+1]-=128;}
		else {scan=-os->exw_Y[i+2];}

		count=(os->exw_Y[i] << im->fmt.tile_power)+os->exw_Y[i+1];

		assert(count < im->fmt.end / 4);
		im->im_jpeg[count]=scan;

	}

	free(os->exw_Y);
	im->im_process=(short*)malloc(IM_SIZE*sizeof(short));

	end_transform=0;
	//for (stage=wavelet_order-1;stage>=0;stage--) dec_wavelet_synthesis(im,IM_DIM>>stage,end_transform++,0);

	dec_wavelet_synthesis(im,(IM_DIM>>1),end_transform++,0);

	process_nhw_offset_codes(im->im_process, im->im_jpeg, IM_DIM, IM_SIZE);

	dec_wavelet_synthesis(im,IM_DIM,end_transform,0);

	free(im->im_jpeg);

	im_nhw=(short*)im->im_process;
	
	if (im->setup->quality_setting<=LOW6) count=35;
	else count=60;

	for (i=IM_DIM;i<IM_SIZE-IM_DIM;i+=(IM_DIM))
	{
		for (scan=i+1,j=1;j<(IM_DIM-1);j++,scan++)
		{
			res	   =   (im_nhw[scan]<<3) -
						im_nhw[scan-1]-im_nhw[scan+1]-
						im_nhw[scan-(IM_DIM)]-im_nhw[scan+(IM_DIM)]-
						im_nhw[scan-(IM_DIM+1)]-im_nhw[scan+(IM_DIM-1)]-
						im_nhw[scan-(IM_DIM-1)]-im_nhw[scan+(IM_DIM+1)];

			if (abs(res)>count)
			{
				if (res>0)
				{
					if (res>160) im_nhw[scan]+=3;
					else if (res>count) im_nhw[scan]+=2;
				}
				else 
				{
					if (res<-160) im_nhw[scan]-=3;
					else if (res<-count) im_nhw[scan]-=2;
				}
			}
		}
	}

	for (i=0;i<IM_SIZE;i++) 
	{
		if ((im_nhw[i]>>8)!=0) 
		{
			if (im_nhw[i]<0) im_nhw[i]=0;
			else if (im_nhw[i]>255) im_nhw[i]=255;
		}
	}

	im->scale=(unsigned char*)malloc(2*IM_SIZE*sizeof(char));
	nhw_scale=(unsigned char*)im->scale;

	/*for (j=0;j<IM_DIM;j++)
	{
		for (i=0;i<(IM_SIZE-IM_DIM);i+=(IM_DIM))
		{
			im->scale[j+(2*i)]=im->im_process[j+i];
			im->scale[j+(2*i)+IM_DIM]=((im->im_process[j+i]+im->im_process[j+i+IM_DIM]+1)>>1);
		}

		im->scale[j+(2*IM_SIZE-2*IM_DIM)]=im->im_process[j+(IM_SIZE-IM_DIM)];
		im->scale[j+(2*IM_SIZE-IM_DIM)]=im->im_process[j+(IM_SIZE-IM_DIM)];

	}*/
	// faster version
	for (j=0;j<IM_DIM;j++)
	{
		stage=j;count=j;
		for (;count<(IM_SIZE-IM_DIM);count+=IM_DIM,stage+=(2*IM_DIM)) 
		{
			nhw_scale[stage]=(im_nhw[count]);
			nhw_scale[stage+IM_DIM]=((im_nhw[count]+im_nhw[count+IM_DIM]+1)>>1);
		}

		nhw_scale[j+(2*IM_SIZE-2*IM_DIM)]=im_nhw[j+(IM_SIZE-IM_DIM)];
		nhw_scale[j+(2*IM_SIZE-IM_DIM)]=im_nhw[j+(IM_SIZE-IM_DIM)];

	}

	free(im->im_process);
	im->im_bufferV=(unsigned char*)malloc(4*IM_SIZE*sizeof(char));
	nhw_chr=(unsigned char*)im->im_bufferV;

	/*for (i=0,e=0;i<(4*IM_SIZE);i+=(IM_DIM*2),e+=IM_DIM)
	{
		for (j=0;j<IM_DIM-1;j++)
		{
			im->im_bufferU[(j*2)+i]=im->scale[j+e];
			im->im_bufferU[(j*2)+1+i]=(im->scale[j+e]+im->scale[j+e+1]+1)>>1;
		}

		im->im_bufferU[(IM_DIM*2)-2+i]=im->scale[IM_DIM-1+e];
		im->im_bufferU[(IM_DIM*2)-1+i]=im->scale[IM_DIM-1+e];
	}*/

	//faster version
	for (count=0,stage=0;count<(4*IM_SIZE);count+=(IM_DIM*2),stage+=IM_DIM)
	{
		for (j=0;j<IM_DIM-1;j++,stage++) 
		{
			nhw_chr[count++]=nhw_scale[stage];
			nhw_chr[count++]=(nhw_scale[stage]+nhw_scale[stage+1]+1)>>1;
		}

		im->im_bufferV[count++]=nhw_scale[stage];
		im->im_bufferV[count]=nhw_scale[stage];

		count-=(2*IM_DIM-1);
		stage-=(IM_DIM-1);
	}

	free(im->scale);
}


void decode_image(image_buffer *im,decode_state *os, int bypass_compression)
{
 	int nhw,stage,i,j,count,scan,exw1,res;
	short *im_nhw,*pr;
	unsigned char *nhw_scale;
	NhwIndex *nhwresH1 = 0,*nhwresH2 = 0;
	NhwIndex *nhwres1 = 0,*nhwres2 = 0;
	NhwIndex *nhwres3 = 0,*nhwres4 = 0,*nhwres5 = 0,*nhwres6 = 0;

	int IM_SIZE = im->fmt.end / 4;
	int IM_DIM = im->fmt.tile_size / 2;

	im->im_jpeg=(short*)malloc(4*IM_SIZE*sizeof(short));
	im_nhw=(short*)im->im_jpeg;
	pr=(short*)im->im_process;

	if (bypass_compression) {
		unsigned char *nhwp = im->im_nhw;
		for (i = 0; i < im->fmt.end; i++) {
			*pr++ = reverse_offset_correction_coding(*nhwp++);
		}
		pr=(short*)im->im_process;
	} else {
		retrieve_pixel_Y_comp(im,os,4*IM_SIZE,os->packet1,pr);
	}


	// Y
	for (j=0,count=0;j<im->fmt.tile_size;)
	{
		for (i=0;i<IM_DIM;i++)
		{
			im_nhw[j]=pr[count];
			im_nhw[j+1]=pr[count+1];
			im_nhw[j+2]=pr[count+2];
			im_nhw[j+3]=pr[count+3];
	
			j+=im->fmt.tile_size;
			im_nhw[j+3]=pr[count+4];
			im_nhw[j+2]=pr[count+5];
			im_nhw[j+1]=pr[count+6];
			im_nhw[j]=pr[count+7];

			j+=im->fmt.tile_size;
			count+=8;
		}

		j-=(im->fmt.end-4);
	}

	if (os->res1.bit_len > 0) {
		assert(os->end_ch_res >= os->res1.bit_len-1);
		nhwres1=(NhwIndex *)malloc(os->end_ch_res*sizeof(NhwIndex));
		// assert(os->d_size_tree1 >= os->res1.bit_len-1);
		nhwres2=(NhwIndex *)malloc(os->res1.bit_len * 8 *sizeof(NhwIndex));

		decode_res1(os, nhwres1, nhwres2);
	}

////////////////////////////////////////////////////////////////////////////////////////////////

	if (os->res5.bit_len > 0) {
		// FIXME: No malloc here
		// nhwresH1=(NhwIndex *)malloc(os->end_ch_res*sizeof(NhwIndex));
		// nhwresH2=(NhwIndex *)malloc(os->d_size_tree1*sizeof(NhwIndex));
		nhwresH1=(NhwIndex *)malloc(os->res5.bit_len * 8 *sizeof(NhwIndex));
		nhwresH2=(NhwIndex *)malloc(os->res5.bit_len * 8 *sizeof(NhwIndex));
		decode_res5(os, nhwresH1, nhwresH2);
		// XXX
		free(os->res5.res);
		free(os->res5.res_bit);
		free(os->res5.res_word);
	}

////////////////////////////////////////////////////////////////////////////////////////////////////////////

	if (os->nhw_res6_len > 0 && os->nhw_res6_bit_len > 0) {
		decode_res6(im, os);

		// XXX Not pretty
		free(os->nhw_res6);
		free(os->nhw_res6_bit);
		free(os->nhw_res6_word);
	}

///////////////////////////////////////////////////////////////////////////////////////////////////////////

	if (os->res3.bit_len > 0)
	{
		int n = os->res3.bit_len<<3;

		nhwres3=(NhwIndex *)malloc(n*sizeof(NhwIndex));
		nhwres4=(NhwIndex *)malloc(n*sizeof(NhwIndex));
		nhwres5=(NhwIndex *)malloc(n*sizeof(NhwIndex));
		nhwres6=(NhwIndex *)malloc(n*sizeof(NhwIndex));

		decode_res3(os, nhwres3, nhwres4, nhwres5, nhwres6);
	}

	// PROCESS code values
	// CODE_NHW_NB_1, ..
	process_nhwcodes(im, im_nhw, IM_SIZE, IM_DIM);

	// START processing residual compensation:

	nhw=0;
	for (i=0;i<(4*IM_SIZE>>2);i+=(2*IM_DIM))
	{
		for (scan=i,j=0;j<(IM_DIM>>1);j++) 
		{
			im->im_jpeg[scan++]=os->res_comp[nhw++];
		}
	}


	if (os->nhw_res4_len>0)
	{
		decode_res4(im, os);
		free(os->nhw_res4);
	}

	exw1 = decode_exwY(im, os);
	for (i=(2*IM_DIM);i<((2*IM_SIZE)-(2*IM_DIM));i+=(2*IM_DIM))
	{
		for (scan=i+1,j=1;j<IM_DIM-1;j++,scan++) 
		{
			if (abs(im_nhw[scan])>8)
			{
				if (abs(im_nhw[scan-(2*IM_DIM+1)])>8) continue;
				if (abs(im_nhw[scan-(2*IM_DIM)])>8) continue;
				if (abs(im_nhw[scan-(2*IM_DIM-1)])>8) continue;
				if (abs(im_nhw[scan-1])>8) continue;
				if (abs(im_nhw[scan+1])>8) continue;
				if (abs(im_nhw[scan+(2*IM_DIM-1)])>8) continue;
				if (abs(im_nhw[scan+(2*IM_DIM)])>8) continue;
				if (abs(im_nhw[scan+(2*IM_DIM+1)])>8) continue;
			
				if (i>=IM_SIZE || j>=(IM_DIM>>1))
				{
					if (im_nhw[scan]>0) im_nhw[scan]--;
					else im_nhw[scan]++;
				}
			}
		}
	}

	im->setup->wavelet_type=WVLTS_53;
	// wavelet_order=im->setup->wvlts_order;
	//for (stage=wavelet_order-1;stage>=0;stage--) dec_wavelet_synthesis(im,(2*IM_DIM)>>stage,end_transform++,1);

	// First reverse synthesis for low res:
	// dec_wavelet_synthesis(im,(2*IM_DIM)>>1, 0 /* first stage */ ,1);
	wl_synth_luma(im, IM_DIM, 0);

	// Modifies process:

	// This function frees the possibly used nhwres arrays.
	// Not pretty...
	residual_compensation(im, os, nhwresH1, nhwresH2,
		nhwres1, nhwres2, nhwres3, nhwres4, nhwres5, nhwres6);

	for (i=(2*IM_DIM),stage=0;i<((2*IM_SIZE)-(2*IM_DIM));i+=(2*IM_DIM))
	{
		for (scan=i+1,j=1;j<(IM_DIM-2);j++,scan++)
		{
			res	   =   (pr[scan]<<3) -
						pr[scan-1]-pr[scan+1]-
						pr[scan-(2*IM_DIM)]-pr[scan+(2*IM_DIM)]-
						pr[scan-(2*IM_DIM+1)]-pr[scan+(2*IM_DIM-1)]-
						pr[scan-(2*IM_DIM-1)]-pr[scan+(2*IM_DIM+1)];

			j++;scan++;

			count   =  (pr[scan]<<3) -
						pr[scan-1]-pr[scan+1]-
						pr[scan-(2*IM_DIM)]-pr[scan+(2*IM_DIM)]-
						pr[scan-(2*IM_DIM+1)]-pr[scan+(2*IM_DIM-1)]-
						pr[scan-(2*IM_DIM-1)]-pr[scan+(2*IM_DIM+1)];


			if (res>41 && res<108 && count<16)
			{
				pr[scan-1]+=16000;stage++;
			}
			else if (res<-41 && res>-108 && count>-16)
			{
				pr[scan-1]+=16000;stage++;
			}
			else if (count>41 && count<108 && res<16)
			{
				pr[scan]+=16000;stage++;
			}
			else if (count<-41 && count>-108 && res>-16)
			{
				pr[scan]+=16000;stage++;
			}
		}
	}

	// Index list for values to be offset corrected:
	NhwIndex *offset_list;
	offset_list = (NhwIndex *) malloc(stage*sizeof(NhwIndex));

	int step = im->fmt.tile_size;
	for (i = step,count = 0;i < im->fmt.half-step; i += step)
	{
		for (scan=i,j=0;j<IM_DIM;j++,scan++)
		{
			unsigned long tmp = PACK_COORDINATES(j, i);
			if (pr[scan]>10000)
			{
				offset_list[count++]= tmp;
				// printf("scan: %d  0x%x\n", PACK_COORDINATES(j, i),
					// PACK_COORDINATES(j, i));

				pr[scan]-=16000;
			}
		}
	}

	for (i=0;i<IM_DIM;i++,im_nhw+=(2*IM_DIM))
	{
		for (scan=i,j=0;j<IM_DIM;j++,scan+=(2*IM_DIM)) im_nhw[j]=pr[scan];
	}

	// Second synthesis for high res:
	dec_wavelet_synthesis2(im,os,(2*IM_DIM), 1 /* Second */,1);

	im_nhw=(short*)im->im_jpeg;

	for (i=0;i<count;i++)
	{
		scan = UNPACK_COORDINATES(offset_list[i]);

		assert(scan < im->fmt.end);
		
		if(scan-(2*IM_DIM) < 0) {
			printf("At %d: scan %d\n", i, scan);
		}
		assert(scan-(2*IM_DIM) >= 0);

		res	   =   (im_nhw[scan]<<3) -
					im_nhw[scan-1]-im_nhw[scan+1]-
					im_nhw[scan-(2*IM_DIM)]-im_nhw[scan+(2*IM_DIM)]-
					im_nhw[scan-(2*IM_DIM+1)]-im_nhw[scan+(2*IM_DIM-1)]-
					im_nhw[scan-(2*IM_DIM-1)]-im_nhw[scan+(2*IM_DIM+1)];

		if (abs(res)<116)
		{
			im_nhw[scan]=((im_nhw[scan]<<2)
							+im_nhw[scan-1]+im_nhw[scan+1]+im_nhw[scan-(2*IM_DIM)]+im_nhw[scan+(2*IM_DIM)]+4)>>3;
		}
	}

	
	free(offset_list);

	dec_wavelet_synthesis(im,(2*IM_DIM), 1 /* second */ ,3);
	// wl_synth_luma(im, 2*IM_DIM, 1);

	free(im->im_jpeg);

	im_nhw=(short*)im->im_process;

	im->im_bufferY=(unsigned char*)malloc(4*IM_SIZE*sizeof(char));
	nhw_scale=(unsigned char*)im->im_bufferY;

	for (i=0;i<(4*IM_SIZE);i++)
	{
		if ((im_nhw[i]>>8)!=0)
		{
			if (im_nhw[i]<0) nhw_scale[i]=0;
			else if (im_nhw[i]>255) nhw_scale[i]=255;
		}
		else 
		{
			nhw_scale[i]=im_nhw[i];
		}
	}

	free(im->im_process);

	im->im_nhw3=(short*)calloc(2*IM_SIZE,sizeof(short));

	if (bypass_compression) {
//		unsigned char *nhwp = &im->im_nhw[im->fmt.end];
//		pr= im->im_nhw3;
//		for (i = 0; i < im->fmt.end / 2; i++) {
//			*pr++ = *nhwp++;
//		}
	} else {
		retrieve_pixel_UV_comp(im,os,(2*IM_SIZE-1),os->packet2,im->im_nhw3);
	}

	exw1 = decode_u(im, os, exw1);
	decode_v(im, os, exw1);

	free(im->im_nhw3);
}

// Should finally head towards a endian safe and flexible file format
// (IFF approach?)

int parse_file(image_buffer *imd,decode_state *os,char** argv)
{
	FILE *compressed_file;
	int ret;
	int IM_DIM = imd->fmt.tile_size / 2;

	compressed_file=fopen(argv[1],"rb");

	if (compressed_file==NULL)
	{
		printf("\nCould not open file %s", argv[1]);
		exit(-1);
	}

	FREAD(&imd->setup->RES_HIGH,1,1,compressed_file);
	FREAD(&imd->setup->quality_setting,1,1,compressed_file);

	if (imd->setup->RES_HIGH>6)
	{
		printf("\nNot an .nhw file");exit(-1);
	}

	imd->setup->colorspace=YUV;
	imd->setup->wavelet_type=WVLTS_53;
	imd->setup->wvlts_order=2;

	FREAD(&os->d_size_tree1,2,1,compressed_file);
	FREAD(&os->d_size_tree2,2,1,compressed_file);
	FREAD(&os->d_size_data1,4,1,compressed_file);
	FREAD(&os->d_size_data2,4,1,compressed_file);
	FREAD(&os->tree_end,2,1,compressed_file);
	FREAD(&os->exw_Y_end,2,1,compressed_file);
	if (imd->setup->quality_setting>LOW8)
		FREAD(&os->res1.len,2,1,compressed_file);
	else
		os->res1.len = 0;

	if (imd->setup->quality_setting>=LOW1)
	{
		FREAD(&os->res3.len,2,1,compressed_file);
		FREAD(&os->res3.bit_len,2,1,compressed_file);
	} else {
		os->res3.len = 0;
		os->res3.bit_len = 0;
	}
			
	if (imd->setup->quality_setting>LOW3)
	{
		FREAD(&os->nhw_res4_len,2,1,compressed_file);
	} else {
		os->nhw_res4_len = 0;
	}

	if (imd->setup->quality_setting>LOW8)
		FREAD(&os->res1.bit_len,2,1,compressed_file);
	else
		os->res1.bit_len = 0;

	if (imd->setup->quality_setting>=HIGH1)
	{
		FREAD(&os->res5.len,2,1,compressed_file);
		FREAD(&os->res5.bit_len,2,1,compressed_file);

		os->res5.res=(ResIndex*)malloc(os->res5.len*sizeof(ResIndex));
		os->res5.res_bit=(unsigned char*)malloc(os->res5.bit_len*sizeof(char));
		os->res5.res_word=(unsigned char*)malloc(os->res5.bit_len*sizeof(char));
	} else {
		os->res5.len = 0;
		os->res5.bit_len = 0;
	}

	if (imd->setup->quality_setting>HIGH1)
	{
		FREAD(&os->nhw_res6_len,4,1,compressed_file);
		FREAD(&os->nhw_res6_bit_len,2,1,compressed_file);
		FREAD(&os->nhw_char_res1_len,2,1,compressed_file);

		os->nhw_res6=(ResIndex*)malloc(os->nhw_res6_len*sizeof(ResIndex));
		os->nhw_res6_bit=(unsigned char*)malloc(os->nhw_res6_bit_len*sizeof(char));
		os->nhw_res6_word=(unsigned char*)malloc(os->nhw_res6_bit_len*sizeof(char));
		os->nhw_char_res1=(unsigned short*)malloc(os->nhw_char_res1_len*sizeof(short));

		if (imd->setup->quality_setting>HIGH2)
		{
			FREAD(&os->qsetting3_len,2,1,compressed_file);
			os->high_qsetting3=(unsigned int*)malloc(os->qsetting3_len*sizeof(int));
		} else {
			os->qsetting3_len = 0;
		}
	} else {
		os->nhw_res6_len = 0;
		os->nhw_res6_bit_len = 0;
		os->nhw_char_res1_len = 0;
	}

	FREAD(&os->nhw_select1,2,1,compressed_file);
	FREAD(&os->nhw_select2,2,1,compressed_file);
	
	if (imd->setup->quality_setting>LOW5)
	{
		FREAD(&os->highres_comp_len,2,1,compressed_file);
	} else {
		os->highres_comp_len = 0;
	}
	
	FREAD(&os->end_ch_res,2,1,compressed_file);

	os->res_comp=(ResIndex*)malloc((96*IM_DIM+1)*sizeof(ResIndex));
	os->d_tree1=(unsigned char*)calloc(os->d_size_tree1,sizeof(char));
	os->d_tree2=(unsigned char*)calloc(os->d_size_tree2,sizeof(char));
	os->exw_Y=(unsigned char*)malloc(os->exw_Y_end*sizeof(char));
	
	if (os->res1.len > 0 && os->res1.bit_len > 0)
	{
		os->res1.res=(ResIndex*)malloc(os->res1.len*sizeof(ResIndex));
		os->res1.res_bit=(unsigned char*)malloc(os->res1.bit_len*sizeof(char));
		os->res1.res_word=(unsigned char*)malloc(os->res1.bit_len*sizeof(char));
	}

	if (os->res3.len > 0 && os->res3.bit_len > 0)
	{
		os->res3.res=(ResIndex*)malloc(os->res3.len*sizeof(ResIndex));
		os->res3.res_bit=(unsigned char*)malloc(os->res3.bit_len*sizeof(char));
		os->res3.res_word=(unsigned char*)malloc((os->res3.bit_len<<1)*sizeof(char));
	}

	if (os->nhw_res4_len > 0) 
	{
		os->nhw_res4=(ResIndex*)malloc(os->nhw_res4_len*sizeof(ResIndex));
	}

	os->nhw_select_word1=(unsigned char*)malloc(os->nhw_select1*sizeof(char));
	os->nhw_select_word2=(unsigned char*)malloc(os->nhw_select2*sizeof(char));

	if (os->highres_comp_len > 0)
	{
		os->res_U_64=(unsigned char*)malloc((IM_DIM<<1)*sizeof(char));
		os->res_V_64=(unsigned char*)malloc((IM_DIM<<1)*sizeof(char));
		os->highres_comp=(unsigned char*)malloc(os->highres_comp_len*sizeof(char));
	}

	
	os->res_ch=(unsigned char *)malloc(os->end_ch_res*sizeof(char));
	os->packet1=(unsigned int*)malloc(os->d_size_data1*sizeof(int));
	os->packet2=(unsigned int*)malloc((os->d_size_data2-os->d_size_data1)*sizeof(int));

	// COMPRESSED FILE DATA
	FREAD(os->d_tree1,os->d_size_tree1,1,compressed_file);
	FREAD(os->d_tree2,os->d_size_tree2,1,compressed_file);
	FREAD(os->exw_Y,os->exw_Y_end,1,compressed_file);
	
	if (os->res1.len > 0 && os->res1.bit_len > 0)
	{
		FREAD(os->res1.res,os->res1.len,1,compressed_file);
		FREAD(os->res1.res_bit,os->res1.bit_len,1,compressed_file);
		FREAD(os->res1.res_word,os->res1.bit_len,1,compressed_file);
	}

	if (os->nhw_res4_len > 0)
	{
		FREAD(os->nhw_res4,os->nhw_res4_len,1,compressed_file);
	}

	if (os->res3.len > 0 && os->res3.bit_len > 0)
	{
		FREAD(os->res3.res,os->res3.len,1,compressed_file);
		FREAD(os->res3.res_bit,os->res3.bit_len,1,compressed_file);
		FREAD(os->res3.res_word,(os->res3.bit_len<<1),1,compressed_file);
	}

	if (os->res5.len > 0 && os->res5.bit_len > 0)
	{
		FREAD(os->res5.res,os->res5.len,1,compressed_file);
		FREAD(os->res5.res_bit,os->res5.bit_len,1,compressed_file);
		FREAD(os->res5.res_word,os->res5.bit_len,1,compressed_file);
	}

	if (os->nhw_res6_len > 0 && os->nhw_res6_bit_len > 0
	 && os->nhw_char_res1_len > 0)
	{
		FREAD(os->nhw_res6,os->nhw_res6_len,1,compressed_file);
		FREAD(os->nhw_res6_bit,os->nhw_res6_bit_len,1,compressed_file);
		FREAD(os->nhw_res6_word,os->nhw_res6_bit_len,1,compressed_file);
		FREAD(os->nhw_char_res1,os->nhw_char_res1_len,2,compressed_file);

		if (os->qsetting3_len > 0 )
		{
			FREAD(os->high_qsetting3,os->qsetting3_len,4,compressed_file);
		}
	}

	FREAD(os->nhw_select_word1,os->nhw_select1,1,compressed_file);
	FREAD(os->nhw_select_word2,os->nhw_select2,1,compressed_file);

	if (os->highres_comp_len > 0)
	{
		FREAD(os->res_U_64,(IM_DIM<<1),1,compressed_file);
		FREAD(os->res_V_64,(IM_DIM<<1),1,compressed_file);
		FREAD(os->highres_comp,os->highres_comp_len,1,compressed_file);
	}

	FREAD(os->res_ch,os->end_ch_res,1,compressed_file);
	FREAD(os->packet1,os->d_size_data1*4,1,compressed_file);
	FREAD(os->packet2,(os->d_size_data2-os->d_size_data1)*4,1,compressed_file);

	fclose(compressed_file);
	return 0;
}
