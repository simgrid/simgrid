/* 	$Id$	 */

/* Copyright (c) 2004 Arnaud Legrand. All rights reserved.                  */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef _SURF_SURF_H
#define _SURF_SURF_H

#include "xbt/swag.h"
#include "xbt/dynar.h"
#include "xbt/dict.h"

/* Actions and resources are higly connected structures... */
typedef struct surf_action *surf_action_t;
typedef struct surf_resource *surf_resource_t;

/*****************/
/* Action object */
/*****************/
typedef enum {
  SURF_ACTION_READY = 0,	/* Ready        */
  SURF_ACTION_RUNNING,		/* Running      */
  SURF_ACTION_FAILED,		/* Task Failure */
  SURF_ACTION_DONE,		/* Completed    */
  SURF_ACTION_TO_FREE, 		/* Action to free in next cleanup */
  SURF_ACTION_NOT_IN_THE_SYSTEM	/* Not in the system anymore. Why did you ask ? */
} e_surf_action_state_t;

typedef struct surf_action_state {
  xbt_swag_t ready_action_set;
  xbt_swag_t running_action_set;
  xbt_swag_t failed_action_set;
  xbt_swag_t done_action_set;
} s_surf_action_state_t, *surf_action_state_t;

/* Never create s_surf_action_t by yourself !!!! */
/* Use action_new from the corresponding resource */
typedef struct surf_action {
  s_xbt_swag_hookup_t state_hookup;
  xbt_swag_t state_set;
  double cost;			/* cost        */
  double max_duration;		/* max_duration (may fluctuate until
				   the task is completed) */
  double remains;		/* How much of that cost remains to
				 * be done in the currently running task */
  double start;			/* start time  */
  double finish;		/* finish time : this is modified during the run
				 * and fluctuates until the task is completed */
  void *data;			/* for your convenience */
  int using;
  surf_resource_t resource_type;
} s_surf_action_t;

/***************************/
/* Generic resource object */
/***************************/

typedef struct surf_resource_private *surf_resource_private_t;
typedef struct surf_resource_public {
  s_surf_action_state_t states;	/* Any living action on this resource */
  void *(*name_service) (const char *name);
  const char *(*get_resource_name) (void *resource_id);

   e_surf_action_state_t(*action_get_state) (surf_action_t action);
  void (*action_use) (surf_action_t action);
  int  (*action_free) (surf_action_t action);
  void (*action_cancel) (surf_action_t action);
  void (*action_recycle) (surf_action_t action);
  void (*action_change_state) (surf_action_t action,
			       e_surf_action_state_t state);
  void (*action_set_data) (surf_action_t action, void *data);
  void (*suspend) (surf_action_t action);
  void (*resume) (surf_action_t action);
  int (*is_suspended) (surf_action_t action);
  void (*set_max_duration) (surf_action_t action, double duration);
  const char *name;
} s_surf_resource_public_t, *surf_resource_public_t;

typedef struct surf_resource {
  surf_resource_private_t common_private;
  surf_resource_public_t common_public;
} s_surf_resource_t;

/**************************************/
/* Implementations of resource object */
/**************************************/
/* Timer resource */
typedef struct surf_timer_resource_extension_private
*surf_timer_resource_extension_private_t;
typedef struct surf_timer_resource_extension_public {
  void (*set) (double date, void *function, void *arg);
  int (*get)  (void **function, void **arg);
} s_surf_timer_resource_extension_public_t,
  *surf_timer_resource_extension_public_t;

typedef struct surf_timer_resource {
  surf_resource_private_t common_private;
  surf_resource_public_t common_public;
  surf_timer_resource_extension_public_t extension_public;
} s_surf_timer_resource_t, *surf_timer_resource_t;
extern surf_timer_resource_t surf_timer_resource;
void surf_timer_resource_init(const char *filename);

/* Cpu resource */
typedef enum {
  SURF_CPU_ON = 1,		/* Ready        */
  SURF_CPU_OFF = 0		/* Running      */
} e_surf_cpu_state_t;

typedef struct surf_cpu_resource_extension_private
*surf_cpu_resource_extension_private_t;
typedef struct surf_cpu_resource_extension_public {
  surf_action_t(*execute) (void *cpu, double size);
  surf_action_t(*sleep) (void *cpu, double duration);
  e_surf_cpu_state_t(*get_state) (void *cpu);
} s_surf_cpu_resource_extension_public_t,
    *surf_cpu_resource_extension_public_t;

typedef struct surf_cpu_resource {
  surf_resource_private_t common_private;
  surf_resource_public_t common_public;
  surf_cpu_resource_extension_public_t extension_public;
} s_surf_cpu_resource_t, *surf_cpu_resource_t;
extern surf_cpu_resource_t surf_cpu_resource;
void surf_cpu_resource_init_Cas01(const char *filename);

/* Network resource */
typedef struct surf_network_resource_extension_private
*surf_network_resource_extension_private_t;
typedef struct surf_network_resource_extension_public {
  surf_action_t(*communicate) (void *src, void *dst, double size,
			       double max_rate);
} s_surf_network_resource_extension_public_t,
    *surf_network_resource_extension_public_t;

typedef struct surf_network_resource {
  surf_resource_private_t common_private;
  surf_resource_public_t common_public;
  surf_network_resource_extension_public_t extension_public;
} s_surf_network_resource_t, *surf_network_resource_t;

extern surf_network_resource_t surf_network_resource;
void surf_network_resource_init_CM02(const char *filename);

/* Workstation resource */
typedef struct surf_workstation_resource_extension_private
*surf_workstation_resource_extension_private_t;
typedef struct surf_workstation_resource_extension_public {
  surf_action_t(*execute) (void *workstation, double size);
  surf_action_t(*sleep) (void *workstation, double duration);
  e_surf_cpu_state_t(*get_state) (void *workstation);
  surf_action_t(*communicate) (void *workstation_src,
			       void *workstation_dst, double size,
			       double max_rate);
} s_surf_workstation_resource_extension_public_t,
    *surf_workstation_resource_extension_public_t;

typedef struct surf_workstation_resource {
  surf_resource_private_t common_private;
  surf_resource_public_t common_public;
  surf_workstation_resource_extension_public_t extension_public;
} s_surf_workstation_resource_t, *surf_workstation_resource_t;

extern surf_workstation_resource_t surf_workstation_resource;
void surf_workstation_resource_init_CLM03(const char *filename);
void surf_workstation_resource_init_KCCFLN05(const char *filename);
extern xbt_dict_t workstation_set;

/*******************************************/
/*** SURF Globals **************************/
/*******************************************/

void surf_init(int *argc, char **argv);	/* initialize common structures */

extern xbt_dynar_t resource_list;	/* list of initialized resources */

double surf_solve(void);	/*  update all states and returns
				   the time elapsed since last
				   event */
double surf_get_clock(void);
void surf_finalize(void);	/* clean everything */

#endif				/* _SURF_SURF_H */
