			if (abs(p[0])>=(ratio-2)) 
			{	

				if (abs(p[0])<y_wavelet2)
				{
					if (((abs(p[-1]))+2)>=8) scan++;
					if (((abs(p[1]))+2)>=8) scan++;
					if (((abs(p[-step]))+2)>=8) scan++;
					if (((abs(p[step]))+2)>=8) scan++;

					if (scan<3 && p[0]<y_wavelet && p[0]>(-y_wavelet))
					{
						//printf("%d %d %d\n",pr[a-1],pr[a],pr[a+1]);
						if (e<-6)     p[0]=-7;
						else if (e>6) p[0]=7;
					}
					/*else if (!scan && abs(pr[a])<9) 
					{
						if (pr[a]<-6) pr[a]=-7;
						else if (pr[a]>6) pr[a]=7;
					}*/

					scan=0;
				}
				else if (abs(p[0])>6)
				{

					if (e>=8 && (e&7)<2) 
					{
						if (p[1]>7 && p[1]<10000) p[1]--;
						//if (p[step]>8) p[step]--;
					}
					else if (e==-7 && p[1]==8) p[0]=-8;
					else if (e==8 && p[1]==-7) p[1]=-8;
					else if (e<-7 && ((-e)&7)<2)
					{
						if (p[1]<-14 && p[1]<10000)
						{
							if (((-p[1])&7)==7) p[1]++;
							else if (((-p[1])&7)<2 && j<(step-2) && p[2]<=0)
								p[1]++;
						}
					}
				}
			}
			else p[0]=0;

