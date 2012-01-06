/**--------- <copy/paste C code snippet in surf/network.c> -------------
  * produced by:
  * ./regression2.py ./pingpong-in.dat 0.15 30 2 2.4e-5 1.25e8
  * outliers: 66
  * gnuplot: 
    plot "./pingpong-in.dat" using 1:2 with lines title "data", \
        (x >= 65472) ? 0.00850436*x+558.894 : \
        (x >= 15424) ? 0.0114635*x+167.446 : \
        (x >= 9376) ? 0.0136219*x+124.464 : \
        (x >= 5776) ? 0.00735707*x+105.022 : \
        (x >= 3484) ? 0.0103235*x+90.2886 : \
        (x >= 1426) ? 0.0131384*x+77.3159 : \
        (x >= 732) ? 0.0233927*x+93.6146 : \
        (x >= 257) ? 0.0236608*x+93.7637 : \
        (x >= 127) ? 0.0246645*x+94.0447 : \
        (x >= 64) ? 0.037963*x+93.0877 : \
        (x >= 0) ? 2.22507e-308*x+98.0633 : \
        1.0 with lines title "piecewise function"
  *-------------------------------------------------------------------*/

static double smpi_bandwidth_factor(double size)
{

    if (size >= 65472) return 0.940694;
    if (size >= 15424) return 0.697866;
    if (size >= 9376) return 0.58729;
    if (size >= 5776) return 1.08739;
    if (size >= 3484) return 0.77493;
    if (size >= 1426) return 0.608902;
    if (size >= 732) return 0.341987;
    if (size >= 257) return 0.338112;
    if (size >= 127) return 0.324353;
    if (size >= 64) return 0.210731;
    if (size >= 0) return 3.59539e+305;
    return 1.0;
}

static double smpi_latency_factor(double size)
{

    if (size >= 65472) return 11.6436;
    if (size >= 15424) return 3.48845;
    if (size >= 9376) return 2.59299;
    if (size >= 5776) return 2.18796;
    if (size >= 3484) return 1.88101;
    if (size >= 1426) return 1.61075;
    if (size >= 732) return 1.9503;
    if (size >= 257) return 1.95341;
    if (size >= 127) return 1.95926;
    if (size >= 64) return 1.93933;
    if (size >= 0) return 2.04299;
    return 1.0;
}

/**--------- <copy/paste C code snippet in surf/network.c> -----------*/
