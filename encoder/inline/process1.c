
				res= pr[scan]-res256[count];res256[count]=0;

				switch (res) {
					case 0:
					case 1:
						if (p[0]==-7 || p[0]==-8) 
						{
							if (p[-1]<2 && p[-1]>-8) p[0]=-9;
						}
						break;
					case 2:
						switch (p[0]) {
							case -7:
							case -8:
								if (p[-1] <= 1) p[0] = -9;
								break;
							case -6:
								if (p[-1] <= -1 && p[-1]>-8) p[0]=-9;
								break;
							default:
								if (p[0]>15 && !(p[0]&7)) p[0]--;
						}
						break;
					case 3:
						if (quality>=HIGH1)
							{res256[count] = 144; enc->nhw_res5_word_len++;}
						else
						{
							if (p[0] > 15 && !(p[0] & 7)) p[0]--;
							else if (p[0] <= 0 && ((((-p[0]) + 2) & ~3) == 8)) 
							{
								if (p[-1] <= 2) p[0]=-10;
							}
						}
						break;
					default:
						if (res>res_setting) 
						{
							res256[count]=141;enc->nhw_res1_word_len++;

							if (res == 4)
							{
								if (p[0] == 7 || (p[0] & ~1)==8)
								{
									if (p[-1] >= 0 && p[-1] < 8) p[0] += 2;
								}
							}
							else if (res>6) 
							{
								if (res > 7 && quality>=HIGH1) 
								{
									res256[count]=148;
									enc->nhw_res5_word_len++;
									enc->nhw_res1_word_len++;
								}
								else
								{
									if (p[0] > 15 && (p[0] & 7) == 0) p[0]--;
									else if (p[0] == -6 || p[0]== -7 || p[0] == -8) 
									{
										if (p[-1] < 0 && p[-1] > -8) p[0] = -9;
									}
								}
							}
						}
				}


