/* Copyright (c) 2016. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */
#include "nas_common.h"

static double start[64], elapsed[64];

/* integer log base two. Return error is argument isn't a power of two or is less than or equal to zero */
int ilog2(int i)
{
  int log2;
  int exp2 = 1;
  if (i <= 0) return(-1);

  for (log2 = 0; log2 < 20; log2++) {
    if (exp2 == i) return(log2);
    exp2 *= 2;
  }
  return(-1);
}

/*  get_info(): Get parameters from command line */
void get_info(int argc, char *argv[], int *nprocsp, char *classp)
{
  if (argc < 3) {
    printf("Usage: %s (%d) nprocs class\n", argv[0], argc);
    exit(1);
  }

  *nprocsp = atoi(argv[1]);
  *classp = *argv[2];
}

/*  check_info(): Make sure command line data is ok for this benchmark */
void check_info(int type, int nprocs, char class)
{
  int logprocs;

  /* check number of processors */
  if (nprocs <= 0) {
    printf("setparams: Number of processors must be greater than zero\n");
    exit(1);
  }
  switch(type) {
  case IS:
    logprocs = ilog2(nprocs);
    if (logprocs < 0) {
      printf("setparams: Number of processors must be a power of two (1,2,4,...) for this benchmark\n");
      exit(1);
    }
    break;
  case EP:
  case DT:
    break;
  default:
    /* never should have gotten this far with a bad name */
    printf("setparams: (Internal Error) Benchmark type %d unknown to this program\n", type);
    exit(1);
  }

  /* check class */
  if (class != 'S' && class != 'W' && class != 'A' && class != 'B' && class != 'C' && class != 'D' && class != 'E') {
    printf("setparams: Unknown benchmark class %c\n", class);
    printf("setparams: Allowed classes are \"S\", \"W\", and \"A\" through \"E\"\n");
    exit(1);
  }

  if (class == 'E' && (type == IS || type == DT)) {
    printf("setparams: Benchmark class %c not defined for IS or DT\n", class);
    exit(1);
  }

  if (class == 'D' && type == IS && nprocs < 4) {
    printf("setparams: IS class D size cannot be run on less than 4 processors\n");
    exit(1);
  }
}

void timer_clear(int n)
{
  elapsed[n] = 0.0;
}

void timer_start(int n)
{
  start[n] = MPI_Wtime();
}

void timer_stop(int n)
{
  elapsed[n] += MPI_Wtime() - start[n];
}

double timer_read(int n)
{
  return elapsed[n];
}

double vranlc(int n, double x, double a, double *y)
{
  int i;
  uint64_t  i246m1=0x00003FFFFFFFFFFF;
  uint64_t  LLx, Lx, La;
  double d2m46;

// This doesn't work, because the compiler does the calculation in 32 bits and overflows. No standard way (without
// f90 stuff) to specifythat the rhs should be done in 64 bit arithmetic.
// parameter(i246m1=2**46-1)

  d2m46=pow(0.5,46);

  Lx = (uint64_t)x;
  La = (uint64_t)a;
  //fprintf(stdout,("================== Vranlc ================");
  //fprintf(stdout,("Before Loop: Lx = " + Lx + ", La = " + La);
  LLx = Lx;
  for (i=0; i< n; i++) {
    Lx   = Lx*La & i246m1 ;
    LLx = Lx;
    y[i] = d2m46 * (double)LLx;
    /*
     if(i == 0) {
       fprintf(stdout,("After loop 0:");
       fprintf(stdout,("Lx = " + Lx + ", La = " + La);
       fprintf(stdout,("d2m46 = " + d2m46);
       fprintf(stdout,("LLX(Lx) = " + LLX.doubleValue());
       fprintf(stdout,("Y[0]" + y[0]);
     }
     */
  }

  x = (double)LLx;
  /*
  fprintf(stdout,("Change: Lx = " + Lx);
  fprintf(stdout,("=============End   Vranlc ================");
   */
  return x;
}

