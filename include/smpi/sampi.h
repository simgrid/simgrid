/* Copyright (c) 2007-2019. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SAMPI_H_
#define SAMPI_H_

#define SAMPI_OVERRIDEN_MALLOC
#include <stdlib.h>
#include <smpi/smpi.h>

#define AMPI_CALL(type, name, args)                                                                                    \
  type _XBT_CONCAT(A, name) args __attribute__((weak));                                                                \
  type _XBT_CONCAT(AP, name) args;

#ifndef HAVE_SMPI
#undef malloc
#undef calloc
#undef free
// Internally disable these overrides (HAVE_SMPI is only defined when building the library)
#define malloc(nbytes) _sampi_malloc(nbytes)
#define calloc(n_elm, elm_size) _sampi_calloc((n_elm), (elm_size))
#define realloc(ptr, nbytes) _sampi_realloc((ptr), (nbytes))
#define free(ptr) _sampi_free(ptr)
#endif

SG_BEGIN_DECL

XBT_PUBLIC void* _sampi_malloc(size_t size);
XBT_PUBLIC void* _sampi_calloc(size_t n_elm, size_t elm_size);
XBT_PUBLIC void* _sampi_realloc(void *ptr, size_t size);
XBT_PUBLIC void _sampi_free(void* ptr);

AMPI_CALL(XBT_PUBLIC int, MPI_Iteration_in, (MPI_Comm comm))
AMPI_CALL(XBT_PUBLIC int, MPI_Iteration_out, (MPI_Comm comm))
AMPI_CALL(XBT_PUBLIC void, MPI_Migrate, (MPI_Comm comm))

SG_END_DECL

#endif
