/* $Id$ */

/* sg_emul - Emulation support (simulation)                                 */

/* Copyright (c) 2003-5 Arnaud Legrand, Martin Quinson. All rights reserved.*/

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <stdio.h> /* sprintf */
#include "gras/emul.h"
#include "gras_simix/Virtu/gras_simix_virtu_sg.h"
#include "gras_modinter.h"

#include "xbt/xbt_portability.h" /* timers */
#include "xbt/dict.h"
#include "xbt/ex.h"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(gras_virtu_emul,gras_virtu,"Emulation support");

/*** Timing macros ***/
static xbt_os_timer_t timer;
static int benchmarking = 0;
static xbt_dict_t benchmark_set = NULL;
static double reference = .00000000523066250047108838; /* FIXME: we should benchmark host machine to set this; unit=s/flop */
static double duration = 0.0;

static char* locbuf = NULL;
static int locbufsize;

void gras_emul_init(void)
{
  if(!benchmark_set) {
    benchmark_set = xbt_dict_new();
    timer = xbt_os_timer_new();
  }
}

void gras_emul_exit(void) {
  if (locbuf) free(locbuf);
  xbt_dict_free(&benchmark_set);
  xbt_os_timer_free(timer);
}


static void store_in_dict(xbt_dict_t dict, const char *key, double value)
{
  double *ir;

  ir = xbt_dict_get_or_null(dict, key);
  if (!ir) {
    ir = xbt_new0(double,1);
    xbt_dict_set(dict, key, ir, xbt_free_f);
  }
  *ir = value;
}

static double get_from_dict(xbt_dict_t dict, const char *key) {
  double *ir = xbt_dict_get(dict, key);

  return *ir;
}

int gras_bench_always_begin(const char *location,int line)
{
  xbt_assert0(!benchmarking,"Already benchmarking");
  benchmarking = 1;

  if (!timer)
  xbt_os_timer_start(timer);
  return 0;
}

int gras_bench_always_end(void)
{
	smx_action_t act;
	smx_cond_t cond;
	smx_mutex_t mutex;

  xbt_assert0(benchmarking,"Not benchmarking yet");
  benchmarking = 0;
  xbt_os_timer_stop(timer);
  duration = xbt_os_timer_elapsed(timer);

	cond = SIMIX_cond_init();
	mutex = SIMIX_mutex_init();
	
	SIMIX_mutex_lock(mutex);
	act = SIMIX_action_execute(SIMIX_host_self(), (char*) "task", (duration)/reference);
	
	SIMIX_register_action_to_condition(act,cond);
	SIMIX_register_condition_to_action(act,cond);
	SIMIX_cond_wait(cond, mutex);
	
	SIMIX_action_destroy(act);
	SIMIX_mutex_unlock(mutex);

	SIMIX_cond_destroy(cond);
	SIMIX_mutex_destroy(mutex);

  return 0;
}

int gras_bench_once_begin(const char *location,int line) { 
  double *ir = NULL;
  xbt_assert0(!benchmarking,"Already benchmarking");
  benchmarking = 1;

  if (!locbuf || locbufsize < strlen(location) + 64) {
     locbufsize = strlen(location) + 64;
     locbuf = xbt_realloc(locbuf,locbufsize);
  }
  sprintf(locbuf,"%s:%d",location, line);
   
  ir = xbt_dict_get_or_null(benchmark_set, locbuf);
  if(!ir) {
    DEBUG1("%s",locbuf); 
    duration = 1;
    xbt_os_timer_start(timer);
    return 1;
  } else {
    duration = -1.0;
    return 0;
  }
}

int gras_bench_once_end(void)
{
	smx_action_t act;
	smx_cond_t cond;
	smx_mutex_t mutex;

  xbt_assert0(benchmarking,"Not benchmarking yet");
  benchmarking = 0;
  if(duration>0) {
    xbt_os_timer_stop(timer);
    duration = xbt_os_timer_elapsed(timer);
    store_in_dict(benchmark_set, locbuf, duration);
  } else {
    duration = get_from_dict(benchmark_set,locbuf);
  }
  DEBUG2("Simulate the run of a task of %f sec for %s",duration,locbuf);
	cond = SIMIX_cond_init();
	mutex = SIMIX_mutex_init();
	
	SIMIX_mutex_lock(mutex);
	act = SIMIX_action_execute(SIMIX_host_self(), (char*)"task", (duration)/reference);
	
	SIMIX_register_action_to_condition(act,cond);
	SIMIX_register_condition_to_action(act,cond);
	SIMIX_cond_wait(cond, mutex);
	
	SIMIX_action_destroy(act);
	SIMIX_mutex_unlock(mutex);

	SIMIX_cond_destroy(cond);
	SIMIX_mutex_destroy(mutex);
  return 0;
}


/*** Conditional execution support ***/

int gras_if_RL(void) {
   return 0;
}

int gras_if_SG(void) {
   return 1;
}

void gras_global_init(int *argc,char **argv) {
return SIMIX_global_init(argc,argv);
}
void gras_create_environment(const char *file) {
return SIMIX_create_environment(file);
}
void gras_function_register(const char *name, void *code) {
return SIMIX_function_register(name, (smx_process_code_t)code);
}
void gras_main() {
	smx_cond_t cond = NULL;
	smx_action_t smx_action;
	xbt_fifo_t actions_done = xbt_fifo_new();
	xbt_fifo_t actions_failed = xbt_fifo_new();


	/* Clean IO before the run */
	fflush(stdout);
	fflush(stderr);


	while (SIMIX_solve(actions_done, actions_failed) != -1.0) {

		while ( (smx_action = xbt_fifo_pop(actions_failed)) ) {


			DEBUG1("** %s failed **",smx_action->name);
			while ( (cond = xbt_fifo_pop(smx_action->cond_list)) ) {
				SIMIX_cond_broadcast(cond);
			}
			/* action finished, destroy it */
		//	SIMIX_action_destroy(smx_action);
		}

		while ( (smx_action = xbt_fifo_pop(actions_done)) ) {

			DEBUG1("** %s done **",smx_action->name);
			while ( (cond = xbt_fifo_pop(smx_action->cond_list)) ) {
				SIMIX_cond_broadcast(cond);
			}
			/* action finished, destroy it */
			//SIMIX_action_destroy(smx_action);
		}
	}
	xbt_fifo_free(actions_failed);
	xbt_fifo_free(actions_done);
  return;

}
void gras_launch_application(const char *file) {
return SIMIX_launch_application(file);
}
void gras_clean() {
return SIMIX_clean();
}

