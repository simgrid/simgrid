#!/usr/bin/env python

import sys
from math import sqrt


if len(sys.argv) != 2 and len(sys.argv) != 4:
	print("Usage : %s datafile", sys.argv[0])
	print("or    : %s datafile p1 p2", sys.argv[0])
	print("where : p1 < p2 belongs to sizes in datafiles")
	sys.exit(-1)

if len(sys.argv) == 4:
	p1=int(sys.argv[2])
	p2=int(sys.argv[3])

##-----------------------------------------
## avg : return average of a list of values
## param l list of values
##-----------------------------------------
def avg (l):
	sum=0
	for e in l:
		sum=sum+e;
	return sum/len(l)

##-------------------------------------------------
## cov : covariance 
## param X first data vector (..x_i..)
## param Y second data vector (..x_i..)
## = 1/n \Sum_{i=1}^n (x_i - avg(x)) * (y_i - avg(y))
##--------------------------------------------------
def cov(X,Y):

	n=len(X)   #  n=len(X)=len(Y)
	avg_X = avg( X )
	avg_Y = avg( Y )
	S_XY=0
	for i in range(n):
		S_XY = S_XY + ((X[i]-avg_X)*(Y[i]-avg_Y))

	return (S_XY/n) 


##----------------------------------
## variance : variance 
## param X data vector ( ..x_i.. )
## (S_X)^2 = (Sum ( x_i - avg(x) )^2 ) / n
##----------------------------------
def variance( X ):
	S_X2 = 0
	n = len( X )
	avg_X = avg ( X )
	for i in range(n):
		S_X2 = S_X2 + ((X[i] - avg_X)**2)

	return (S_X2/n)

##-----------------------------------------------------------------------------------------------
## correl_split_weighted : compute regression on each segment and
## return the weigthed sum of correlation coefficients
## param X first data vector (..x_i..)
## param Y second data vector (..x_i..)
## param segments list of pairs (i,j) where i refers to the ith value in X, and  jth value in X
## return (C,[(i1,j1,X[i1],X[j1]), (i2,j2,X[i2],X[j2]), ....]
##    where i1,j1 is the first segment,  c1 the correlation coef on this segment, n1 the number of values
##          i2,j2 is the second segment, c2 the correlation coef on this segment, n2 the number of values
##          ...
##    and C=c1/n1+c2/n2+...
##-----------------------------------------------------------------------------------------------
def correl_split_weighted( X , Y , segments ):
	# expects segments = [(0,i1-1),(i1-1,i2-1),(i2,len-1)] 
	correl = list();
	interv = list();  # regr. line coeffs and range
	glob_corr=0
	sum_nb_val=0
	for (start,stop) in segments:
		sum_nb_val = sum_nb_val + stop - start;
		#if start==stop :
		#	return 0
		S_XY= cov( X [start:stop+1], Y [start:stop+1] )
		S_X2 = variance(  X [start:stop+1] ) 
		S_Y2 = variance(  Y [start:stop+1] )  # to compute correlation
		if S_X2*S_Y2 == 0:
			return (0,[])
		c = S_XY/(sqrt(S_X2)*sqrt(S_Y2)) 
		a = S_XY/S_X2				# regr line coeffs
		b= avg ( Y[start:stop+1] ) - a * avg( X[start:stop+1] )
		print("   range [%d,%d] corr=%f, coeff det=%f [a=%f, b=%f]" % (X[start],X[stop],c,c**2,a, b)) 
		correl.append( (c, stop-start) );   # store correl. coef + number of values (segment length)
		interv.append( (a,b, X[start],X[stop]) );
	
	for (c,l) in correl:
		glob_corr = glob_corr + (l/sum_nb_val)*c  # weighted product of correlation 
		print('-- %f * %f' % (c,l/sum_nb_val))
		
	print("-> glob_corr=%f\n" % glob_corr)
	return (glob_corr,interv);
	



##-----------------------------------------------------------------------------------------------
## correl_split : compute regression on each segment and
## return the product of correlation coefficient
## param X first data vector (..x_i..)
## param Y second data vector (..x_i..)
## param segments list of pairs (i,j) where i refers to the ith value in X, and  jth value in X
## return (C,[(i1,j1,X[i1],X[j1]), (i2,j2,X[i2],X[j2]), ....]
##    where i1,j1 is the first segment,  c1 the correlation coef on this segment,
##          i2,j2 is the second segment, c2 the correlation coef on this segment,
##          ...
##    and C=c1*c2*...
##-----------------------------------------------------------------------------------------------
def correl_split( X , Y , segments ):
	# expects segments = [(0,i1-1),(i1-1,i2-1),(i2,len-1)] 
	correl = list();
	interv = list();  # regr. line coeffs and range
	glob_corr=1
	for (start,stop) in segments:
		#if start==stop :
		#	return 0
		S_XY= cov( X [start:stop+1], Y [start:stop+1] )
		S_X2 = variance(  X [start:stop+1] ) 
		S_Y2 = variance(  Y [start:stop+1] )  # to compute correlation
		if S_X2*S_Y2 == 0:
			return (0,[])
		c = S_XY/(sqrt(S_X2)*sqrt(S_Y2)) 
		a = S_XY/S_X2				# regr line coeffs
		b= avg ( Y[start:stop+1] ) - a * avg( X[start:stop+1] )
		print("   range [%d,%d] corr=%f, coeff det=%f [a=%f, b=%f]" % (X[start],X[stop],c,c**2,a, b)) 
		correl.append( (c, stop-start) );   # store correl. coef + number of values (segment length)
		interv.append( (a,b, X[start],X[stop]) );
	
	for (c,l) in correl:
		glob_corr = glob_corr * c  # product of correlation coeffs
	print("-> glob_corr=%f\n" % glob_corr)
	return (glob_corr,interv);
	


