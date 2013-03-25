/***************************************************************************
****************************************************************************
*  NHW Image Codec 													       *
*  file: wavelet_filterbank.c  										       *
*  version: 0.1.3 						     		     				   *
*  last update: $ 06012012 nhw exp $							           *
*																		   *
****************************************************************************
****************************************************************************

****************************************************************************
*  remark: -analysis and synthesis filterbanks							   *
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

#include "codec.h"

void wavelet_analysis(image_buffer *im,int norder,int last_stage,int Y)
{
	int i,j,a;
	short *data,*res,*res2;

	for (i=0;i<(norder*IM_DIM);i+=(2*IM_DIM))
	{
		for (a=i,j=0;j<(norder>>1);j++,a++) im->im_process[a]=0; 
	}

	if (Y)
	{

	data=im->im_jpeg;
	res=im->im_process;
	res2=im->im_process + norder/2;

	if (!last_stage)
	{
		for (i=0;i<norder;i++)
		{
			downfilter53IV(data,norder,0,res);downfilter53IV(data,norder,1,res2);
			data +=(2*IM_DIM);res +=(2*IM_DIM);res2 +=(2*IM_DIM);
		}
	}
	/*else if (last_stage>1)
	{
		for (i=0;i<norder;i++)
		{
			downfilter53IV(data,norder,0,res);downfilter53IV(data,norder,1,res2);
			data +=(2*IM_DIM);res +=(2*IM_DIM);res2 +=(2*IM_DIM);
		}
	}*/
	else
	{

		for (i=0;i<norder;i++)
		{
			downfilter53IV(data,norder,0,res);downfilter53IV(data,norder,1,res2);
			data +=(2*IM_DIM);res +=(2*IM_DIM);res2 +=(2*IM_DIM);
		}
	}

	//image transposition
	/*for (i=0;i<norder;i++)
	{
		for (j=0;j<norder;j++) im->im_jpeg[i*(2*IM_DIM)+j]=im->im_process[i+j*(2*IM_DIM)];
	}*/
	res=im->im_jpeg;data=im->im_process;
	for (i=0;i<norder;i++,res+=(2*IM_DIM))
	{
		a=i;
		for (j=0;j<norder;j++,a+=(2*IM_DIM)) res[j]=data[a];
	}

	data=im->im_jpeg;
	res=im->im_process;
	res2=im->im_process + norder/2;

	if (!im->setup->RES_HIGH)
	{
		for (i=0;i<norder/2;i++)
		{
			downfilter53VI(data,norder,0,res);downfilter53VI(data,norder,1,res2);
			data +=(2*IM_DIM);res +=(2*IM_DIM);res2 +=(2*IM_DIM);
		}
	}
	/*else if (last_stage>1)
	{
		for (i=0;i<norder/2;i++)
		{
			downfilter53IV(data,norder,0,res);downfilter53IV(data,norder,1,res2);
			data +=(2*IM_DIM);res +=(2*IM_DIM);res2 +=(2*IM_DIM);
		}
	}*/
	else
	{
		for (i=0;i<norder/2;i++)
		{
			downfilter53II(data,norder,0,res);downfilter53II(data,norder,1,res2);
			data +=(2*IM_DIM);res +=(2*IM_DIM);res2 +=(2*IM_DIM);
		}
	}

	data=im->im_jpeg + norder*((2*IM_DIM)>>1);
	res=im->im_process + norder*((2*IM_DIM)>>1);
	res2=im->im_process + norder*((2*IM_DIM)>>1) + norder/2;

	if (!last_stage)
	{
		for (i=norder/2;i<norder;i++)
		{
			downfilter53(data,norder,0,res);downfilter53(data,norder,1,res2);
			data +=(2*IM_DIM);res +=(2*IM_DIM);res2 +=(2*IM_DIM);
		}
	}
	/*else if (last_stage>1)
	{
		for (i=norder/2;i<norder;i++)
		{
			downfilter53IV(data,norder,0,res);downfilter53IV(data,norder,1,res2);
			data +=(2*IM_DIM);res +=(2*IM_DIM);res2 +=(2*IM_DIM);
		}
	}*/
	else 
	{
		for (i=norder/2;i<norder;i++)
		{
			downfilter53(data,norder,0,res);downfilter53(data,norder,1,res2);
			data +=(2*IM_DIM);res +=(2*IM_DIM);res2 +=(2*IM_DIM);
		}
	}

	if (last_stage!=im->setup->wvlts_order-1)
	{// image transposition
		/*for (i=0;i<norder/2;i++)
		{
			for (j=0;j<norder/2;j++) im->im_jpeg[i*(2*IM_DIM)+j]=im->im_process[i+j*(2*IM_DIM)];
		}*/
		res=im->im_jpeg;data=im->im_process;
		for (i=0;i<(norder>>1);i++,res+=(2*IM_DIM))
		{
			a=i;
			for (j=0;j<(norder>>1);j++,a+=(2*IM_DIM)) res[j]=data[a];
		}
	}
	}
	else
	{
	data=im->im_jpeg;
	res=im->im_process;
	res2=im->im_process + norder/2;

	if (!last_stage)
	{
		for (i=0;i<norder;i++)
		{
			downfilter53IV(data,norder,0,res);downfilter53IV(data,norder,1,res2);
			data +=IM_DIM;res +=IM_DIM;res2 +=IM_DIM;
		}
	}
	/*else if (last_stage>1)
	{
		for (i=0;i<norder;i++)
		{
			downfilter53IV(data,norder,0,res);downfilter53IV(data,norder,1,res2);
			data +=IM_DIM;res +=IM_DIM;res2 +=IM_DIM;
		}
	}*/
	else
	{
		for (i=0;i<norder;i++)
		{
			downfilter53IV(data,norder,0,res);downfilter53IV(data,norder,1,res2);
			data +=IM_DIM;res +=IM_DIM;res2 +=IM_DIM;
		}
	}

	//image transposition
	/*for (i=0;i<norder;i++)
	{
		for (j=0;j<norder;j++) im->im_jpeg[i*(IM_DIM)+j]=im->im_process[i+j*(IM_DIM)];
	}*/
	res=im->im_jpeg;data=im->im_process;
	for (i=0;i<norder;i++,res+=(IM_DIM))
	{
		a=i;
		for (j=0;j<norder;j++,a+=(IM_DIM)) res[j]=data[a];
	}

	data=im->im_jpeg;
	res=im->im_process;
	res2=im->im_process + norder/2;

	if (!im->setup->RES_HIGH)
	{
		for (i=0;i<norder/2;i++)
		{
			downfilter53VI(data,norder,0,res);downfilter53VI(data,norder,1,res2);
			data +=IM_DIM;res +=IM_DIM;res2 +=IM_DIM;
		}
	}
	/*else if (last_stage>1)
	{
		for (i=0;i<norder/2;i++)
		{
			downfilter53IV(data,norder,0,res);downfilter53IV(data,norder,1,res2);
			data +=IM_DIM;res +=IM_DIM;res2 +=IM_DIM;
		}
	}*/
	else
	{
		for (i=0;i<norder/2;i++)
		{
			downfilter53II(data,norder,0,res);downfilter53II(data,norder,1,res2);
			data +=IM_DIM;res +=IM_DIM;res2 +=IM_DIM;
		}
	}

	data=im->im_jpeg + norder*(IM_DIM>>1);
	res=im->im_process + norder*(IM_DIM>>1);
	res2=im->im_process + norder*(IM_DIM>>1) + norder/2;

	if (!last_stage)
	{
		for (i=norder/2;i<norder;i++)
		{
			downfilter53(data,norder,0,res);downfilter53(data,norder,1,res2);
			data +=IM_DIM;res +=IM_DIM;res2 +=IM_DIM;
		}
	}
	/*else if (last_stage>1)
	{
		for (i=norder/2;i<norder;i++)
		{
			downfilter53IV(data,norder,0,res);downfilter53IV(data,norder,1,res2);
			data +=IM_DIM;res +=IM_DIM;res2 +=IM_DIM;
		}
	}*/
	else 
	{
		for (i=norder/2;i<norder;i++)
		{
			downfilter53(data,norder,0,res);downfilter53(data,norder,1,res2);
			data +=IM_DIM;res +=IM_DIM;res2 +=IM_DIM;
		}
	}

	if (last_stage!=im->setup->wvlts_order-1)
	{// image transposition
		/*for (i=0;i<norder/2;i++)
		{
			for (j=0;j<norder/2;j++) im->im_jpeg[i*(IM_DIM)+j]=im->im_process[i+j*(IM_DIM)];
		}*/
		res=im->im_jpeg;data=im->im_process;
		for (i=0;i<(norder>>1);i++,res+=(IM_DIM))
		{
			a=i;
			for (j=0;j<(norder>>1);j++,a+=(IM_DIM)) res[j]=data[a];
		}
	}

	}
}


