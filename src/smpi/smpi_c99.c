/* Copyright (c) 2011. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
  * under the terms of the license (GNU LGPL) which comes with this package. */

#include <stdlib.h>
#include "private.h"

static void smpi_free_static(int status, void* arg) {
   free(arg);
}

void smpi_register_static(void* arg) {

#ifndef APPLE
// FIXME
// On Apple this error occurs:
//	Undefined symbols for architecture x86_64:
//	  "_on_exit", referenced from:
//	      _smpi_register_static in smpi_c99.c.o
   on_exit(&smpi_free_static, arg);
#endif
}
