/* Specific user API allowing to replay traces without context switches */

/* Copyright (c) 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef TRACE_REPLAY_H_
#define TRACE_REPLAY_H_

#include "xbt/dynar.h"
#include "simix/simix.h"
#include "simix/context.h"

/* function used in each process */
int replay_runner(int argc, char *argv[]);
/* initialize my factory */
void statem_factory_init(smx_context_factory_t * factory);

void SG_replay_init(int *argc, char **argv);

typedef void *(*replay_init_func_t)(int argc, char *argv[]);
typedef void (*replay_run_func_t)(void* data,void *syscall_result);
typedef void (*replay_fini_func_t)(void* data);

replay_init_func_t replay_func_init;
replay_run_func_t replay_func_run;
replay_fini_func_t replay_func_fini;
void SG_replay_set_functions(replay_init_func_t init, replay_run_func_t run,replay_fini_func_t fini);

void SG_replay(const char *environment_file, const char *deploy_file);


/* Trace parsing logic */
typedef struct s_replay_trace_reader *replay_trace_reader_t;

replay_trace_reader_t replay_trace_reader_new(const char*filename);
const char * const *replay_trace_reader_get(replay_trace_reader_t reader);
void replay_trace_reader_free(replay_trace_reader_t *reader);
const char *replay_trace_reader_position(replay_trace_reader_t reader);


#endif /* TRACE_REPLAY_H_ */
