			int k = (i >> 9) + (j << 9);
			short *p;
			p = &pr[scan];

			int a;

			stage = k + IM_DIM;

			short *q = &pr[stage];
			
			short *p1 = &p[step];
			short *p2 = &p1[step];

			res = p[0] - res256[count];
			a = p[step] - r0;

			short qd = *p2 - rcs;


////////////////////////////////////////////////////////////////////////////
// XXX Unfortunately, this can't simply be put into a table.
// Do we need to develop a VLIW machine?
////////////////////////////////////////////////////////////////////////////

#define _MOD_ASSIGN(x, y) \
	{ res256[count]=x; *p1 += y; *p2 += y; }




			if (res==2 && a==2 && qd>=2)
			{
				if (qd<5 || qd>6) _MOD_ASSIGN(12400, -2);
			}
			else if (((res==2 && a==3) || (res==3 && a==2)) && qd>1 && qd<6)
				 _MOD_ASSIGN(12400, -2)
			else if ((res==3 && a==3))
			{
				if (qd>0 && qd<6)
					 _MOD_ASSIGN(12400, -2)
				else if (quality>=LOW1)
				{
					res256[count]=12100;p[step]=r0;
				}
			}
			else if (a==-4 && (res==2 || res==3) && (qd==2 || qd==3))
			{
				if (res==2 && qd==2) p[step]++;
				else _MOD_ASSIGN(12400, -2)
			} // DONE
			else if (res==1 && a==3 && qd==2) // DONE
			{
				// FIXME: No compare of loop variable inside loop
				if (i>0) 
				{
					if ((p[-step]-res256[count-IM_DIM])>=0) 
						_MOD_ASSIGN(12400, -2)
				}
			}
			else if ((res==3 || res==4 || res==5 || res>6) &&
				(a==3 ||
				(a&65534)==4))
			{
				if ((res)>6) {res256[count]=12500;p[step]=r0;}
				else if (quality>=LOW1) // REDUNDANT
				{
					res256[count]=12100;p[step]=r0;
				}
				else if (quality==LOW2) // DONE
				{
					if (res<5 && a==5) p[step] =14100;
					else if (res>=5) res256[count]=14100;
					else if (res==3 && a>=4) p[step] =14100;
					
					res256[count+IM_DIM] = p[step];
				}
			}
			else if ((res==2 || res==3) && (a==2 || a==3))
			{ 
				if (qd == 0 || qd == 1)
				{
					if ((p[1]-res256[count+1])==2 || (p[1]-res256[count+1])==3)
					{
						if ((p1[1]-res256[count+(IM_DIM+1)])==2 || (p1[1]-res256[count+(IM_DIM+1)])==3)
						{
							if ((p2[1]-res256[count+(2*IM_DIM+1)])>0)
							{
								 _MOD_ASSIGN(12400, -2)
							}
						}
					}
				}
			}
			else if (a==4 && (res==-2 || res==-3) && (qd==-2 || qd==-3)) // DONE
			{
				if (res==-2 && qd==-2) p[step]--;
				else _MOD_ASSIGN(12300, 2)
			}
			else if ((res==-3 || res==-4 || res==-5 || res<-7) &&
				(a == -3 || a == -4 || a == -5))
			{
				if (res<-7) 
				{
					res256[count]=12600;p[step]=r0;
				}
				else if (quality>=LOW1)
				{
					res256[count]=12200;p[step]=r0;
				}
				else if (quality==LOW2)
				{
					if (res>-5 && a==-5)       p[step]=14000;
					else if (res<=-5) res256[count]=14000;
					else if (res==-3 && a<=-4) p[step]=14000;
					
					res256[count+IM_DIM]=p[step];
				}
			}
			else if (a==-2 || a==-3)
			{
				if (res==-2 || res==-3)
				{
					if(qd<0) _MOD_ASSIGN(12300, 2)
					else if (res==-3 && quality>=HIGH1)
					{
						res256[count]=14500;
					}
					else if (qd == 0)
					{
						if ((p[1]-res256[count+1])==-2 || (p[1]-res256[count+1])==-3)
						{
							if ((p[(2*IM_DIM+1)]-res256[count+(IM_DIM+1)])==-2
							 || (p[(2*IM_DIM+1)]-res256[count+(IM_DIM+1)])==-3)
							{
								if ((p2[(1)]-res256[count+(2*IM_DIM+1)])<0)
									_MOD_ASSIGN(12300, 2)
							}
						}
					}
					else if (res==-2) goto L_W2;
					else goto L_W3;
				}
				else if (res==-1 && a==-3 && qd==-2)
				{
					// FIXME: No compare of loop variable inside loop
					if (i>0) 
					{
						if ((p[-step]-res256[count-IM_DIM])<=0) 
							_MOD_ASSIGN(12300, 2)
					}
				}
				else if (res==-1)
				{
					if (qd==-3)
						_MOD_ASSIGN(12300, 2)
					else goto L_W1;

				}
				else if (res==-4)
				{
					if (qd<-1)
					{
						if(qd>-4)
							_MOD_ASSIGN(12300, 2)
						else goto L_W5;
					}
					else goto L_W5;
				}
			}
			else if (!res || res==-1)
			{
L_W1:
				if (q[0]==7)
				{
					if (q[0-1]>=0 && q[0-1]<8) q[0]+=2;
				}
				else if (q[0]==8)
				{
					if (q[0-1]>=-2 && q[0-1]<8) q[0]+=2;
				}
			}
			else if (res==-2)
			{
L_W2:

				if (q[0]<-14)
				{
					if (!((-q[0])&7) || ((-q[0])&7)==7) q[0]++;
				}
				else if (q[0]==7 || (q[0]&65534)==8)
				{
					if (q[0-1]>=-2) q[0]+=3;
				}
			}
			else if (res==-3) 
			{
L_W3:			if (quality>=HIGH1) {res256[count]=14500;} // Redundant
				else if (q[0]<-14)
				{
					if (!((-q[0])&7) || ((-q[0])&7)==7)
					{
						q[0]++;
					}
				}
				else if (q[0]>=0 && ((q[0]+2)&65532)==8)
				{
					if (q[0-1]>=-2) q[0]=10;
				}
				else if (q[0]>14 && (q[0]&7)==7)
				{
					q[0]++;
				}
			}
			else if (res<(-res_setting))
			{
L_W5:			res256[count]=14000;

				if (res==-4)
				{

					if (q[0]==-7 || q[0]==-8) 
					{
						if (q[0-1]<2 && q[0-1]>-8) q[0]=-9;
					}
				}
				else if (res<-6)
				{
					if (res<-7 && quality>=HIGH1) {res256[count]=14900;}
					else
					{
						if (q[0]<-14)
						{
							if (!((-q[0])&7) || ((-q[0])&7)==7) q[0]++;
						}
						else if (q[0]==7 || q[0]==8)
						{
							if (q[0-1]>=-1 && q[0-1]<8) q[0]+=3;
						}
					}
				}
			}

