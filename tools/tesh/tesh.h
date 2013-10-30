/* TESH (Test Shell) -- mini shell specialized in running test units        */

/* Copyright (c) 2007-2011. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef TESH_H
#define TESH_H

#include "portable.h"
#include "xbt/xbt_os_thread.h"
#include "xbt/strbuff.h"

/*** What we need to know about signals ***/
/******************************************/
/* return the name of a signal, aliasing SIGBUS to SIGSEGV since
   segfault leads to any of them depending on the system */
const char *signal_name(unsigned int got, char *expected);

#include "run_context.h"

/*** Options ***/
extern int timeout_value;       /* child timeout value */
extern int sort_len;            /* length of the prefix to sort */
extern char* option;            /* cfg for command line */
#define SORT_LEN_DEFAULT 19
extern int coverage;       /* coverage */

extern rctx_t rctx;
extern const char *testsuite_name;


/* Environment related definitions */
# ifdef __APPLE__
/* under darwin, the environment gets added to the process at startup time. So, it's not defined at library link time, forcing us to extra tricks */
# include <crt_externs.h>
# define environ (*_NSGetEnviron())
# else
/* the environment, as specified by the opengroup, used to initialize the process properties */
extern char **environ;
# endif

extern xbt_dict_t env; /* the environment, stored as a dict (for variable substitution) */

#endif                          /* TESH_H */
