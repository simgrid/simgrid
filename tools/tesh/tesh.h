/* $Id$ */

/* TESH (Test Shell) -- mini shell specialized in running test units        */

/* Copyright (c) 2007 Martin Quinson.                                       */
/* All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef TESH_H
#define TESH_H

#include "xbt/xbt_thread.h"
/*** Buffers ***/
/***************/
#include "buff.h"

/*** What we need to know about signals ***/
/******************************************/
/* return the name of a signal, aliasing SIGBUS to SIGSEGV since
   segfault leads to any of them depending on the system */
const char* signal_name(unsigned int got, char *expected);

#include "run_context.h"
  
/*** Options ***/
int timeout_value; /* child timeout value */

rctx_t rctx;
char *testsuite_name;
#endif /* TESH_H */
