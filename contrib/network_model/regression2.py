#!/usr/bin/python
# This script takes the following command line parameters
# 1) an input file containing 2 columns: message size and 1-way trip time
# 2) the maximum relative error for a line segment
# 3) the minimum number of points needed to justify adding a line segment
# 4) the number of links
# 5) the latency
# 6) the bandwidth

import sys

def compute_regression(points):
    N = len(points)

    if N < 1:
        return None

    if N < 2:
        return (0, points[0][1])

    Sx = Sy = Sxx = Syy = Sxy = 0.0

    for x, y in points:
        Sx  += x
        Sy  += y
        Sxx += x*x
        Syy += y*y
        Sxy += x*y
    denom = Sxx * N - Sx * Sx
    # don't return 0 or negative values as a matter of principle...
    m = max(sys.float_info.min, (Sxy * N - Sy * Sx) / denom)
    b = max(sys.float_info.min, (Sxx * Sy - Sx * Sxy) / denom)
    return (m, b)

def compute_error(m, b, x, y):
    yp = m*x+b
    return abs(yp - y) / max(min(yp, y), sys.float_info.min)

def compute_max_error(m, b, points):
    max_error = 0.0
    for x, y in points:
        max_error = max(max_error, compute_error(m, b, x, y))
    return max_error

def get_max_error_point(m, b, points):
    max_error_index = -1
    max_error = 0.0

    i = 0
    while i < len(points):
        x, y = points[i]
        error = compute_error(m, b, x, y)
        if error > max_error:
            max_error_index = i
            max_error = error
        i += 1

    return (max_error_index, max_error)

infile_name = sys.argv[1]
error_bound = float(sys.argv[2])
min_seg_points = int(sys.argv[3])
links = int(sys.argv[4])
latency = float(sys.argv[5])
bandwidth = float(sys.argv[6])

infile = open(infile_name, 'r')

# read datafile
points = []
for line in infile:
    fields = line.split()
    points.append((int(fields[0]), int(fields[1])))
infile.close()

# should sort points by x values
points.sort()

# break points up into segments
pointsets = []
lbi = 0
while lbi < len(points):
    min_ubi = lbi
    max_ubi = len(points) - 1
    while max_ubi - min_ubi > 1:
        ubi = (min_ubi + max_ubi) / 2
        m, b = compute_regression(points[lbi:ubi+1])
        max_error = compute_max_error(m, b, points[lbi:ubi+1])
        if max_error > error_bound:
            max_ubi = ubi - 1
        else:
            min_ubi = ubi
    ubi = max_ubi
    if min_ubi < max_ubi:
        m, b = compute_regression(points[lbi:max_ubi+1])
        max_error = compute_max_error(m, b, points[lbi:max_ubi+1])
        if max_error > error_bound:
            ubi = min_ubi
    pointsets.append(points[lbi:ubi+1])
    lbi = ubi+1

# try to merge larger segments if possible and compute piecewise regression
i = 0
segments = []
notoutliers = 0
while i < len(pointsets):
    currpointset = []
    j = i
    while j < len(pointsets):
        newpointset = currpointset + pointsets[j] 
        # if joining a small segment, we can delete bad points
        if len(pointsets[j]) < min_seg_points:
            k = 0
            while k < len(pointsets[j]):
                m, b = compute_regression(newpointset)
                max_error_index, max_error = get_max_error_point(m, b, newpointset)
                if max_error <= error_bound:
                    break
                del newpointset[max_error_index]
                k += 1
            # only add new pointset if we had to delete fewer than its length
            # points
            if k < len(pointsets[j]):
                i = j
                currpointset = newpointset   
        # otherwise, we just see if it works...
        else:
            m, b = compute_regression(newpointset)
            max_error = compute_max_error(m, b, newpointset)
            if max_error > error_bound:
                break
            i = j
            currpointset = newpointset   
        j += 1
    i += 1
    # outliers are ignored when constructing the piecewise funciton
    if len(currpointset) < min_seg_points:
        continue
    notoutliers += len(currpointset)
    m, b = compute_regression(currpointset)
    lb = min(x for x, y in currpointset)
    lat_factor = b / (1.0e6 * links * latency)
    bw_factor = 1.0e6 / (m * bandwidth)
    segments.append((lb, m, b, lat_factor, bw_factor))

outliers = len(points) - notoutliers
segments.sort()
segments.reverse()

print "/**--------- <copy/paste C code snippet in surf/network.c> -------------"
print "  * produced by:"
print "  *", " ".join(sys.argv)
print "  * outliers:", outliers
print "  * gnuplot: "
print "    plot \"%s\" using 1:2 with lines title \"data\", \\" % (infile_name)
for lb, m, b, lat_factor, bw_factor in segments:
    print "        (x >= %d) ? %g*x+%g : \\" % (lb, m, b)
print "        1.0 with lines title \"piecewise function\""
print "  *-------------------------------------------------------------------*/"
print
print "static double smpi_bandwidth_factor(double size)\n{\n"
for lb, m, b, lat_factor, bw_factor in segments:
    print "    if (size >= %d) return %g;" % (lb, bw_factor)
print "    return 1.0;\n}\n"
print "static double smpi_latency_factor(double size)\n{\n"
for lb, m, b, lat_factor, bw_factor in segments:
    print "    if (size >= %d) return %g;" % (lb, lat_factor)
print "    return 1.0;\n}\n"
print "/**--------- <copy/paste C code snippet in surf/network.c> -----------*/"
