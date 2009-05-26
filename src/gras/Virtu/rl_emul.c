/* $Id$ */

/* rl_emul - Emulation support (real life)                                  */

/* Copyright (c) 2003-5 Arnaud Legrand, Martin Quinson. All rights reserved.*/

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "gras/emul.h"
#include "gras/Virtu/virtu_rl.h"
#include "gras_modinter.h"

XBT_LOG_NEW_SUBCATEGORY(gras_virtu_emul, gras_virtu, "Emulation support");

/*** Timing macros: nothing to do in RL. Actually do the job and shutup ***/

void gras_emul_init(void)
{
}

void gras_emul_exit(void)
{
}

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
  return 1;
}

int gras_bench_once_end(void)
{
  return 0;
}

/*** Conditional execution support ***/

int gras_if_RL(void)
{
  return 1;
}

int gras_if_SG(void)
{
  return 0;
}
