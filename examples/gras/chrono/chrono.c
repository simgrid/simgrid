/* $Id$ */

/* chrono - demo of GRAS benchmarking features                              */

/* Copyright (c) 2005 Martin Quinson, Arnaud Legrand. All rights reserved.  */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "gras.h"
#include "xbt/log.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(Chrono,"Messages specific to this example");


/* Function prototypes */
int multiplier (int argc,char *argv[]);

int multiplier (int argc,char *argv[])
{
  int i,j,k,l;
  double *A,*B,*C;
  int n = 500;
  double start = 0.0;

  gras_init(&argc, argv, NULL);

  A = malloc(n*n*sizeof(double));
  B = malloc(n*n*sizeof(double));
  C = malloc(n*n*sizeof(double));

  INFO1("Before computation: %lg", start=gras_os_time());

  for(l=0; l<4; l++) {
    GRAS_BENCH_ONCE_RUN_ONCE_BEGIN();
    for(i=0; i<n; i++)
      for(j=0; j<n; j++) {
	A[i*n+j]=2/n;
	B[i*n+j]=1/n;
	C[i*n+j]=0.0;
      }
    
    for(i=0; i<n; i++)
      for(j=0; j<n; j++)
	for(k=0; k<n; k++)	
	  C[i*n+j] += A[i*n+k]*B[k*n+j];
    
    GRAS_BENCH_ONCE_RUN_ONCE_END();
  }

  start = gras_os_time()-start;
  INFO2("After computation: %lg; Duration: %lg ", gras_os_time(), start);

  return 0;
}
