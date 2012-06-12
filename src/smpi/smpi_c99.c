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
#ifndef __APPLE__
  // on_exit is not implemented on Apple.
  // That's fine, the memory won't be released on UNIX process terminaison.
  // This means that valgrind will report it as leaked (but who cares?)
   on_exit(&smpi_free_static, arg);
#endif
}