##-----------------------------------------------------------------------------------------------
## main
##-----------------------------------------------------------------------------------------------
sum=0
nblines=0
skampidat = open(sys.argv[1], "r")

timings = []
sizes = []
for line in skampidat:
	l = line.split();
	if line[0] != '#' and len(l)>=3:   # is it a comment ?

## expected format
## ---------------
#count= 8388608  8388608  144916.1       7.6       32  144916.1  143262.0
#("%s %d %d %f %f %d %f %f\n" % (countlbl, count, countn, time, stddev, iter, mini, maxi)
		timings.append (float(l[3]))
		sizes.append(int(l[1]))
		nblines=nblines+1



##----------------------- search for best break points-----------------
## example
## p1=2048  -> p1inx=11  delta=3 -> [8;14]
## 						8 : segments[(0,7),(8,13),(13,..)]
##						....						
## p2=65536 -> p2inx=16  delta=3 -> [13;19]

if len(sys.argv) == 4:

	p1inx = sizes.index( p1 );
	p2inx = sizes.index( p2 );
	max_glob_corr = 0;
	max_p1inx = p1inx
	max_p2inx = p2inx

	## tweak parameters here to extend/reduce search
	search_p1 = 30 		# number of values to search +/- around p1
	search_p2 = 45		# number of values to search +/- around p2
	min_seg_size = 3

	lb1 = max(1, p1inx-search_p1)		
	ub1 = min(p1inx+search_p1,search_p1, p2inx);
	lb2 = max(p1inx,p2inx-search_p2)	# breakpoint +/- delta
	ub2 = min(p2inx+search_p2,len(sizes)-1);    

	print("** evaluating over \n");
	print("interv1:\t %d <--- %d ---> %d" % (sizes[lb1],p1,sizes[ub1]))
	print("rank:   \t (%d)<---(%d)--->(%d)\n" % (lb1,p1inx,ub1))
	print("interv2:\t\t %d <--- %d ---> %d" % (sizes[lb2],p2,sizes[ub2]))
	print("rank:   \t\t(%d)<---(%d)--->(%d)\n" % (lb2,p2inx,ub2))
	for i in range(lb1,ub1+1):
		for j in range(lb2,ub2+1):
			if i<j:		# segments must not overlap
				if i+1 >=min_seg_size and j-i+1 >= min_seg_size and len(sizes)-1-j >= min_seg_size : # not too small segments
					print("** i=%d,j=%d" % (i,j))
					segments = [(0,i),(i,j),(j,len(sizes)-1)]
					(glob_cor, interv) = correl_split( sizes, timings, segments)
					if ( glob_cor > max_glob_corr):
						max_glob_corr = glob_cor
						max_interv = interv

	for (a,b,i,j) in max_interv:
		print("** OPT: [%d .. %d]" % (i,j))
	print("** Product of correl coefs = %f" % (max_glob_corr))

	print("#-------------------- cut here the gnuplot code -----------------------------------------------------------\n");
	preamble='set output "regr.eps"\n\
set terminal postscript eps color\n\
set key left\n\
set xlabel "Each message size in bytes"\n\
set ylabel "Time in us"\n\
set logscale x\n\
set logscale y\n\
set grid'

	print(preamble);
	print('plot "%s" u 3:4:($5) with errorbars title "skampi traces %s",\\' % (sys.argv[1],sys.argv[1]));
	for (a,b,i,j) in max_interv:
		print('"%s" u (%d<=$3 && $3<=%d? $3:0/0):(%f*($3)+%f) w linespoints title "regress. %s-%s bytes",\\' % (sys.argv[1],i,j,a,b,i,j))
	
	print("#-------------------- /cut here the gnuplot code -----------------------------------------------------------\n");


else:
	print('\n** Linear regression on %d values **\n' % (nblines))
	print('\n   sizes=',sizes,'\n\n')
	avg_sizes = avg( sizes )
	avg_timings = avg( timings )
	print("avg_timings=%f, avg_sizes=%f, nblines=%d\n" % (avg_timings,avg_sizes,nblines))

	S_XY= cov( sizes, timings )
	S_X2 = variance(  sizes ) 
	S_Y2 = variance(  timings )  # to compute correlation

	a = S_XY/S_X2;
	correl = S_XY/(sqrt(S_X2)*sqrt(S_Y2))  # corealation coeff (Bravais-Pearson)


	b= avg_timings - a * avg_sizes
	print("[S_XY=%f, S_X2=%f]\n[correlation=%f, coeff det=%f]\n[a=%f, b=%f]\n" % (S_XY, S_X2, correl,correl**2,a, b)) 
	

