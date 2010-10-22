#!/usr/bin/env python


import sys
from math import sqrt


if len(sys.argv) < 5:
   print("Usage : %s datafile links latency bandwidth [size...]" % sys.argv[0])
   print("where : links is the number of links between nodes")
   print("        latency is the nominal latency given in the platform file")
   print("        bandwidth is the nominal bandwidth given in the platform file")
   print("        size are segments limites")
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
   for i in xrange(n):
      S_XY += (X[i] - avg_X) * (Y[i] - avg_Y)
   return (S_XY / n)

##----------------------------------
## variance : variance
## param X data vector ( ..x_i.. )
## (S_X)^2 = (Sum ( x_i - avg(x) )^2 ) / n
##----------------------------------
def variance (X):
   n = len(X)
   avg_X = avg (X)
   S_X2 = 0.0
   for i in xrange(n):
      S_X2 += (X[i] - avg_X) ** 2
   return (S_X2 / n)

def calibrate (links, latency, bandwidth, sizes, timings):
   assert len(sizes) == len(timings)
   if len(sizes) < 2:
      return None
   S_XY = cov(sizes, timings)
   S_X2 = variance(sizes)
   a = S_XY / S_X2
   b = avg(timings) - a * avg(sizes)
   return (b * 1e-6) / (latency * links), 1e6 / (a * bandwidth)

##-----------------------------------------------------------------------------------------------
## main
##-----------------------------------------------------------------------------------------------
links = int(sys.argv[2])
latency = float(sys.argv[3])
bandwidth = float(sys.argv[4])
skampidat = open(sys.argv[1], "r")

timings = []
sizes = []
for line in skampidat:
   l = line.split();
   if line[0] != '#' and len(l) >= 3:   # is it a comment ?
      ## expected format
      ## ---------------
      #count= 8388608  8388608  144916.1       7.6       32  144916.1  143262.0
      #("%s %d %d %f %f %d %f %f\n" % (countlbl, count, countn, time, stddev, iter, mini, maxi)
      timings.append(float(l[3]) / links)
      sizes.append(int(l[1]))

## adds message sizes of interest: if values are specified starting from the 6th command line arg 
## and these values are found as message sizes in te log file, add it to the limits list.
## Each of these value si considered a potential inflexion point between two segments.
##
## If no value specified, a single segment is considered from 1st to last message size logged.
limits = []
if len(sys.argv) > 5:
   for i in xrange(5, len(sys.argv)):
      limits += [idx for idx in xrange(len(sizes)) if sizes[idx] == int(sys.argv[i])]
limits.append(len(sizes) - 1)

low = 0
for lim in limits:
   correc = calibrate(links, latency, bandwidth, sizes[low:lim + 1], timings[low:lim + 1])
   if correc:
      print("Segment [%d:%d] -- Latency factor=%g -- Bandwidth factor=%g" % (sizes[low], sizes[lim], correc[0], correc[1]))
   low = lim + 1
