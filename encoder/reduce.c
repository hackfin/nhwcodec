#include "codec.h"

#define REDUCE(x, w)    if (abs(x) < w)  x = 0

#define FLOOR2(x) ( (x) & ~1)

#define RESIII_GETXY(x, y, off)  \
					(((((y) >> 1) + (x)) >> 1) + (off))

inline void reduce(short *q, int s, char w0, char w1)
{

	short *q1= &q[IM_DIM];

	REDUCE(q1[0], w0); REDUCE(q1[1], w0);
	q1 += s;
	REDUCE(q1[0], w0); REDUCE(q1[1], w0);
	
	w0 += 6;
	q += 2 * IM_SIZE;

	REDUCE(q[0], w0); REDUCE(q[1], w0);
	q += s;
	REDUCE(q[0], w0); REDUCE(q[1], w0);
	
	q -= IM_DIM;
	REDUCE(q[0], w1); REDUCE(q[1], w1);
	q += s;
	REDUCE(q[0], w1); REDUCE(q[1], w1);
}

#define CONDITION_A(p, w, v) \
	   abs(p[4]-p[0]) < w \
	&& abs(p[4]-p[3]) < w \
	&& abs(p[1]-p[0]) < w \
	&& abs(p[3]-p[1]) < w \
	&& abs(p[3]-p[2]) < (v-2)

#define CONDITION_B(p, w, v) \
	   abs(p[4]-p[0]) < w \
	&& abs(p[4]-p[3]) < w \
	&& abs(p[1]-p[0]) < w \
	&& abs(p[3]-p[1]) < v \
	&& abs(p[3]-p[2]) < v

