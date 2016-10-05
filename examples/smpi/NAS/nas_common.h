/* Copyright (c) 2016. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */
#ifndef NAS_COMMON_H
#define NAS_COMMON_H

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <math.h>
#include "mpi.h"

enum benchmark_types {IS, DT, EP};

int ilog2(int i);

void timer_clear(int n);
void timer_start(int n);
void timer_stop(int n);
double timer_read(int n);

double vranlc(int n, double x, double a, double *y);
double randlc(double *X, double*A);

void c_print_results(const char *name, char class, int n1, int n2, int n3, int niter, int nprocs_compiled,
                     int nprocs_total, double t, double mops, const char *optype, int passed_verification);

void get_info(int argc, char *argv[], int *nprocsp, char *classp);
void check_info(int type, int nprocs, char class);

#endif
