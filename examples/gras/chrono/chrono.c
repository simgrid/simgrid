/* $Id$ */

/* chrono - demo of GRAS benchmarking features                              */

/* Copyright (c) 2005 Martin Quinson, Arnaud Legrand. All rights reserved.  */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "gras.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(Chrono,"Messages specific to this example");


/* Function prototypes */
int multiplier (int argc,char *argv[]);

int multiplier (int argc,char *argv[])
{
  int i,j,k;
  double *A,*B,*C;
  int n = 500;
  gras_init(&argc, argv, NULL);
  
  A = malloc(n*n*sizeof(double));
  B = malloc(n*n*sizeof(double));
  C = malloc(n*n*sizeof(double));

  INFO1("Before computation : %lg", gras_os_time());
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

  INFO1("After computation : %lg", gras_os_time());

  return 0;
}
