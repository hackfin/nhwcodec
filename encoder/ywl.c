#include "codec.h"

void ywl(int quality, short *pr, int ratio)
{

	int a;
	int y_wavelet, y_wavelet2;
	int i, j, count, scan;
	int e;

	short *p;

	int step = 2 * IM_DIM;

	if (quality>HIGH2) 
	{
		y_wavelet=8;y_wavelet2=4;
	}
	else
	{
		y_wavelet=9;y_wavelet2=9;
	}

	for (i=(2*IM_DIM),count=0,scan=0;i<((4*IM_SIZE>>1)-(2*IM_DIM));i+=(2*IM_DIM))
	{
		for (j=(IM_DIM+1);j<(2*IM_DIM-1);j++)
		{
			if (abs(pr[i+j])>=(ratio-2)) 
			{
				a=i+j;

				if (abs(pr[a])<y_wavelet2)
				{
					if (((abs(pr[a-1]))+2)>=8) scan++;
					if (((abs(pr[a+1]))+2)>=8) scan++;
					if (((abs(pr[a-(2*IM_DIM)]))+2)>=8) scan++;
					if (((abs(pr[a+(2*IM_DIM)]))+2)>=8) scan++;

					if (scan<3 && pr[a]<y_wavelet && pr[a]>-y_wavelet) 
					{
						//printf("%d %d %d\n",pr[a-1],pr[a],pr[a+1]);
						if (pr[a]<-6) pr[a]=-7;
						else if (pr[a]>6) pr[a]=7;
					}
					/*else if (!scan && abs(pr[a])<9) 
					{
						if (pr[a]<-6) pr[a]=-7;
						else if (pr[a]>6) pr[a]=7;
					}*/

					scan=0;
				}
			}
			else pr[i+j]=0;

			//if (abs(pr[i+j])<9) pr[i+j]=0;
			if (abs(pr[i+j])>6)
			{
			e=pr[i+j];

			if (e>=8 && (e&7)<2) 
			{
				if (pr[i+j+1]>7 && pr[i+j+1]<10000) pr[i+j+1]--;
				//if (pr[i+j+(2*IM_DIM)]>8) pr[i+j+(2*IM_DIM)]--;
			}
			else if (e==-7 && pr[i+j+1]==8) pr[i+j]=-8;
			else if (e==8 && pr[i+j+1]==-7) pr[i+j+1]=-8;
			else if (e<-7 && ((-e)&7)<2)
			{
				if (pr[i+j+1]<-14 && pr[i+j+1]<10000)
				{
					if (((-pr[i+j+1])&7)==7) pr[i+j+1]++;
					else if (((-pr[i+j+1])&7)<2 && j<((2*IM_DIM)-2) && pr[i+j+2]<=0) pr[i+j+1]++;
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
			if (abs(pr[i+j])>=(ratio-2)) 
			{	
				a=i+j;

				if (abs(pr[a])<y_wavelet2)
				{
					if (((abs(pr[a-1]))+2)>=8) scan++;
					if (((abs(pr[a+1]))+2)>=8) scan++;
					if (((abs(pr[a-(2*IM_DIM)]))+2)>=8) scan++;
					if (((abs(pr[a+(2*IM_DIM)]))+2)>=8) scan++;

					if (scan<3 && pr[a]<y_wavelet && pr[a]>(-y_wavelet))
					{
						if (pr[a]<0) pr[a]=-7;else pr[a]=7;
					}
					else if (!scan && abs(pr[a])<y_wavelet2)
					{
						if (pr[a]<0) pr[a]=-7;else pr[a]=7;
					}

					scan=0;
				}
			}
			else pr[i+j]=0;

			if (abs(pr[i+j])>6)
			{

			e=pr[i+j];

			if (e>=8 && (e&7)<2) 
			{
				if (pr[i+j+1]>7 && pr[i+j+1]<10000) pr[i+j+1]--;
				//else if (pr[i+j+(2*IM_DIM)]>8 && pr[i+j+(2*IM_DIM)]<10000) pr[i+j+(2*IM_DIM)]--;
			}
			else if (e==-7 && pr[i+j+1]==8) pr[i+j]=-8;
			else if (e==8 && pr[i+j+1]==-7) pr[i+j+1]=-8;
			else if (e<-7 && ((-e)&7)<2)
			{
				if (pr[i+j+1]<-14 && pr[i+j+1]<10000)
				{
					if (((-pr[i+j+1])&7)==7) pr[i+j+1]++;
					else if (((-pr[i+j+1])&7)<2 && j<(IM_DIM-2) && pr[i+j+2]<=0) pr[i+j+1]++;
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
			if (abs(pr[i+j])>=(ratio-1)) 
			{	
				a=i+j;

				if (abs(pr[a])<y_wavelet)
				{
					if (((abs(pr[a-1]))+2)>=8) scan++;
					if (((abs(pr[a+1]))+2)>=8) scan++;
					if (((abs(pr[a-(2*IM_DIM)]))+2)>=8) scan++;
					if (((abs(pr[a+(2*IM_DIM)]))+2)>=8) scan++;

					if (scan<3 && pr[a]<y_wavelet && pr[a]>-y_wavelet)
					{
						if (pr[a]<0) pr[a]=-7;else pr[a]=7;
					}
					/*else if (!scan && abs(pr[a])<11) 
					{
						if (pr[a]<0) pr[a]=-7;else pr[a]=7;
					}*/

					scan=0;
				}
			}
			else pr[i+j]=0;

			if (abs(pr[i+j])>6)
			{
			e=pr[i+j];

			if (e>=8 && (e&7)<2) 
			{
				if (pr[i+j+1]>7 && pr[i+j+1]<10000) pr[i+j+1]--;
				//else if (pr[i+j+(2*IM_DIM)]>7 && pr[i+j+(2*IM_DIM)]<10000) pr[i+j+(2*IM_DIM)]--;
			}
			else if (e==-7 && pr[i+j+1]==8) pr[i+j]=-8;
			else if (e==8 && pr[i+j+1]==-7) pr[i+j+1]=-8;
			else if (e<-7 && ((-e)&7)<2)
			{
				if (pr[i+j+1]<-14 && pr[i+j+1]<10000)
				{
					if (((-pr[i+j+1])&7)==7) pr[i+j+1]++;
					else if (((-pr[i+j+1])&7)<2 && j<((2*IM_DIM)-2) && pr[i+j+2]<=0) pr[i+j+1]++;
				}
			}
			}
		}
	}


}
