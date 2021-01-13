/* Copyright (c) 2013-2021. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/* Copy to src/include/xbt/ folder  */

/* Benchmarking a code block */

/* Use functions from bench.h to benchmark execution time of the desired block,
 * then Rhist.R script to read all timings and produce histograms
 * and finally inject.h to inject values instead of executing block*/

#ifndef __BENCH_H__
#define __BENCH_H__

#include <time.h>
#include <stdio.h>
#include <string.h>

/* Structure that has all benchmarking information for the block*/
typedef struct bench {
	struct timespec start_time;
	struct timespec end_time;
	int benchmarking;
	char block_id[256];
	char suffix[100];
	FILE* output;
}*bench_t;

extern bench_t get_mybench(void);
typedef bench_t (*get_bench_func_t)(void);

/* In order to divide nanoseconds and get result in seconds */
#define BILLION  1000000000L

/* Macros for benchmarking */
#define BENCH_BLOCK(block_id) for(bench_begin_block();bench_end_block(block_id);)
#define BENCH_EXTEND(block_id) xbt_bench_extend(block_id)

static inline void xbt_bench_init(char *tracefile);
static inline void bench_init_starpu(char *tracefile, bench_t *bench);

static inline void bench_begin_block();
static inline int bench_end_block(char* block_id);

static inline void xbt_bench_begin(char* block_id);
static inline int xbt_bench_end(char* block_id);

static inline void xbt_bench_extend(char* block_id);

/* Additional functions in order to manipulate with struct timespec */
static inline void xbt_diff_time(struct timespec* start, struct timespec* end, struct timespec* result_time);
static inline double xbt_get_time(struct timespec* timer);

/* Initializing SMPI benchmarking */
static inline void xbt_bench_init(char *tracefile)
{
	bench_t mybench = get_mybench();
	mybench->output = fopen(tracefile, "a+");
	if (mybench->output == NULL)
		printf("Error while opening the tracefile");
}

/* Initializing StarPU benchmarking */
static inline void bench_init_starpu(char *tracefile, bench_t *bench)
{
  *bench = (bench_t) malloc(sizeof(**bench));
  bench_t mybench = *bench;
  mybench->output = fopen(tracefile, "a+");
  if (mybench->output == NULL)
		printf("Error while opening the tracefile");
}

/* Start benchmarking using macros */
static inline void bench_begin_block()
{
	bench_t mybench = get_mybench();
	clock_gettime(CLOCK_REALTIME, &mybench->start_time);
	mybench->benchmarking = 1; // Only benchmarking once
}

/* End benchmarking using macros */
static inline int bench_end_block(char* block_id)
{
	bench_t mybench = get_mybench();
	if (mybench->benchmarking > 0)
	{
		mybench->benchmarking--;
		return 1;
	}
	else
	{
		clock_gettime(CLOCK_REALTIME, &mybench->end_time);
		struct timespec interval;
		xbt_diff_time(&mybench->start_time, &mybench->end_time, &interval);
		fprintf(mybench->output, "%s %f %f %f\n", block_id, xbt_get_time(&mybench->start_time), xbt_get_time(&mybench->end_time), xbt_get_time(&interval));
		return 0;
	}
}

/* Start SMPI benchmarking */
static inline void xbt_bench_begin(char* block_id)
{
	bench_t mybench = get_mybench();
	if(block_id != NULL)
		strcpy (mybench->block_id, block_id);
	else
		strcpy (mybench->block_id, "");
	clock_gettime(CLOCK_REALTIME, &mybench->start_time);
	mybench->benchmarking = 1; // Only benchmarking once
}

/* End SMPI benchmarking */
static inline int xbt_bench_end(char* block_id)
{
	bench_t mybench = get_mybench();

	clock_gettime(CLOCK_REALTIME, &mybench->end_time);
	struct timespec interval;
	xbt_diff_time(&mybench->start_time, &mybench->end_time, &interval);

	if(mybench->suffix != NULL)
	{
		strcat (mybench->block_id, mybench->suffix);
		strcpy (mybench->suffix, "");
	}
	if(block_id != NULL)
		strcat (mybench->block_id, block_id);
	if(mybench->block_id == NULL)
		strcat (mybench->block_id, "NONAME");

	fprintf(mybench->output, "%s %f %f %f\n", mybench->block_id, xbt_get_time(&mybench->start_time), xbt_get_time(&mybench->end_time), xbt_get_time(&interval));
	return 0;
}

/* Extending the block_id name*/
static inline void xbt_bench_extend(char* block_id)
{
	bench_t mybench = get_mybench();
	strcpy (mybench->suffix, block_id);
}

/* Calculating time difference */
static inline void xbt_diff_time(struct timespec* start, struct timespec* end,	struct timespec* result_time)
{
	if ((end->tv_nsec - start->tv_nsec) < 0)
	{
		result_time->tv_sec = end->tv_sec - start->tv_sec - 1;
		result_time->tv_nsec = (double) BILLION + end->tv_nsec - start->tv_nsec;
	}
	else
	{
		result_time->tv_sec = end->tv_sec - start->tv_sec;
		result_time->tv_nsec = end->tv_nsec - start->tv_nsec;
	}
}

/* Printing time in "double" format */
static inline double xbt_get_time(struct timespec* timer)
{
	return timer->tv_sec + (double) (timer->tv_nsec / (double) BILLION);
}

#endif //__BENCH_H__
