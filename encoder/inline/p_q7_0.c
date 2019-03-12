			short *p = &pr[scan];
			if (abs(p[4]-p[0])<wvlt[0] &&
				abs(p[4]-p[3])<wvlt[0] && abs(p[1]-p[0])<wvlt[0] && 
				abs(p[3]-p[1])<wvlt[0] && abs(p[3]-p[2])<(wvlt[1]-2))
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
					int o = IM_DIM;

					if (abs(q[o])<wvlt[5])       q[o]=0;
					if (abs(q[o+1])<wvlt[5])     q[o+1]=0;
					o *= 3;
					if (abs(q[o])<wvlt[5])   q[o]=0;
					o++;
					if (abs(q[o])<wvlt[5]) q[o]=0;
					
					o = IM_SIZE << 1;
					if (abs(q[o])<(wvlt[5]+6))   q[o]=0;
					if (abs(q[o+1])<(wvlt[5]+6)) q[o+1]=0;
					if (abs(q[o+s])<(wvlt[5]+6)) q[o+s]=0;
					if (abs(q[o+s+1])<(wvlt[5]+6)) q[o+s+1]=0;

					q = &q[(2*IM_SIZE)+IM_DIM];
					if (abs(q[0])<wvlt[4])   q[0]=0;
					if (abs(q[1])<wvlt[4])   q[1]=0;
					if (abs(q[s])<wvlt[4])   q[s]=0;
					if (abs(q[s+1])<wvlt[4]) q[s+1]=0;
				}
					
				if (quality<=LOW9)
				{
					for (count=1;count<4;count++)
					{
						short *q = &p[count];
						if (abs(q[(IM_DIM>>1)])<11)
							q[(IM_DIM>>1)]=0;
						if (abs(q[IM_SIZE])<12)
							q[IM_SIZE]=0;
						if (abs(q[IM_SIZE+(IM_DIM>>1)])<13)
							q[IM_SIZE+(IM_DIM>>1)]=0;
					}
				}
						
			}
			else if (abs(p[4]-p[0])<(wvlt[1]+1)
			    &&   abs(p[4]-p[3])<(wvlt[1]+1)
			    &&   abs(p[1]-p[0])<(wvlt[1]+1))
			{
				if (abs(p[3]-p[1])<(wvlt[1]+6)
				 && abs(p[3]-p[2])<(wvlt[1]+6))
				{
					if (((p[3]-p[2])>=0 && (p[2]-p[1])>=0) ||
						((p[3]-p[2])<=0 && (p[2]-p[1])<=0)) 
					{
						//p[2]=(p[3]+p[1]+1)>>1;
						
						for (count=1;count<4;count++)
						{
							short *q = &pr[((scan+count)<<1) + IM_DIM];
							short *q1 = &q[(2*IM_DIM)];

							if (abs(q[0])<wvlt[5])            q[0]=0;
							if (abs(q[1])<wvlt[5])            q[1]=0;
							if (abs(q1[0])<wvlt[5])   q1[0]=0;
							if (abs(q1[1])<wvlt[5])   q1[1]=0;
						
							q -= IM_DIM;
							q = &q[(2*IM_SIZE)];

							if (abs(q[0])<(wvlt[5]+6))   q[0]=0;
							if (abs(q[1])<(wvlt[5]+6))   q[1]=0;
							if (abs(q[s])<(wvlt[5]+6))   q[s]=0;
							if (abs(q[s+1])<(wvlt[5]+6)) q[s+1]=0;
						
							if (abs(q[0])<wvlt[4])   q[0]=0;
							if (abs(q[1])<wvlt[4])   q[1]=0;
							q += s;
							if (abs(q[0])<wvlt[4])   q[0]=0;
							if (abs(q[1])<wvlt[4])   q[1]=0;
						}
						
						if (quality<=LOW9)
						{
							for (count=1;count<4;count++)
							{
								short *q = &p[count];
								if (abs(q[(IM_DIM>>1)])<11)
									q[(IM_DIM>>1)]=0;
								if (abs(q[IM_SIZE])<12)
									q[IM_SIZE]=0;
								if (abs(q[IM_SIZE+(IM_DIM>>1)])<13)
									q[IM_SIZE+(IM_DIM>>1)]=0;
							}
						}
					}
				}
			}

