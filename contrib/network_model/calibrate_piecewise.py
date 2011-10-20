#!/usr/bin/env python


import sys
from math import sqrt


if len(sys.argv) < 5:
   print("Usage : %s datafile links latency bandwidth [size...]" % sys.argv[0])
   print("where : datafile is a SkaMPI pingpong measurement log file"); 
   print("        links is the number of links between nodes")
   print("        latency is the nominal latency given in the platform file")
   print("        bandwidth is the nominal bandwidth given in the platform file")
   print("        size are segments limits")
   sys.exit(-1)

##-----------------------------------------
## avg : return average of a list of values
## param l list of values
##-----------------------------------------
def avg (l):
   sum = 0
   for e in l:
      sum += float(e);
   return sum / len(l)

##-------------------------------------------------
## cov : covariance
## param X first data vector (..x_i..)
## param Y second data vector (..x_i..)
## = 1/n \Sum_{i=1}^n (x_i - avg(x)) * (y_i - avg(y))
##--------------------------------------------------
def cov (X, Y):
   assert len(X) == len(Y)
   n = len(X)   #  n=len(X)=len(Y)
   avg_X = avg(X)
   avg_Y = avg(Y)
   S_XY = 0.0
   for i in range(n):
      S_XY += (X[i] - avg_X) * (Y[i] - avg_Y)
   return (S_XY / n)

##---------------------------------------------------------------------
## variance : variance
## param X data vector ( ..x_i.. )
## (S_X)^2 = (Sum ( x_i - avg(x) )^2 ) / n
##---------------------------------------------------------------------
def variance (X):
   n = len(X)
   avg_X = avg (X)
   S_X2 = 0.0
   for i in range(n):
      S_X2 += (X[i] - avg_X) ** 2
   return (S_X2 / n)

##---------------------------------------------------------------------
## calibrate : output correction factors, c_lat on latency, c_bw on bw
## such that bandwidth * c_bw = bw_regr, latency * c_lat = lat_regr
## where bw_regr and lat_regr are the values approximating experimental
## observations.
##
## param links number of links traversed during ping-pong
## param latency as specified on command line, in s
## param bandwidth as specified on command line, in Byte/s
## param sizes vector of data sizes, in Bytes
## param timings vector of time taken: timings[i] for sizes[i], in us
##---------------------------------------------------------------------
def calibrate (links, latency, bandwidth, sizes, timings):
   assert len(sizes) == len(timings)
   if len(sizes) < 2:
      return None
   # compute linear regression : find an affine form  time = a*size+b
   S_XY = cov(sizes, timings)
   S_X2 = variance(sizes)
   a = S_XY / S_X2
   b = avg(timings) - a * avg(sizes)
   # corresponding bandwith, in byte/s (was in byte/us in skampi dat)
   bw_regr = 1e6 / a     
   # corresponding latency, in s (was in us in skampi dat)
   lat_regr = b*1e-6
   print("\nregression: {0} * x + {1}".format(a,b))
   print("corr_bw = bw_regr/bandwidth= {0}/{1}={2}     lat_regr/(lat_xml*links)={3}/({4}*{5}))".format(bw_regr,bandwidth,bw_regr/bandwidth,lat_regr,latency,links))
   # return linear regression result and corresponding correction factors c_bw,c_lat
   return a,b, bw_regr/bandwidth, lat_regr/(latency*links)


##---------------------------------------------------------------------
## outputs a C formatted conditional return value for factor
##
## param lb lower bound
## param ub upper bound
## param lb_included boolean to tell if bound is included (<=) or exclude (<) 
## param ub_included boolean to tell if bound is included (<=) or exclude (<) 
##---------------------------------------------------------------------
def c_code_print (lb,ub, retval, lb_included, ub_included):
	lb_cmp = ub_cmp = "<"
	if lb_included:
		lb_cmp ="<="
	if ub_included:
		ub_cmp ="<="

	ub_kib=ub/1024.
	lb_kib=lb/1024.
	print("\t /* case {0:.1f} KiB {1} size {2} {3:.1f} KiB */".format(lb_kib,lb_cmp,ub_cmp,ub_kib))
	print("\t if ({0:d} {1}  size && size {2} {3:d}) ".format(lb,lb_cmp,ub_cmp,ub))
	print("\t	return({0});" . format(retval))


##-----------------------------------------------------------------------------------------------
## main
##-----------------------------------------------------------------------------------------------
links = int(sys.argv[2])
latency = float(sys.argv[3])
bandwidth = float(sys.argv[4])
skampidat = open(sys.argv[1], "r")