void wavelet_synthesis(image_buffer *im,int norder,int last_stage,int Y)
{
	short *data,*res,*data2;
	int i,j,IM_SYNTH=IM_DIM,a;

	if (Y)
	{
	data=im->im_jpeg;
	res=im->im_process;
	data2=im->im_jpeg + norder/2;

	if (im->setup->wavelet_type==WVLTS_97)
	{
		for (i=0;i<norder/2;i++)
		{
			upfilter97(data,norder/2,1,res);upfilter97(data2,norder/2,0,res);	
			data +=(2*IM_SYNTH);res +=(2*IM_SYNTH);data2 +=(2*IM_SYNTH);
		}
	}
	else
	{
		for (i=0;i<norder/2;i++)
		{
			upfilter53I(data,norder/2,res);upfilter53III(data2,norder/2,res);	
			data +=(2*IM_SYNTH);res +=(2*IM_SYNTH);data2 +=(2*IM_SYNTH);
		}
	}

	data=im->im_jpeg + norder*((2*IM_SYNTH)>>1);
	res=im->im_process + norder*((2*IM_SYNTH)>>1);
	data2=im->im_jpeg + norder*((2*IM_SYNTH)>>1) + norder/2;

	if (im->setup->wavelet_type==WVLTS_97)
	{
		for (i=norder/2;i<norder;i++)
		{
			upfilter97(data,norder/2,1,res);upfilter97(data2,norder/2,0,res);
			data +=(2*IM_SYNTH);res +=(2*IM_SYNTH);data2 +=(2*IM_SYNTH);
		}
	}
	else
	{
		for (i=norder/2;i<norder;i++)
		{
			upfilter53I(data,norder/2,res);upfilter53III(data2,norder/2,res);
			data +=(2*IM_SYNTH);res +=(2*IM_SYNTH);data2 +=(2*IM_SYNTH);
		}
	}

	//image transposition
	/*for (i=0;i<norder;i++)
	{
		for (j=0;j<norder;j++) im->im_jpeg[i*(2*IM_SYNTH)+j]=im->im_process[i+j*(2*IM_SYNTH)];
	}*/
	//faster version
	res=im->im_jpeg;data=im->im_process;
	for (i=0;i<norder;i++,res+=(2*IM_DIM))
	{
		a=i;
		for (j=0;j<norder;j++,a+=(2*IM_DIM)) res[j]=data[a];
	}

	data=im->im_jpeg;
	res=im->im_process;
	data2=im->im_jpeg + norder/2;

	if (im->setup->wavelet_type==WVLTS_97)
	{
		for (i=0;i<norder;i++)
		{
			upfilter97(data,norder/2,1,res);upfilter97(data2,norder/2,0,res);
			data +=(2*IM_SYNTH);res +=(2*IM_SYNTH);data2 +=(2*IM_SYNTH);
		}
	}
	else
	{
		for (i=0;i<norder;i++)
		{
			upfilter53I(data,norder/2,res);upfilter53VI(data2,norder/2,res);
			data +=(2*IM_SYNTH);res +=(2*IM_SYNTH);data2 +=(2*IM_SYNTH);
		}
	}

	if (last_stage!=im->setup->wvlts_order-1)
	{//image transposition
		/*for (i=0;i<norder;i++)
		{
			for (j=0;j<norder;j++) im->im_jpeg[i*(2*IM_SYNTH)+j]=im->im_process[i+j*(2*IM_SYNTH)];
		}*/
		//faster version
		res=im->im_jpeg;data=im->im_process;
		for (i=0;i<norder;i++,res+=(2*IM_DIM))
		{
			a=i;
			for (j=0;j<norder;j++,a+=(2*IM_DIM)) res[j]=data[a];
		}
	}
	}
	else 
	{
	data=im->im_jpeg;
	res=im->im_process;
	data2=im->im_jpeg + norder/2;

	if (im->setup->wavelet_type==WVLTS_97)
	{
		for (i=0;i<norder/2;i++)
		{
			upfilter97(data,norder/2,1,res);upfilter97(data2,norder/2,0,res);	
			data +=(IM_SYNTH);res +=(IM_SYNTH);data2 +=(IM_SYNTH);
		}
	}
	else
	{
		for (i=0;i<norder/2;i++)
		{
			upfilter53I(data,norder/2,res);upfilter53III(data2,norder/2,res);	
			data +=IM_SYNTH;res +=IM_SYNTH;data2 +=IM_SYNTH;
		}
	}

	data=im->im_jpeg + norder*(IM_SYNTH>>1);
	res=im->im_process + norder*(IM_SYNTH>>1);
	data2=im->im_jpeg + norder*(IM_SYNTH>>1) + norder/2;

	if (im->setup->wavelet_type==WVLTS_97)
	{
		for (i=norder/2;i<norder;i++)
		{
			upfilter97(data,norder/2,1,res);upfilter97(data2,norder/2,0,res);
			data +=IM_SYNTH;res +=IM_SYNTH;data2 +=IM_SYNTH;
		}
	}
	else
	{
		for (i=norder/2;i<norder;i++)
		{
			upfilter53I(data,norder/2,res);upfilter53III(data2,norder/2,res);
			data +=IM_SYNTH;res +=IM_SYNTH;data2 +=IM_SYNTH;
		}
	}

	//image transposition
	/*for (i=0;i<norder;i++)
	{
		for (j=0;j<norder;j++) im->im_jpeg[i*(IM_SYNTH)+j]=im->im_process[i+j*(IM_SYNTH)];
	}*/
	//faster version
	res=im->im_jpeg;data=im->im_process;
	for (i=0;i<norder;i++,res+=(IM_DIM))
	{
		a=i;
		for (j=0;j<norder;j++,a+=(IM_DIM)) res[j]=data[a];
	}

	data=im->im_jpeg;
	res=im->im_process;
	data2=im->im_jpeg + norder/2;

	if (im->setup->wavelet_type==WVLTS_97)
	{
		for (i=0;i<norder;i++)
		{
			upfilter97(data,norder/2,1,res);upfilter97(data2,norder/2,0,res);
			data +=IM_SYNTH;res +=IM_SYNTH;data2 +=IM_SYNTH;
		}
	}
	else
	{
		for (i=0;i<norder;i++)
		{
			upfilter53I(data,norder/2,res);upfilter53VI(data2,norder/2,res);
			data +=IM_SYNTH;res +=IM_SYNTH;data2 +=IM_SYNTH;
		}
	}

	if (last_stage!=im->setup->wvlts_order-1)
	{//image transposition
		/*for (i=0;i<norder;i++)
		{
			for (j=0;j<norder;j++) im->im_jpeg[i*(IM_SYNTH)+j]=im->im_process[i+j*(IM_SYNTH)];
		}*/
		//faster version
		res=im->im_jpeg;data=im->im_process;
		for (i=0;i<norder;i++,res+=(IM_DIM))
		{
			a=i;
			for (j=0;j<norder;j++,a+=(IM_DIM)) res[j]=data[a];
		}
	}
	}
}