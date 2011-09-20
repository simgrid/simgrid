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
## where bw_regr and lat_regr are the values approximatong experimental
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
   # compute linear regression : find an affine form  a*size+b
   S_XY = cov(sizes, timings)
   S_X2 = variance(sizes)
   a = S_XY / S_X2
   b = avg(timings) - a * avg(sizes)
   # corresponding bandwith, in s (was in us in skampi dat)
   bw_regr = 1e6 / a     
   # corresponding latency, in s (was in us in skampi dat)
   lat_regr = b*1e-6
   #print("\nregression: {0} * x + {1}".format(a,b))
   #print("corr_bw = bw_regr/bandwidth= {0}/{1}={2}     lat_regr/(lat_xml*links)={3}/({4}*{5}))".format(bw_regr,bandwidth,bw_regr/bandwidth,lat_regr,latency,links))
   # return correction factors c_bw,c_lat
   return bw_regr/bandwidth, lat_regr/(latency*links)

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
	# save interval [lb,ub] and correction factors for bw and lat resp.
	factors.append( (sizes[low],sizes[lim], correc[0], correc[1]) )
	print("Segment [%d:%d] --Bandwidth factor=%g --Latency factor=%g " % (sizes[low], sizes[lim], correc[0], correc[1]))
   low = lim + 1

print("\n/**\n *------------------ <copy/paste C code snippet in surf/network.c> ----------------------")
print(" *\n * produced by: {0}\n *".format(' '.join(sys.argv)))
print(" *---------------------------------------------------------------------------------------\n **/")
print("static double smpi_bandwidth_factor(double size)\n{")                                
for (lb,ub,factor_bw,factor_lat) in factors:
	print("\t /* case %d Bytes <= size <=%d Bytes */" % (lb,ub))
	print("\t if (%d <= size && size <=  %d) {" % (lb,ub))
	print("\t	return(%g);" % (factor_bw))
	print("\t }")
print("}\n")  

print("static double smpi_latency_factor(double size)\n{")                                
for (lb,ub,factor_bw,factor_lat) in factors:
	print("\t /* case %d Bytes <= size <=%d Bytes */" % (lb,ub))
	print("\t if (%d <= size && size <=  %d) {" % (lb,ub))
	print("\t	return(%g);" % (factor_lat))
	print("\t }")
print("}\n")  
print("/**\n *------------------ </copy/paste C code snippet in surf/network.c> ---------------------\n **/")