## read data from skampi logs.
timings = []
sizes = []
readdata =[]
for line in skampidat:
	l = line.split();
	if line[0] != '#' and len(l) >= 3:   # is it a comment ?
      ## expected format
      ## ---------------
      #count= 8388608  8388608  144916.1       7.6       32  144916.1  143262.0
      #("%s %d %d %f %f %d %f %f\n" % (countlbl, count, countn, time, stddev, iter, mini, maxi)
		readdata.append( (int(l[1]),float(l[3]) / 2) );   # divide by 2 because of ping-pong measured

## These may not be sorted so sort it by message size before processing.
sorteddata = sorted( readdata, key=lambda pair: pair[0])
sizes,timings= zip(*sorteddata)


## adds message sizes of interest: if values are specified starting from the 6th command line arg 
## and these values are found as message sizes in te log file, add it to the limits list.
## Each of these value si considered a potential inflexion point between two segments.
##
## If no value specified, a single segment is considered from 1st to last message size logged.
limits = []
if len(sys.argv) > 5:
   for i in range(5, len(sys.argv)):
      limits += [idx for idx in range(len(sizes)) if sizes[idx] == int(sys.argv[i])]
limits.append(len(sizes) - 1)

factors = []
low = 0
for lim in limits:
   correc = calibrate(links, latency, bandwidth, sizes[low:lim + 1], timings[low:lim + 1])
   if correc:
	# save interval [lb,ub] correction, regression line direction and origin
      # and corresponding correction factors for bw and lat resp. 
	(dircoef,origin,factor_bw,factor_lat) = correc
	factors.append( (sizes[low],sizes[lim], dircoef, origin, factor_bw,factor_lat) )
	print("Segment [%d:%d] --Bandwidth factor=%g --Latency factor=%g " % (sizes[low], sizes[lim], factor_bw,factor_lat))
   low = lim + 1

# now computes joining lines between segments
joinseg=[]

print("\n/**\n *------------------ <copy/paste C code snippet in surf/network.c> ----------------------")
print(" *\n * produced by: {0}\n *".format(' '.join(sys.argv)))
print(" *---------------------------------------------------------------------------------------\n **/")

# print correction factor for bandwidth for each segment
print("static double smpi_bandwidth_factor(double size)\n{")                                
for (lb,ub,a,b,factor_bw,factor_lat) in factors:
	c_code_print(lb,ub,factor_bw,True,True)

	# save ends and starts of segments 
	if lb != sizes[0]:
		joinseg.append( (lb,timings[sizes.index(lb)]) )
	if ub != sizes[-1]:
		joinseg.append( (ub,timings[sizes.index(ub)]) )

# print correction factor for bandwidth between segments
joinseg.reverse()
print("\n\t /* ..:: inter-segment corrections ::.. */");
inx=len(joinseg)-1
while inx>=1:
	(x0,y0) = joinseg[inx]
	inx = inx -1
	(x1,y1) = joinseg[inx]
	inx = inx -1
	# line eq. is y = (y1-y0)/(x0-x1) * x +  (y0 x1 - y1 x0)/(x1-x0)
	a = (y1-y0) / (x1-x0)
	bw_join = 1e6 / a
	factor_join_bw = bw_join / bandwidth
	#print("Joining points (%f,%f) -> (%f,%f)  : line dir : a=%g\n" % (x0,y0,x1,y1,a))
	c_code_print(x0,x1,factor_join_bw,False,False)

print("}\n")  

# print correction factor for latency for each segment
print("static double smpi_latency_factor(double size)\n{")                                
for (lb,ub,a,b,factor_bw,factor_lat) in factors:
	c_code_print(lb,ub,factor_lat,True,True)

print("\n\t /* ..:: inter-segment corrections ::.. */");
while joinseg:
	(x0,y0) = joinseg.pop()
	(x1,y1) = joinseg.pop()
	# line eq. is y = (y0-y1)/(x0-x1) * x +  (y0 x1 - y1 x0)/(x1-x0)
	#print("(%f,%f) -> (%f,%f)\n" % (x0,y0,x1,y1))
	b = 1e-6 * (y0*x1-y1*x0) / (x1-x0)
	factor_join_lat = b / (latency*links)
	c_code_print(x0,x1,factor_join_lat,False,False)

print("}\n")  

print("\n/**\n *------------------ <copy/paste C code snippet in surf/network.c> ----------------------\n **/")
