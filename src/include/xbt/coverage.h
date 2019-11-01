/* Copyright (c) 2019. The SimGrid Team. All rights reserved.               */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef COVERAGE_H
#define COVERAGE_H

#include <xbt/base.h>

SG_BEGIN_DECL

#ifdef COVERAGE
extern void __gcov_flush();
#define coverage_checkpoint() __gcov_flush()
#else
#define coverage_checkpoint() (void)0
#endif

SG_END_DECL

#endif
