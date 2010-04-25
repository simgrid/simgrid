/* chrono - demo of GRAS benchmarking features                              */

/* Copyright (c) 2005 Martin Quinson, Arnaud Legrand. All rights reserved.  */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "gras.h"
#include "xbt/log.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(Chrono,"Messages specific to this example");

#include <cblas.h>
void cblas_dgemm(const enum CBLAS_ORDER Order,
		 const enum CBLAS_TRANSPOSE TransA,
		 const enum CBLAS_TRANSPOSE TransB, const int M,
		 const int N, const int K, const double alpha,
		 const double *A, const int lda, const double *B,
		 const int ldb, const double beta, double *C,
		 const int ldc);


/* Function prototypes */
static int mat_mult(int n)
{
  int i,j,k,l;
  double *A,*B,*C;
  double start = 0.0;
  double now = 0.0;

  A = malloc(n*n*sizeof(double));
  B = malloc(n*n*sizeof(double));
  C = malloc(n*n*sizeof(double));

  start=now=gras_os_time();

  INFO1("Matrix size: %d",n);
/*   INFO1("Before computation: %lg", start); */

  for(l=0; l<40; l++) {
    now=gras_os_time();
    GRAS_BENCH_ONCE_RUN_ONCE_BEGIN();
    for(i=0; i<n; i++)
      for(j=0; j<n; j++) {
	A[i*n+j]=2/n;
	B[i*n+j]=1/n;
	C[i*n+j]=0.0;
      }

    cblas_dgemm(CblasRowMajor,
		CblasNoTrans, CblasNoTrans, n, n, n,
		1.0, A, n, B, n, 0.0, C, n);    
    GRAS_BENCH_ONCE_RUN_ONCE_END();
    now=gras_os_time()-now;
/*     INFO2("Iteration %d : %lg ", l, now); */
  }

  now=gras_os_time()-start;
  INFO1("Duration: %lg ", now);

  free(A);
  free(B);
  free(C);
}

int multiplier (int argc,char *argv[])
{
  gras_init(&argc, argv);

  mat_mult(10);
  mat_mult(20);
  mat_mult(30);
  mat_mult(40);
  mat_mult(50);
  mat_mult(75);
  mat_mult(100);
  mat_mult(300);
  mat_mult(500);
  mat_mult(750);
  mat_mult(1000);

  return 0;
}
