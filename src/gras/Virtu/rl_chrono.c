/* 	$Id$	 */

/* rl_chrono.c - code benchmarking for emulation (fake for real life)       */

/* Copyright (c) 2005 Martin Quinson, Arnaud Legrand. All rights reserved.  */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "xbt/sysdep.h"
#include "xbt/dict.h"
#include "gras/chrono.h"
#include "msg/msg.h"
#include "portable.h"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(chrono,gras,"Benchmarking used code");


int gras_bench_always_begin(const char *location, int line)
{
  return 0;
}

int gras_bench_always_end(void)
{
  return 0;
}

int gras_bench_once_begin(const char *location, int line)
{
  return 0;
}

int gras_bench_once_end(void)
{
  return 0;
}

void gras_chrono_init(void)
{
}