/*
 *    FUNCTION RANDLC (X, A)
 *
 *  This routine returns a uniform pseudorandom double precision number in the
 *  range (0, 1) by using the linear congruential generator
 *
 *  x_{k+1} = a x_k  (mod 2^46)
 *
 *  where 0 < x_k < 2^46 and 0 < a < 2^46.  This scheme generates 2^44 numbers
 *  before repeating.  The argument A is the same as 'a' in the above formula,
 *  and X is the same as x_0.  A and X must be odd double precision integers
 *  in the range (1, 2^46).  The returned value RANDLC is normalized to be
 *  between 0 and 1, i.e. RANDLC = 2^(-46) * x_1.  X is updated to contain
 *  the new seed x_1, so that subsequent calls to RANDLC using the same
 *  arguments will generate a continuous sequence.
 *
 *  This routine should produce the same results on any computer with at least
 *  48 mantissa bits in double precision floating point data.  On Cray systems,
 *  double precision should be disabled.
 *
 *  David H. Bailey     October 26, 1990
 *
 *     IMPLICIT DOUBLE PRECISION (A-H, O-Z)
 *     SAVE KS, R23, R46, T23, T46
 *     DATA KS/0/
 *
 *  If this is the first call to RANDLC, compute R23 = 2 ^ -23, R46 = 2 ^ -46,
 *  T23 = 2 ^ 23, and T46 = 2 ^ 46.  These are computed in loops, rather than
 *  by merely using the ** operator, in order to insure that the results are
 *  exact on all systems.  This code assumes that 0.5D0 is represented exactly.
 */
double randlc(double *X, double*A)
{
  static int        KS=0;
  static double  R23, R46, T23, T46;
  double    T1, T2, T3, T4;
  double    A1, A2;
  double    X1, X2;
  double    Z;
  int       i, j;

  if (KS == 0) {
    R23 = 1.0;
    R46 = 1.0;
    T23 = 1.0;
    T46 = 1.0;

    for (i=1; i<=23; i++) {
      R23 = 0.50 * R23;
      T23 = 2.0 * T23;
    }
    for (i=1; i<=46; i++) {
      R46 = 0.50 * R46;
      T46 = 2.0 * T46;
    }
    KS = 1;
  }

/*  Break A into two parts such that A = 2^23 * A1 + A2 and set X = N.  */
  T1 = R23 * *A;
  j  = T1;
  A1 = j;
  A2 = *A - T23 * A1;

/*  Break X into two parts such that X = 2^23 * X1 + X2, compute
    Z = A1 * X2 + A2 * X1  (mod 2^23), and then X = 2^23 * Z + A2 * X2  (mod 2^46). */
  T1 = R23 * *X;
  j  = T1;
  X1 = j;
  X2 = *X - T23 * X1;
  T1 = A1 * X2 + A2 * X1;

  j  = R23 * T1;
  T2 = j;
  Z = T1 - T23 * T2;
  T3 = T23 * Z + A2 * X2;
  j  = R46 * T3;
  T4 = j;
  *X = T3 - T46 * T4;
  return(R46 * *X);
}

void c_print_results(const char *name, char class, int n1, int n2, int n3, int niter, int nprocs_compiled,
                     int nprocs_total, double t, double mops, const char *optype, int passed_verification)
{
  printf( "\n\n %s Benchmark Completed\n", name );
  printf( " Class           =                        %c\n", class );

  if( n3 == 0 ) {
    long nn = n1;
    if ( n2 != 0 ) nn *= n2;
    printf( " Size            =             %12ld\n", nn );   /* as in IS */
  } else
    printf( " Size            =              %3dx %3dx %3d\n", n1,n2,n3 );

  printf( " Iterations      =             %12d\n", niter );
  printf( " Time in seconds =             %12.2f\n", t );
  printf( " Total processes =             %12d\n", nprocs_total );

  if ( nprocs_compiled != 0 )
    printf( " Compiled procs  =             %12d\n", nprocs_compiled );

  printf( " Mop/s total     =             %12.2f\n", mops );
  printf( " Mop/s/process   =             %12.2f\n", mops/((float) nprocs_total) );
  printf( " Operation type  = %24s\n", optype);

  if( passed_verification )
    printf( " Verification    =               SUCCESSFUL\n" );
  else
    printf( " Verification    =             UNSUCCESSFUL\n" );
}
