/* Specific user API allowing to replay traces without context switches */

/* Copyright (c) 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid_config.h"     /* getline */
#include "replay.h"
#include "simix/simix.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(replay,"trace replaying");

replay_init_func_t replay_func_init=NULL;
replay_run_func_t replay_func_run=NULL;
replay_fini_func_t replay_func_fini=NULL;

static int replay_get_PID(void);
static int replay_get_PID(void) {
  /* FIXME: implement it */
  return 0;
}

static int replay_wrapper(int argc, char*argv[]) {
  THROW_UNIMPLEMENTED;
}
/** \brief initialize the replay mechanism */
void SG_replay_init(int *argc, char **argv) {
  factory_initializer_to_use = statem_factory_init;
  xbt_getpid = replay_get_PID;
  SIMIX_global_init(argc, argv);

  /* Restore the default exception handlers: we have no real processes */
  __xbt_running_ctx_fetch = &__xbt_ex_ctx_default;
  __xbt_ex_terminate = &__xbt_ex_terminate_default;

  SIMIX_function_register_default(replay_wrapper);
}
void SG_replay_set_functions(replay_init_func_t init, replay_run_func_t run,replay_fini_func_t fini) {
  replay_func_init =init;
  replay_func_run  =run;
  replay_func_fini =fini;
}

/** \brief Loads a platform and deployment from the given file. Trace must be loaded from deployment */
void SG_replay(const char *environment_file, const char *deploy_file) {
  SIMIX_create_environment(environment_file);
  SIMIX_launch_application(deploy_file);

  SIMIX_run();
}
