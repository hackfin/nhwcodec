#include "imgio.h"
#include "codec.h"

#ifndef SWAPOUT_FUNCTION_X
#define SWAPOUT_FUNCTION_X(f) f
#endif

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
	im->im_buffer4=(unsigned char*)calloc(3*im->fmt.end,sizeof(char));

	// OPEN IMAGE
	if ((im256 = fopen(argv[1],"rb")) == NULL )
	{
		printf ("\n Could not open file \n");
		exit(-1);
	}

	// READ IMAGE DATA (tile)
	ret = read_png_tile(im256, im->im_buffer4, im->fmt.tile_size); 
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


int SWAPOUT_FUNCTION_X(main)(int argc, char **argv) 
{	
	image_buffer im;
	encode_state enc;
	int select;
	char *arg;
	codec_setup setup;
	char OutputFile[200];

	if (argv[1]==NULL || argv[1]==0)
	{
		printf("\n Copyright (C) 2007-2013 NHW Project (Raphael C.)\n");
		printf("\n-> nhw_encoder.exe filename.bmp");
		printf("\n  (with filename a bitmap color 512x512 image)\n");
		exit(-1);
	}

	im.setup = &setup;
	setup.quality_setting=NORM;
	imgbuf_init(&im, 9);

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

	int len;

	len=strlen(argv[1]);
	memset(argv[1]+len-4,0,4);
	sprintf(OutputFile,"%s.nhw",argv[1]);

	write_compressed_file(&im, &enc, OutputFile);
	
	// Need to free structures written in previous routine:
	free(enc.tree1);
	free(enc.tree2);

	return 0;
}
