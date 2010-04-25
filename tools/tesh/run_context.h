/* run_context -- stuff in which TESH runs a command                        */

/* Copyright (c) 2007, 2008, 2009, 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef TESH_RUN_CONTEXT_H
#define TESH_RUN_CONTEXT_H

#include "tesh.h"

typedef enum { e_output_check, e_output_display,
  e_output_ignore
} e_output_handling_t;


typedef struct {
  /* kind of job */
  char *cmd;
  char **env;
  int env_size;
  char *filepos;
  int pid;
  int is_background:1;
  int is_empty:1;
  int is_stoppable:1;

  int brokenpipe:1;
  int timeout:1;

  int reader_done:1;            /* reader set this to true when he's done because
                                   the child is dead. The main thread use it to detect
                                   that the child is not dead before the end of timeout */

  int interrupted:1;            /* Whether we got stopped by an armageddon */
  xbt_os_mutex_t interruption;  /* To allow main thread to kill a runner
                                   one only at certain points */

  e_output_handling_t output;

  int status;

  /* expected results */
  int end_time;                 /* begin_time + timeout, as epoch */
  char *expected_signal;        /* name of signal to raise (or NULL if none) */
  int expected_return;          /* the exepeted return code of following command */

  /* buffers */
  xbt_strbuff_t input;
  xbt_strbuff_t output_wanted;
  xbt_strbuff_t output_got;

  /* Threads */
  xbt_os_thread_t writer, reader;       /* IO handlers */
  xbt_os_thread_t runner;       /* Main thread, counting for timeouts */

  /* Pipes from/to the child */
  int child_to, child_from;

} s_rctx_t, *rctx_t;

/* module mgmt */
void rctx_init(void);
void rctx_exit(void);

/* wait for all currently running background jobs */
void rctx_wait_bg(void);

/* kill forcefully all currently running background jobs */
extern rctx_t armageddon_initiator;
extern xbt_os_mutex_t armageddon_mutex;
void rctx_armageddon(rctx_t initiator, int exitcode);


/* create a new empty running context */
rctx_t rctx_new(void);
void rctx_free(rctx_t rctx);
void rctx_empty(rctx_t rc);     /*reset to empty */
void rctx_dump(rctx_t rctx, const char *str);


/* Launch the current command */
void rctx_start(void);
/* Wait till the end of this command */
void *rctx_wait(void *rctx);

/* Parse a line comming from the suite file, and add this into the rctx */
void rctx_pushline(const char *filepos, char kind, char *line);

#endif /* TESH_RUN_CONTEXT_H */
