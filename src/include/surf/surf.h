/* Authors: Arnaud Legrand                                                  */

/* This program is free software; you can redistribute it and/or modify it
   under the terms of the license (GNU LGPL) which comes with this package. */


#ifndef _SURF_SURF_H
#define _SURF_SURF_H

#include "xbt/swag.h"
#include "xbt/heap.h" /* for xbt_heap_float_t only */
#include "surf/maxmin.h" /* for xbt_maxmin_float_t only  */

/* Actions and resources are higly connected structures... */
typedef struct surf_action *surf_action_t;
typedef struct surf_resource *surf_resource_t;

/*****************/
/* Action object */
/*****************/
typedef enum {
  SURF_ACTION_READY = 0,		/* Ready        */
  SURF_ACTION_RUNNING,			/* Running      */
  SURF_ACTION_FAILED,			/* Task Failure */
  SURF_ACTION_DONE,			/* Completed    */
  SURF_ACTION_NOT_IN_THE_SYSTEM		/* Not in the system anymore. Why did you ask ? */
} e_surf_action_state_t;

/* Never create s_surf_action_t by yourself !!!! */
/* Use action_new from the corresponding resource */
typedef struct surf_action {
  s_xbt_swag_hookup_t state_hookup;
  xbt_swag_t state_set;
  xbt_maxmin_float_t cost;	/* cost        */
  xbt_maxmin_float_t remains;	/* How much of that cost remains to
				 * be done in the currently running task */
  xbt_heap_float_t start;	/* start time  */
  xbt_heap_float_t finish;	/* finish time : this is modified during the run
				 * and fluctuates until the task is completed */
  void *callback;		/* for your convenience */
  surf_resource_t resource_type;
} s_surf_action_t;

/***************************/
/* Generic resource object */
/***************************/
typedef struct surf_resource {
  void (*parse_file)(const char *file);
  void *(*name_service)(const char *name);

  surf_action_t (*action_new)(void *callback);
  e_surf_action_state_t (*action_get_state)(surf_action_t action);
  void (*action_free)(surf_action_t * action);	/* Call it when you're done with this action */
  void (*action_cancel)(surf_action_t action); /* remove the variables from the linear system if needed */
  void (*action_recycle)(surf_action_t action);	/* More efficient than free/new */
  void (*action_change_state)(surf_action_t action, e_surf_action_state_t state);

  xbt_heap_float_t (*share_resources)(void); /* Share the resources to the 
						actions and return the potential 
						next action termination */
  void (*solve)(xbt_heap_float_t date); /* Advance time to "date" and update
					   the actions' state*/
} s_surf_resource_t;

/**************************************/
/* Implementations of resource object */
/**************************************/
/* Host resource */
typedef enum {
  SURF_HOST_ON = 1,		/* Ready        */
  SURF_HOST_OFF = 0,		/* Running      */
} e_surf_host_state_t;

typedef struct surf_host_resource {
  s_surf_resource_t resource;
  void (*execute)(void *host, xbt_maxmin_float_t size, surf_action_t action);
  e_surf_host_state_t (*get_state)(void *host);
} s_surf_host_resource_t, *surf_host_resource_t;
extern surf_host_resource_t surf_host_resource;

/* Network resource */
typedef struct surf_network_resource {
  s_surf_resource_t resource;
  surf_action_t (*communicate)(void *src, void *dst, xbt_maxmin_float_t size,
			       surf_action_t action);
} s_surf_network_resource_t, surf_network_resource_t;
extern surf_network_resource_t surf_network_resource;

/* Timer resource */
typedef struct surf_timer_resource {
  s_surf_resource_t resource;
  surf_action_t (*wait)(void *host, void *dst, xbt_maxmin_float_t size,
			surf_action_t surf);
} s_surf_timer_resource_t, surf_timer_resource_t;
extern surf_timer_resource_t surf_timer_resource;

/*******************************************/
/*** SURF Globals **************************/
/*******************************************/
typedef struct surf_global {
  xbt_swag_t ready_action_set;
  xbt_swag_t running_action_set;
  xbt_swag_t failed_action_set;
  xbt_swag_t done_action_set;
} s_surf_global_t;
/* The main function */
extern s_surf_global_t surf_global;

void surf_init(void); /* initialize all resource objects */
xbt_heap_float_t surf_solve(void); /* returns the next date */

#endif				/* _SURF_SURF_H */