void reduce_q7_LL(int quality, short *pr, const unsigned char *wvlt)
{
	int i, j, count, scan;
	int e;
	char w, v;

	int s = 2 * IM_DIM;
	int s2 = 2 * s;

	// *LL*  HL
	//  LH   HH

	for (i=0,scan=0;i<(IM_SIZE);i+=s)
	{
		for (scan=i,j=0;j<(IM_DIM>>1)-4;j++,scan++)
		{
			short *p = &pr[scan];
			w = wvlt[0];
			v = wvlt[1];

			if (CONDITION_A(p, w, v))
			{
				if      ((p[3]-p[1])>5 && (p[2]-p[3]>=0)) { p[2]=p[3]; }
				else if ((p[1]-p[3])>5 && (p[2]-p[3]<=0)) { p[2]=p[3]; }
				else if ((p[1]-p[3])>5 && (p[2]-p[1]>=0)) { p[2]=p[1]; }
				else if ((p[3]-p[1])>5 && (p[2]-p[1]<=0)) { p[2]=p[1]; }
				else if ((p[3]-p[2])>0 && (p[2]-p[1]>0))  { }
				else if ((p[1]-p[2])>0 && (p[2]-p[3]>0))  { }
				else
				                                     p[2]=(p[3]+p[1])>>1;
						
				for (count=1;count<4;count++)
				{
					short *q = &pr[((scan+count)<<1)];
					reduce(q, s, wvlt[5], wvlt[4]);
				}
					
				if (quality<=LOW9)
				{
					for (count=1;count<4;count++)
					{
						short *q = &p[count];
						REDUCE(q[IM_DIM>>1], 11);
						REDUCE(q[IM_SIZE], 12);
						REDUCE(q[IM_SIZE+(IM_DIM>>1)],13);
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
						//p[2]=(p[3]+p[1]+1)>>1;
						
						for (count=1;count<4;count++) {
							short *q = &pr[((scan+count)<<1)];
							reduce(q, s, wvlt[5], wvlt[4]);
						}
						
						if (quality<=LOW9)
						{
							for (count=1;count<4;count++)
							{
								short *q = &p[count];
								REDUCE(q[IM_DIM>>1], 11);
								REDUCE(q[IM_SIZE], 12);
								REDUCE(q[IM_SIZE+(IM_DIM>>1)],13);
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
	
	for (i=0,scan=0;i<(IM_SIZE)-s2;i+=s)
	{
		for (scan=i,j=0;j<(IM_DIM>>1)-2;j++,scan++)
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
				reduce(q, s, wvlt[5], 32);

				if (quality<=LOW9)
				{
					for (e=count-1;e<count+2;e++)
					{
						q = &pr[e];
						REDUCE(q[(IM_DIM>>1)], 11);
						q += IM_SIZE;
						REDUCE(q[0], 12);
						REDUCE(q[(IM_DIM>>1)],13);
					}
				}
			}
		}
	}

	// *LL*  HL
	//  LH   HH
	
	for (i=0,scan=0;i<(IM_SIZE)-s2;i+=s)
	{
		for (scan=i,j=0;j<(IM_DIM>>1)-2;j++,scan++)
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

					reduce(q, s, wvlt[5], 32);
				}
				
				if (quality<=LOW9)
				{
					for (e=count-1;e<count+2;e++)
					{
						short *q = &pr[e];
						REDUCE(q[(IM_DIM>>1)], 11);
						q += IM_SIZE;
						REDUCE(q[0], 12);
						REDUCE(q[(IM_DIM>>1)],13);
					}
				}
			}
		}
	}
}

void reduce_q9_LL(short *pr, const unsigned char *wvlt, int s)
{
	int i, j, count, scan;
	char w;

	for (i=0,scan=0;i<(IM_SIZE);i+=s)
	{
		for (scan=i,j=0;j<(IM_DIM>>1)-2;j++,scan++)
		{
			w = wvlt[6];
			short *p = &pr[scan];
			if (abs(p[2]-p[1])  < w
			 && abs(p[2]-p[0])  < w
			 && abs(p[1]-p[0])  < w)
			{
				count=scan+1;

				reduce(&pr[(count<<1)], s, wvlt[5], 34);
					
				short *q = &pr[count];
				REDUCE(q[(IM_DIM>>1)], 11);
				q += IM_SIZE;
				REDUCE(q[0], 12);
				REDUCE(q[(IM_DIM>>1)],13);
			}
		}
	}
}

void reduce_q9_LH(short *pr, const unsigned char *wvlt, int ratio, int s)
{
	int i, j, scan;
		
	//  LL    HL
	// *LH*   HH

	for (i=IM_SIZE;i<(2*IM_SIZE);i+=s)
	{
		for (scan=i,j=0;j<(s>>1);j++,scan++)
		{
			short *p = &pr[scan];
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


#if 0

// PROTOTYPE, unused.
// Possible replacement for the above if() inside loop

void reduce_lq9(short *pr, const char *wvlt)
{
	int i, j, count, scan;
	int s = 2 * IM_DIM;
	int s2 = 2 * s;
	char w, v;
	int e;


	for (i=0,scan=0;i<(IM_SIZE);i+=s)
	{
		for (scan=i,j=0;j<(IM_DIM>>1)-4;j++,scan++)
		{
			short *p = &pr[scan];

			w = wvlt[0];
			v = wvlt[1];

			if (CONDITION_A(p, w, v)) {
				for (count=1;count<4;count++)
				{
					short *q = &p[count];
					REDUCE(q[IM_DIM>>1], 11);
					REDUCE(q[IM_SIZE], 12);
					REDUCE(q[IM_SIZE+(IM_DIM>>1)],13);
				}
			} else {
				w = v + 1;
				v += 6;
				if (CONDITION_B(p, w, v))
				{
					if (((p[3]-p[2])>=0 && (p[2]-p[1])>=0) ||
						((p[3]-p[2])<=0 && (p[2]-p[1])<=0)) 
					{
						for (count=1;count<4;count++)
						{
							short *q = &p[count];
							REDUCE(q[IM_DIM>>1], 11);
							REDUCE(q[IM_SIZE], 12);
							REDUCE(q[IM_SIZE+(IM_DIM>>1)],13);
						}
					}
				}
			}

		}
	}

	w = wvlt[2];
	v = wvlt[3];

	for (i=0,scan=0;i<(IM_SIZE)-s2;i+=s)
	{
		for (scan=i,j=0;j<(IM_DIM>>1)-2;j++,scan++)
		{
			short *p = &pr[scan];
			if (abs(p[1]-p[s2+1]) < w
			 && abs(p[s]-p[s+2])  < w
			 && abs(p[s+1]-p[s]) < (v-1)
			 && abs(p[1]-p[s+1]) <  v)
			{
				for (e=count-1;e<count+2;e++)
				{
					short *q = &pr[e];
					REDUCE(q[(IM_DIM>>1)], 11);
					q += IM_SIZE;
					REDUCE(q[0], 12);
					REDUCE(q[(IM_DIM>>1)],13);
				}
			} else
			if (abs(p[2]-p[1])  <w
			 && abs(p[1]-p[0])  <w
			 && abs(p[0]-p[s])  <w
			 && abs(p[2]-p[s+2])<w)
			{
				for (e=count-1;e<count+2;e++)
				{
					short *q = &pr[e];
					REDUCE(q[(IM_DIM>>1)], 11);
					q += IM_SIZE;
					REDUCE(q[0], 12);
					REDUCE(q[(IM_DIM>>1)],13);
				}
			}
		}
	}

}

#endif

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


const char quality_special_default[7] = { 16, 28, 11, 8, 5, 0xff, 0xff };
const char quality_special12500[7] = { 19, 31, 13, 9, 6, 0xff, 0xff };
const char quality_special10000[7] = { 18, 30, 12, 8, 6, 0xff, 0xff };
const char quality_special7000[7] = { 17, 29, 11, 8, 5, 0xff, 0xff };

int configure_wvlt(int quality, char *wvlt)
{
	const char *w;

	if (quality < 0) quality = 0;
	else if (quality > LOW6) quality = LOW6;

	w = quality_table[quality];
	COPY_WVLT(wvlt, w)
	return 0;
}

void reduce_generic_LH_HH(short *pr, short *resIII, int step, int ratio, char *wvlt, int thr)
{
	int i, scan, j;
	//  LL    HL 
	// *LH*   HH

	for (i=(2*IM_SIZE);i<(4*IM_SIZE);i+=step)
	{
		for (scan=i,j=0;j<(IM_DIM);j++,scan++)
		{
			short *p = &pr[scan];
			if (abs(p[0])>=ratio &&  abs(p[0])<(wvlt[0]+2)) 
			{	
				short tmp = resIII[RESIII_GETXY(j, (i-(2*IM_SIZE)), IM_SIZE >> 1)];
				if (abs(tmp) < wvlt[3]) p[0]=0;
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

		for (scan=i+(IM_DIM),j=(IM_DIM);j<(step-1);j++,scan++)
		{
			short *p = &pr[scan];
			if (abs(p[0])>=ratio) {

				short tmp = resIII[RESIII_GETXY(j-IM_DIM, i-(2*IM_SIZE), (IM_SIZE>>1)+(IM_DIM>>1))];
				if (abs(p[0])<(wvlt[1]+1)) 
				{	
					if (abs(tmp)<(wvlt[3]+1)) p[0]=0;
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
				
				if (abs(p[0])<wvlt[1]
				 && (abs(p[-1])<ratio
					 && abs(p[1])<ratio
					 || abs(p[0])<(wvlt[1]-5))) {
						if      (p[0] >=  thr) p[0] = 7;
						else if (p[0] <= -thr) p[0] = -7;
						else     p[0]=0;							
				}
			}
		}
	}


}


void reduce_generic(int quality, short *resIII, short *pr, char *wvlt, encode_state *enc, int ratio)
{
	int i, j, scan, count;

	int step = 2 * IM_DIM;

	if (quality<NORM && quality>LOW5)
	{

		//  LL    HL
		// *LH*   HH

		for (i=(2*IM_SIZE);i<(4*IM_SIZE);i+=step)
		{
			for (scan=i,j=0;j<IM_DIM;j++,scan++)
			{
				short *p = &pr[scan];

				if (abs(p[0])>=ratio && abs(p[0])<9) 
				{	
					 if (p[0]>0) p[0]=7;else p[0]=-7;	
				}
			}

			for (scan=i+(IM_DIM),j=(IM_DIM);j<step;j++,scan++)
			{
				short *p = &pr[scan];
				if (abs(p[0])>=ratio && abs(p[0])<=14) 
				{	
					 if (p[0]>0) p[0]=7;else p[0]=-7;	
				}
			}
		}
	}
	else if (quality<=LOW5 && quality>=LOW6)
	{ 
		wvlt[0] = 11;

		if (quality==LOW5) wvlt[1]=19;
		else if (quality==LOW6) wvlt[1]=20;
		
		//  LL    HL
		// *LH*   HH

		for (i=(2*IM_SIZE);i<(4*IM_SIZE);i+=step)
		{
			for (scan=i,j=0;j<(IM_DIM);j++,scan++)
			{		
				short *p = &pr[scan];
				if (abs(p[0])>=ratio && abs(p[0])<wvlt[0]) 
				{	
					p[0]=0;			
				}
			}

			for (scan=i+(IM_DIM),j=(IM_DIM);j<step;j++,scan++)
			{
				short *p = &pr[scan];
				if (abs(p[0])>=ratio && abs(p[0])<wvlt[1]) 
				{	
					if (p[0]>=14) p[0]=7;
					else if (p[0]<=-14) p[0]=-7;	
					else 	p[0]=0;	
				}
			}
		}
	}
	else if (quality<LOW6)
	{ 
		if (quality<=LOW8)
		{
			for (i=(2*IM_SIZE),count=0;i<(4*IM_SIZE);i++)
			{
				if (abs(pr[i])>=12) count++;
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
	
		//  LL   *HL*
		//  LH    HH 
		
		for (i=0;i<(2*IM_SIZE);i+=step)
		{
			for (scan=i+IM_DIM,j=IM_DIM;j<step;j++,scan++)
			{
				short *p = &pr[scan];
				if (abs(p[0]) >= ratio) {
					if (abs(p[0]) < (wvlt[2]+2)) 
					{	
						short tmp = resIII[RESIII_GETXY(j-IM_DIM, i, IM_DIM >> 1)];

						if (abs(tmp) < wvlt[3])
							p[0] = 0;
						// if (abs(resIII[(((i>>1)+(j-IM_DIM))>>1)+(IM_DIM>>1)])<wvlt[3]) p[0]=0;
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


		if (quality > 10) {
			reduce_generic_LH_HH(pr, resIII, step, ratio, wvlt, 16);
		} else {
			reduce_generic_LH_HH(pr, resIII, step, ratio, wvlt, 0xffff);
		}

	}

	if (quality>LOW4)
	{ 

		//  LL   *HL*
		//  LH    HH 
		
		for (i=step; i<((2*IM_SIZE)-step);i+=step)
		{
			for (scan=i+(IM_DIM+1),j=(IM_DIM+1);j<(step-1);j++,scan++)
			{
				short *p = &pr[scan];
				if (p[0]>4 && p[0]<8)
				{
					if (p[-1]>3 && p[-1]<=7)
					{
						if (p[1]>3 && p[1]<=7)
						{
							p[0]=12700;p[-1]=10100;p[1]=10100;
						}
					}
				}
				else if (p[0]<-4 && p[0]>-8)
				{
					if (p[-1]<-3 && p[-1]>=-7)
					{
						if (p[1]<-3 && p[1]>=-7)
						{
							p[0]=12900;p[-1]=10100;p[1]=10100;	 
						}
					}
				}
				else if ((p[0]==-7) && (p[1]==-6 || p[1]==-7))
				{
					p[0]=10204;p[1]=10100;
				}
				else if (p[0]==7 && p[1]==7)
				{
					p[0]=10300;p[1]=10100;
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

		for (i=((2*IM_SIZE)+step);i<((4*IM_SIZE)-step);i+=step)
		{
			for (scan=i+1,j=1;j<(IM_DIM-1);j++,scan++)
			{
				short *p = &pr[scan];
				if (p[0]>4 && p[0]<8)
				{
					if (p[-1]>3 && p[-1]<=7)
					{
						if (p[1]>3 && p[1]<=7)
						{
							p[0]=12700;p[-1]=10100;p[1]=10100;
						}
					}
				}
				else if (p[0]<-4 && p[0]>-8)
				{
					if (p[-1]<-3 && p[-1]>=-7)
					{
						if (p[1]<-3 && p[1]>=-7)
						{
							p[0]=12900;p[-1]=10100;p[1]=10100;
						}
					}
				}
				else if (p[0]==-6 || p[0]==-7)
				{
					if (p[1]==-7)
					{
						p[0]=10204;p[1]=10100;
					}
					else if (p[-step]==-7)
					{
						if (abs(p[IM_DIM])<8) p[IM_DIM]=10204;p[0]=10100;
					}
					
				}
				else if (p[0]==7)
				{
					if (p[1]==7)
					{
						p[0]=10300;p[1]=10100;
					}
					else if (p[-step]==7)
					{
						if (abs(p[IM_DIM])<8) p[IM_DIM]=10300;p[0]=10100;
					}
				}
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
}

void process_res_q8(int quality, short *pr, short *res256, encode_state *enc)
{
	int i, j, count, res, stage, scan;
	int res_setting;
	int step = 2 * IM_DIM;

	if (quality>=NORM) res_setting=3;
	else if (quality>=LOW2) res_setting=4;
	else if (quality>=LOW5) res_setting=6;
	else if (quality>=LOW7) res_setting=8;

	// *LL*  HL
	//  LH   HH

	// TODO: Pad last 2 lines with something sensible

	for (j=0,count=0,res=0,stage=0;j<IM_DIM;j++)
	{
		for (scan=j,count=j,i=0;i<((2*IM_SIZE)-step);i+=step,scan+=step,count+=IM_DIM)
		{
			int r0 = res256[count+IM_DIM];
			int rcs = res256[count+step]; // This is stepping out of image bounds on the last line
#include "inline/process0.c"
		}	
	}

	enc->nhw_res1_word_len=0;enc->nhw_res3_word_len=0;enc->nhw_res5_word_len=0;

	for (i=0,count=0,stage=0,res=0;i<((4*IM_SIZE)>>1);i+=step)
	{
		for (scan=i,j=0;j<IM_DIM;j++,scan++,count++)
		{
			if (res256[count]<12000)
			{
				// Transposed check pointer:
				short *p = &pr[(j<<9)+(i>>9)+(IM_DIM)];
#include "inline/process1.c"
			}
			else 
			{
				short r = res256[count];
				switch (r) {
					case 14000:
						r=140; enc->nhw_res1_word_len++; 
						break;
					case 14500:
						r=145; enc->nhw_res5_word_len++;
						break;
					case 12200:
						r=122; enc->nhw_res3_word_len++; 
						break;
					case 12100:
						r=121; enc->nhw_res3_word_len++;
						break;
					case 12300:
						r=123; enc->nhw_res3_word_len++; 
						break;
					case 12400:
						r=124; enc->nhw_res3_word_len++;
						break;
					case 14100:
						r=141; enc->nhw_res1_word_len++; 
						break;
					case 12500:
						r=125; enc->nhw_res3_word_len++;
						enc->nhw_res1_word_len++;
						break;
					case 12600:
						r=126;enc->nhw_res3_word_len++;
						enc->nhw_res1_word_len++;
						break;
					case 14900:
						r=149;enc->nhw_res5_word_len++;
						enc->nhw_res1_word_len++;
						break;
				}	
				res256[count] = r;
			}
		}
	}
}