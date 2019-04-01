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

void wavelet_synthesis(image_buffer *im,int norder,int last_stage,int Y)
{
	short *data,*res,*data2;
	int i,j,n=im->fmt.tile_size,a;

	int halfn = n / 2;

	if (Y==1)
	{
	data=im->im_jpeg;
	res=im->im_process;
	data2=im->im_jpeg + norder/2;

	if (last_stage==0)
	{
		for (i=0;i<norder/2;i++)
		{
			//upfilter97(data,norder/2,1,res);upfilter97(data2,norder/2,0,res);	
			upfilter53I(data,norder/2,res);upfilter53III(data2,norder/2,res);	
			data +=n;res +=n;data2 +=n;
		}
	}
	else if (last_stage==1)
	{
		for (i=0;i<norder/2;i++)
		{
			//upfilter97(data,norder/2,1,res);upfilter97(data2,norder/2,0,res);	
			upfilter53I(data,norder/2,res);upfilter53III(data2,norder/2,res);	
			data +=n;res +=n;data2 +=n;
		}
	}

	data=im->im_jpeg + norder*(halfn);
	res=im->im_process + norder*(halfn);
	data2=im->im_jpeg + norder*(halfn) + norder/2;

	if (last_stage==1)
	{
		for (i=norder/2;i<norder;i++)
		{
			//upfilter97(data,norder/2,1,res);upfilter97(data2,norder/2,0,res);	
			upfilter53I(data,norder/2,res);upfilter53III(data2,norder/2,res);
			data +=n;res +=n;data2 +=n;
		}
	}
	else if (last_stage==0)
	{
		for (i=norder/2;i<norder;i++)
		{
			//upfilter97(data,norder/2,1,res);upfilter97(data2,norder/2,0,res);
			upfilter53I(data,norder/2,res);upfilter53III(data2,norder/2,res);
			data +=n;res +=n;data2 +=n;
		}
	}

	//image transposition
	/*for (i=0;i<norder;i++)
	{
		for (j=0;j<norder;j++) im->im_jpeg[i*n+j]=im->im_process[i+j*n];
	}*/

	//faster version
	res=im->im_jpeg;data=im->im_process;
	for (i=0;i<norder;i++,res+=im->fmt.tile_size)
	{
		a=i;
		for (j=0;j<norder;j++,a+=im->fmt.tile_size) res[j]=data[a];
	}
	}
	
	if (Y==1 || Y==3)
	{
	data=im->im_jpeg;
	res=im->im_process;
	data2=im->im_jpeg + norder/2;

	if (last_stage==0)
	{
		for (i=0;i<norder;i++)
		{
			upfilter53I(data,norder/2,res);upfilter53VI(data2,norder/2,res);	
			//upfilter97(data,norder/2,1,res);upfilter97(data2,norder/2,0,res);
			data +=n;res +=n;data2 +=n;
		}
	}
	else
	{
		for (i=0;i<norder;i++)
		{
			//upfilter97(data,norder/2,1,res);upfilter97(data2,norder/2,0,res);	
			upfilter53I(data,norder/2,res);upfilter53VI(data2,norder/2,res);
			data +=n;res +=n;data2 +=n;
		}
	}
	}
	else if (!Y)
	{
	data=im->im_jpeg;
	res=im->im_process;
	data2=im->im_jpeg + norder/2;

	if (last_stage==0)
	{
		for (i=0;i<norder/2;i++)
		{
			upfilter53I(data,norder/2,res);upfilter53III(data2,norder/2,res);	
			data +=halfn;res +=halfn;data2 +=halfn;
		}
	}
	else
	{
		for (i=0;i<norder/2;i++)
		{
			upfilter53I(data,norder/2,res);upfilter53III(data2,norder/2,res);	
			data +=halfn;res +=halfn;data2 +=halfn;
		}
	}

	data=im->im_jpeg + norder*(halfn>>1);
	res=im->im_process + norder*(halfn>>1);
	data2=im->im_jpeg + norder*(halfn>>1) + norder/2;

	if (last_stage==0)
	{
		for (i=norder/2;i<norder;i++)
		{
			upfilter53I(data,norder/2,res);upfilter53III(data2,norder/2,res);
			data +=halfn;res +=halfn;data2 +=halfn;
		}
	}
	else
	{
		for (i=norder/2;i<norder;i++)
		{
			upfilter53I(data,norder/2,res);upfilter53III(data2,norder/2,res);
			data +=halfn;res +=halfn;data2 +=halfn;
		}
	}

	//image transposition
	/*for (i=0;i<norder;i++)
	{
		for (j=0;j<norder;j++) im->im_jpeg[i*halfn+j]=im->im_process[i+j*halfn];
	}*/
	//faster version
	res=im->im_jpeg;data=im->im_process;
	for (i=0;i<norder;i++,res+=halfn)
	{
		a=i;
		for (j=0;j<norder;j++,a+=halfn) res[j]=data[a];
	}

	data=im->im_jpeg;
	res=im->im_process;
	data2=im->im_jpeg + norder/2;

	if (last_stage==1)
	{
		for (i=0;i<norder;i++)
		{
			upfilter53I(data,norder/2,res);upfilter53VI(data2,norder/2,res);	
			//upfilter53(data,norder/2,res);upfilter53IV(data2,norder/2,res);
			data +=halfn;res +=halfn;data2 +=halfn;
		}
	}
	else
	{
		for (i=0;i<norder;i++)
		{
			upfilter53I(data,norder/2,res);upfilter53VI(data2,norder/2,res);
			data +=halfn;res +=halfn;data2 +=halfn;
		}
	}

	/*if (last_stage!=im->setup->wvlts_order-1)
	{//image transposition
		//faster version
		res=im->im_jpeg;data=im->im_process;
		for (i=0;i<norder;i++,res+=halfn)
		{
			a=i;
			for (j=0;j<norder;j++,a+=halfn) res[j]=data[a];
		}
	}*/
	}
}

