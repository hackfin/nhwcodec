#include "codec.h"

#define REDUCE(x, w)    if (abs(x) < w)  x = 0

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

void reduce_q7(int quality, short *pr, const char *wvlt)
{
	int i, j, count, scan;
	int a, e;
	char w, v;

	int s = 2 * IM_DIM;
	int s2 = 2 * s;

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

void reduce_q9(short *pr, const char *wvlt)
{
	int i, j, count, scan;
	int e;
	int s = 2 * IM_DIM;
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
