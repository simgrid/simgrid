/* Copyright (c) 2013-2020. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/* Copy to src/include/xbt/ folder  */

/* Injecting timings for previously benchmarked code blocks */

/* Use functions from bench.h to benchmark execution time of the desired block,
 * then Rhist.R script to read all timings and produce histograms
 * and finally inject.h to inject values instead of executing block*/

#ifndef __INJECT_H__
#define __INJECT_H__

#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "xbt/RngStream.h"
#include "xbt/dict.h"
#include "xbt/sysdep.h"

#define MAX_LINE_INJ 10000

/*
 * Histogram entry for each measured block
 * Each entry is guarded inside xbt dictionary which is read from the file */
typedef struct xbt_hist {
  int n;
  int counts;
  double mean;
  double* breaks;
  double* percentage;
  char* block_id;
} xbt_hist_t;

extern RngStream get_randgen(void);
typedef RngStream (*get_randgen_func_t)(void);

extern xbt_dict_t get_dict(void);
typedef xbt_dict_t (*get_dict_func_t)(void);

static inline void xbt_inject_init(char *inputfile);
static inline void inject_init_starpu(char *inputfile, xbt_dict_t *dict, RngStream *rng);

static inline double xbt_inject_time(char *key);
static inline double xbt_mean_time(char *key);
static inline double xbt_hist_time(char *key);

/* Initializing xbt dictionary for SMPI version, reading xbt_hist_t entries line by line */
static inline void xbt_inject_init(char *inputfile)
{
  xbt_dict_t mydict = get_dict();
  FILE* fpInput     = fopen(inputfile, "r");
  xbt_assert(fpInput != NULL, "Error while opening the inputfile");
  fseek(fpInput, 0, 0);

  char line[200];
  char* key;

  if (fgets(line, 200, fpInput) == NULL)
    printf("Error input file is empty!"); // Skipping first row
  while (fgets(line, 200, fpInput) != NULL) {
    char *saveptr = NULL; /* for strtok_r() */
    key = strtok_r(line, "\t", &saveptr);

    xbt_hist_t* data = xbt_dict_get_or_null(mydict, key);
    if (data)
      printf("Error, data with that block_id already exists!");

    data = (xbt_hist_t*)xbt_new(xbt_hist_t, 1);

    data->block_id = key;
    data->counts   = atoi(strtok_r(NULL, "\t", &saveptr));
    data->mean     = atof(strtok_r(NULL, "\t", &saveptr));
    data->n        = atoi(strtok_r(NULL, "\t", &saveptr));

    data->breaks     = (double*)malloc(sizeof(double) * data->n);
    data->percentage = (double*)malloc(sizeof(double) * (data->n - 1));
    for (int i        = 0; i < data->n; i++)
      data->breaks[i] = atof(strtok_r(NULL, "\t", &saveptr));
    for (int i            = 0; i < (data->n - 1); i++)
      data->percentage[i] = atof(strtok_r(NULL, "\t", &saveptr));

    xbt_dict_set(mydict, key, data);
  }
  fclose(fpInput);
}

/* Initializing xbt dictionary for StarPU version, reading xbt_hist_t entries line by line */
static inline void inject_init_starpu(char *inputfile, xbt_dict_t *dict, RngStream *rng)
{
  *dict                = xbt_dict_new_homogeneous(free);
  *rng                 = RngStream_CreateStream("Randgen1");
  unsigned long seed[] = {134, 233445, 865, 2634, 424242, 876541};
  RngStream_SetSeed(*rng, seed);

  xbt_dict_t mydict = *dict;
  FILE* fpInput     = fopen(inputfile, "r");
  if (fpInput == NULL) {
    printf("Error while opening the inputfile");
    return;
  }

  fseek(fpInput, 0, 0);

  char line[MAX_LINE_INJ];
  char* key;

  if (fgets(line, MAX_LINE_INJ, fpInput) == NULL) {
    printf("Error input file is empty!"); // Skipping first row
    fclose(fpInput);
    return;
  }

  while (fgets(line, MAX_LINE_INJ, fpInput) != NULL) {
    char *saveptr = NULL; /* for strtok_r() */
    key = strtok_r(line, "\t", &saveptr);

    xbt_hist_t* data = xbt_dict_get_or_null(mydict, key);
    if (data)
      printf("Error, data with that block_id already exists!");

    data             = (xbt_hist_t*)xbt_new(xbt_hist_t, 1);
    data->block_id   = key;
    data->counts     = atoi(strtok_r(NULL, "\t", &saveptr));
    data->mean       = atof(strtok_r(NULL, "\t", &saveptr));
    data->n          = atoi(strtok_r(NULL, "\t", &saveptr));
    data->breaks     = (double*)malloc(sizeof(double) * data->n);
    data->percentage = (double*)malloc(sizeof(double) * (data->n - 1));

    for (int i        = 0; i < data->n; i++)
      data->breaks[i] = atof(strtok_r(NULL, "\t", &saveptr));
    for (int i = 0; i < (data->n - 1); i++) {
      data->percentage[i] = atof(strtok_r(NULL, "\t", &saveptr));
    }

    xbt_dict_set(mydict, key, data);
  }
  fclose(fpInput);
}

/* Injecting time */
static inline double xbt_inject_time(char *key)
{
  return xbt_hist_time(key);
  // return xbt_mean_time(key);
}

/* Injecting mean value */
static inline double xbt_mean_time(char *key)
{
  xbt_dict_t mydict = get_dict();
  xbt_hist_t* data  = xbt_dict_get_or_null(mydict, key);

  if (!data) {
    printf("Warning: element with specified key does not exist (%s)\n", key);
    return 0;
  }

  return data->mean;
}

/* Injecting random value from the histogram */
static inline double xbt_hist_time(char *key)
{
  xbt_dict_t mydict = get_dict();
  xbt_hist_t* data  = xbt_dict_get_or_null(mydict, key);

  if (!data) {
    printf("Warning: element with specified key does not exist (%s)\n", key);
    return 0;
  }

  /* Choosing random interval of the histogram */
  RngStream rng_stream = get_randgen();
  double r             = RngStream_RandU01(rng_stream);
  int k                = 0;
  double left          = 0;
  double right         = 1;
  for (int i = 0; i < (data->n - 1); i++) {
    left += (i == 0) ? 0 : data->percentage[i - 1];
    right += data->percentage[i];
    if (left < r && r <= right)
      k = i;
  }

  /* Choosing random value inside the interval of the histogram */
  double r2    = RngStream_RandU01(rng_stream);
  double timer = data->breaks[k] + r2 * (data->breaks[k + 1] - data->breaks[k]);

  return timer;
}

#endif // __INJECT_H__