void wavelet_synthesis2(image_buffer *im,decode_state *os,int norder,int last_stage,int Y)
{
	short *data,*res,*data2;
	int i,j,a;

	int n, halfn;

	n = im->fmt.tile_size;
	halfn = n / 2;

	data=im->im_jpeg;
	res=im->im_process;
	data2=im->im_jpeg + norder/2;

	if (last_stage==0)
	{
		for (i=0;i<norder/2;i++)
		{
			//upfilter97(data,norder/2,1,res);upfilter97(data2,norder/2,0,res);	
			upfilter53I(data,norder/2,res);upfilter53III(data2,norder/2,res);	
			data +=n;res +=n;data2 +=n;
		}
	}
	else if (last_stage==1)
	{
		for (i=0;i<norder/2;i++)
		{
			//upfilter97(data,norder/2,1,res);upfilter97(data2,norder/2,0,res);	
			upfilter53I(data,norder/2,res);upfilter53III(data2,norder/2,res);	
			data +=n;res +=n;data2 +=n;
		}
	}

	data=im->im_jpeg + norder*halfn;
	res=im->im_process + norder*halfn;
	data2=im->im_jpeg + norder*halfn + norder/2;

	if (last_stage==1)
	{
		for (i=norder/2;i<norder;i++)
		{
			//upfilter97(data,norder/2,1,res);upfilter97(data2,norder/2,0,res);	
			upfilter53I(data,norder/2,res);upfilter53III(data2,norder/2,res);
			data +=n;res +=n;data2 +=n;
		}
	}
	else if (last_stage==0)
	{
		for (i=norder/2;i<norder;i++)
		{
			//upfilter97(data,norder/2,1,res);upfilter97(data2,norder/2,0,res);
			upfilter53I(data,norder/2,res);upfilter53III(data2,norder/2,res);
			data +=n;res +=n;data2 +=n;
		}
	}

	//image transposition
	/*for (i=0;i<norder;i++)
	{
		for (j=0;j<norder;j++) im->im_jpeg[i*n+j]=im->im_process[i+j*n];
	}*/

	data=(short*)im->im_process;

	if (im->setup->quality_setting>HIGH1)
	{
		for (i=0;i<os->nhw_res6_bit_len;i++) 
		{
			data[os->nhwresH3[i]]-=32;
		}
		free(os->nhwresH3);

		for (i=0;i<os->nhw_res6_len;i++) 
		{
			data[os->nhwresH4[i]]+=32;
		}
		free(os->nhwresH4);

		for (i=0;i<os->nhw_char_res1_len;i++)
		{
			if ((os->nhw_char_res1[i]&3)==0)
			{
				data[((os->nhw_char_res1[i]<<1)+halfn-2)]+=32;
			}
			else if ((os->nhw_char_res1[i]&3)==1)
			{
				data[(((os->nhw_char_res1[i]-1)<<1)+halfn-2)]-=32;
			}
			else if ((os->nhw_char_res1[i]&3)==2)
			{
				data[(((os->nhw_char_res1[i]-2)<<1)+halfn-1)]+=32;
			}
			else
			{
				data[(((os->nhw_char_res1[i]-3)<<1)+halfn-1)]-=32;
			}
		}

		free(os->nhw_char_res1);
	}

	if (im->setup->quality_setting>HIGH2)
	{
		for (i=0;i<os->qsetting3_len;i++)
		{
			if (!(os->high_qsetting3[i]&1))
			{
				data[(os->high_qsetting3[i]>>1)]+=56;
			}
			else
			{
				data[(os->high_qsetting3[i]>>1)]-=56;
			}
		}

		free(os->high_qsetting3);
	}

	//faster version
	res=im->im_jpeg;data=im->im_process;
	for (i=0;i<norder;i++,res+=n)
	{
		a=i;
		for (j=0;j<norder;j++,a+=n) res[j]=data[a];
	}
}


