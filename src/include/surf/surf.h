/* 	$Id$	 */

/* Copyright (c) 2004 Arnaud Legrand. All rights reserved.                  */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef _SURF_SURF_H
#define _SURF_SURF_H

#include "xbt/swag.h"
#include "xbt/dynar.h"
#include "xbt/dict.h"
#include "xbt/misc.h"

SG_BEGIN_DECL()



/* Actions and resources are higly connected structures... */

/** \brief Action datatype
 *  \ingroup SURF_actions
 *  
 * An action is some working amount on a resource.
 * It is represented as a cost, a priority, a duration and a state.
 *
 * \see e_surf_action_state_t
 */
typedef struct surf_action *surf_action_t;

/** \brief Resource datatype
 *  \ingroup SURF_resources
 *  
 *  Generic data structure for a resource. The workstations,
 *  the CPUs and the network links are examples of resources.
 */
typedef struct surf_resource *surf_resource_t;

/** \brief Action structure
 * \ingroup SURF_actions
 *
 *  Never create s_surf_action_t by yourself ! The actions are created
 *  on the fly when you call execute or communicate on a resource.
 *  
 *  \see e_surf_action_state_t
 */
typedef struct surf_action {
  s_xbt_swag_hookup_t state_hookup;
  xbt_swag_t state_set;
  double cost;			/**< cost        */
  double priority;		/**< priority (1.0 by default) */
  double max_duration;		/**< max_duration (may fluctuate until
				   the task is completed) */
  double remains;		/**< How much of that cost remains to
				 * be done in the currently running task */
  double start;			/**< start time  */
  double finish;		/**< finish time : this is modified during the run
				 * and fluctuates until the task is completed */
  void *data;			/**< for your convenience */
  int using;
  surf_resource_t resource_type;
} s_surf_action_t;

/** \brief Action states
 *  \ingroup SURF_actions
 *
 *  Action states.
 *
 *  \see surf_action_t, surf_action_state_t
 */
typedef enum {
  SURF_ACTION_READY = 0,	/**< Ready        */
  SURF_ACTION_RUNNING,		/**< Running      */
  SURF_ACTION_FAILED,		/**< Task Failure */
  SURF_ACTION_DONE,		/**< Completed    */
  SURF_ACTION_TO_FREE, 		/**< Action to free in next cleanup */
  SURF_ACTION_NOT_IN_THE_SYSTEM	/**< Not in the system anymore. Why did you ask ? */
} e_surf_action_state_t;

/** \brief Action state sets
 *  \ingroup SURF_actions
 *
 *  This structure contains some sets of actions.
 *  It provides a fast access to the actions in each state.
 *
 *  \see surf_action_t, e_surf_action_state_t
 */
typedef struct surf_action_state {
  xbt_swag_t ready_action_set;   /**< Actions in state SURF_ACTION_READY */
  xbt_swag_t running_action_set; /**< Actions in state SURF_ACTION_RUNNING */
  xbt_swag_t failed_action_set;  /**< Actions in state SURF_ACTION_FAILED */
  xbt_swag_t done_action_set;    /**< Actions in state SURF_ACTION_DONE */
} s_surf_action_state_t, *surf_action_state_t;

/***************************/
/* Generic resource object */
/***************************/

/** \brief Public data available on all resources
 *  \ingroup SURF_resources
 *
 *  These functions are implemented by all resources.
 */
typedef struct surf_resource_public {
  s_surf_action_state_t states;	/**< Any living action on this resource */
  void *(*name_service) (const char *name); /**< Return a resource given its name */
  const char *(*get_resource_name) (void *resource_id); /**< Return the name of a resource */

  e_surf_action_state_t(*action_get_state) (surf_action_t action); /**< Return the state of an action */
  double (*action_get_start_time) (surf_action_t action); /**< Return the start time of an action */
  double (*action_get_finish_time) (surf_action_t action); /**< Return the finish time of an action */
  void (*action_use) (surf_action_t action); /**< Set an action used */
  int  (*action_free) (surf_action_t action); /**< Free an action */
  void (*action_cancel) (surf_action_t action); /**< Cancel a running action */
  void (*action_recycle) (surf_action_t action); /**< Recycle an action */
  void (*action_change_state) (surf_action_t action, /**< Change an action state*/
			       e_surf_action_state_t state);
  void (*action_set_data) (surf_action_t action, void *data); /**< Set the user data of an action */
  void (*suspend) (surf_action_t action); /**< Suspend an action */
  void (*resume) (surf_action_t action); /**< Resume a suspended action */
  int (*is_suspended) (surf_action_t action); /**< Return whether an action is suspended */
  void (*set_max_duration) (surf_action_t action, double duration); /**< Set the max duration of an action*/
  void (*set_priority) (surf_action_t action, double priority); /**< Set the priority of an action */
  const char *name; /**< Name of this resource */
} s_surf_resource_public_t, *surf_resource_public_t;

/** \brief Private data available on all resources
 *  \ingroup SURF_resources
 */
typedef struct surf_resource_private *surf_resource_private_t;

/** \brief Resource datatype
 *  \ingroup SURF_resources
 *  
 *  Generic data structure for a resource. The workstations,
 *  the CPUs and the network links are examples of resources.
 */
typedef struct surf_resource {
  surf_resource_private_t common_private;
  surf_resource_public_t common_public;
} s_surf_resource_t;

/**************************************/
/* Implementations of resource object */
/**************************************/

/** \brief Timer resource extension public
 * \ingroup SURF_resource
 *
 * Additionnal functions specific to the timer resource
 */
typedef struct surf_timer_resource_extension_public {
  void (*set) (double date, void *function, void *arg);
  int (*get)  (void **function, void **arg);
} s_surf_timer_resource_extension_public_t,
  *surf_timer_resource_extension_public_t;

/** \brief Timer resource
 *  \ingroup SURF_resources
 */
typedef struct surf_timer_resource {
  surf_resource_private_t common_private;
  surf_resource_public_t common_public;
  surf_timer_resource_extension_public_t extension_public;
} s_surf_timer_resource_t, *surf_timer_resource_t;

/** \brief The timer resource
 *  \ingroup SURF_resources
 */
extern surf_timer_resource_t XBT_PUBLIC_DATA surf_timer_resource;

/** \brief Initializes the timer resource
 *  \ingroup SURF_resources
 */
XBT_PUBLIC(void) surf_timer_resource_init(const char *filename);

/* Cpu resource */

/** \brief CPU state
 *  \ingroup SURF_resources
 */
typedef enum {
  SURF_CPU_ON = 1,		/**< Ready        */
  SURF_CPU_OFF = 0		/**< Running      */
} e_surf_cpu_state_t;

/** \brief CPU resource extension public
 *  \ingroup SURF_resources
 *  
 *  Public functions specific to the CPU resource.
 */
typedef struct surf_cpu_resource_extension_public {
  surf_action_t(*execute) (void *cpu, double size);
  surf_action_t(*sleep) (void *cpu, double duration);
  e_surf_cpu_state_t(*get_state) (void *cpu);
  double (*get_speed) (void *cpu, double load);
  double (*get_available_speed) (void *cpu);
} s_surf_cpu_resource_extension_public_t,
    *surf_cpu_resource_extension_public_t;

/** \brief CPU resource datatype
 *  \ingroup SURF_resources
 */
typedef struct surf_cpu_resource {
  surf_resource_private_t common_private;
  surf_resource_public_t common_public;
  surf_cpu_resource_extension_public_t extension_public;
} s_surf_cpu_resource_t, *surf_cpu_resource_t;

/** \brief The CPU resource
 *  \ingroup SURF_resources
 */
extern surf_cpu_resource_t XBT_PUBLIC_DATA surf_cpu_resource;

/** \brief Initializes the CPU resource with the model Cas01
 *  \ingroup SURF_resources
 *
 *  This function is called by surf_workstation_resource_init_CLM03
 *  so you shouldn't have to call it by yourself.
 *
 *  \see surf_workstation_resource_init_CLM03()
 */
XBT_PUBLIC(void) surf_cpu_resource_init_Cas01(const char *filename);

/* Network resource */

/** \brief Network resource extension public
 *  \ingroup SURF_resources
 *
 *  Public functions specific to the network resource
 */
typedef struct surf_network_resource_extension_public {
  surf_action_t(*communicate) (void *src, void *dst, double size,
			       double max_rate);
  const void** (*get_route) (void *src, void *dst);
  int (*get_route_size) (void *src, void *dst);
  const char* (*get_link_name) (const void *link);
  double (*get_link_bandwidth) (const void *link);
  double (*get_link_latency) (const void *link);
} s_surf_network_resource_extension_public_t,
    *surf_network_resource_extension_public_t;

/** \brief Network resource datatype
 *  \ingroup SURF_resources
 */
typedef struct surf_network_resource {
  surf_resource_private_t common_private;
  surf_resource_public_t common_public;
  surf_network_resource_extension_public_t extension_public;
} s_surf_network_resource_t, *surf_network_resource_t;

/** \brief The network resource
 *  \ingroup SURF_resources
 *
 *  When creating a new API on top on SURF, you shouldn't use the
 *  network resource unless you know what you are doing. Only the workstation
 *  resource should be accessed because depending on the platform model,
 *  the network resource can be NULL.
 */
extern surf_network_resource_t XBT_PUBLIC_DATA surf_network_resource;

/** \brief Initializes the platform with the network model CM02
 *  \ingroup SURF_resources
 *  \param filename XML platform file name
 *
 *  This function is called by surf_workstation_resource_init_CLM03
 *  so you shouldn't call it by yourself.
 *
 *  \see surf_workstation_resource_init_CLM03()
 */
XBT_PUBLIC(void) surf_network_resource_init_CM02(const char *filename);

/** \brief Workstation resource extension public
 *  \ingroup SURF_resources
 *
 *  Public functions specific to the workstation resource.
 */
typedef struct surf_workstation_resource_extension_public {
  surf_action_t(*execute) (void *workstation, double size);            /**< Execute a computation amount on a workstation
									and create the corresponding action */
  surf_action_t(*sleep) (void *workstation, double duration);          /**< Make a workstation sleep during a given duration */
  e_surf_cpu_state_t(*get_state) (void *workstation);                  /**< Return the CPU state of a workstation */
  double (*get_speed) (void *workstation, double load);                /**< Return the speed of a workstation */
  double (*get_available_speed) (void *workstation);                   /**< Return tha available speed of a workstation */
  surf_action_t(*communicate) (void *workstation_src,                  /**< Execute a communication amount between two workstations */
			       void *workstation_dst, double size,
			       double max_rate);
  surf_action_t(*execute_parallel_task) (int workstation_nb,           /**< Execute a parallel task on several workstations */
					 void **workstation_list,
					 double *computation_amount,
					 double *communication_amount,
					 double amount,
					 double rate);
  const void** (*get_route) (void *src, void *dst);                    /**< Return the network link list between two workstations */
  int (*get_route_size) (void *src, void *dst);                        /**< Return the route size between two workstations */
  const char* (*get_link_name) (const void *link);                     /**< Return the name of a network link */
  double (*get_link_bandwidth) (const void *link);                     /**< Return the current bandwidth of a network link */
  double (*get_link_latency) (const void *link);                       /**< Return the current latency of a network link */
} s_surf_workstation_resource_extension_public_t,
    *surf_workstation_resource_extension_public_t;

/** \brief Workstation resource datatype.
 *  \ingroup SURF_resources
 *
 */
typedef struct surf_workstation_resource {
  surf_resource_private_t common_private;
  surf_resource_public_t common_public;
  surf_workstation_resource_extension_public_t extension_public;
} s_surf_workstation_resource_t, *surf_workstation_resource_t;

/** \brief The workstation resource
 *  \ingroup SURF_resources
 *
 *  Note that when you create an API on top of SURF,
 *  the workstation resource should be the only one you use
 *  because depending on the platform model, the network resource and the CPU resource
 *  may not exist.
 */
extern surf_workstation_resource_t XBT_PUBLIC_DATA surf_workstation_resource;

/** \brief Initializes the platform with the workstation model CLM03
 *  \ingroup SURF_resources
 *  \param filename XML platform file name
 *
 *  This platform model seperates the workstation resource and the network resource.
 *  The workstation resource will be initialized with the model CLM03, the network
 *  resource with the model CM02 and the CPU resource with the model Cas01.
 *  In future releases, some other network models will be implemented and will be
 *  combined with the workstation model CLM03.
 *
 *  \see surf_workstation_resource_init_KCCFLN05()
 */
XBT_PUBLIC(void) surf_workstation_resource_init_CLM03(const char *filename);

/** \brief Initializes the platform with the model KCCFLN05
 *  \ingroup SURF_resources
 *  \param filename XML platform file name
 *
 *  With this model, the workstations and the network are handled together.
 *  There is no network resource. This platform model is the default one for
 *  MSG and SimDag.
 *
 *  \see surf_workstation_resource_init_CLM03()
 */

XBT_PUBLIC(void) surf_workstation_resource_init_KCCFLN05(const char *filename);


XBT_PUBLIC(void) surf_workstation_resource_init_KCCFLN05_proportionnal(const char *filename);

#ifdef USE_GTNETS
/* For GTNetS*/
XBT_PUBLIC(void) surf_workstation_resource_init_GTNETS(const char *filename);
#endif

/** \brief The network links
 *  \ingroup SURF_resources
 *
 *  This dict contains all network links.
 *
 *  \see workstation_set
 */
extern xbt_dict_t XBT_PUBLIC_DATA network_link_set;

/** \brief The workstations
 *  \ingroup SURF_resources
 *
 *  This dict contains all workstations.
 *
 *  \see network_link_set
 */
extern xbt_dict_t XBT_PUBLIC_DATA workstation_set;

/** \brief List of initialized resources
 *  \ingroup SURF_resources
 */
extern xbt_dynar_t XBT_PUBLIC_DATA resource_list;

/*******************************************/
/*** SURF Globals **************************/
/*******************************************/

/** \brief Initialize SURF
 *  \ingroup SURF_simulation
 *  \param argc argument number
 *  \param argv arguments
 *
 *  This function has to be called to initialize the common structures.
 *  Then you will have to create the environment by calling surf_timer_resource_init()
 *  and surf_workstation_resource_init_CLM03() or surf_workstation_resource_init_KCCFLN05().
 *
 *  \see surf_timer_resource_init(), surf_workstation_resource_init_CLM03(),
 *  surf_workstation_resource_init_KCCFLN05(), surf_exit()
 */
XBT_PUBLIC(void) surf_init(int *argc, char **argv);	/* initialize common structures */

/** \brief Performs a part of the simulation
 *  \ingroup SURF_simulation
 *  \return the elapsed time, or -1.0 if no event could be executed
 *
 *  This function execute all possible events, update the action states
 *  and returns the time elapsed.
 *  When you call execute or communicate on a resource, the corresponding actions
 *  are not executed immediately but only when you call surf_solve.
 *  Note that the returned elapsed time can be zero.
 */
XBT_PUBLIC(double) surf_solve(void);

/** \brief Return the current time
 *  \ingroup SURF_simulation
 *
 *  Return the current time in millisecond.
 */
XBT_PUBLIC(double)surf_get_clock(void);

/** \brief Exit SURF
 *  \ingroup SURF_simulation
 *
 *  Clean everything.
 *
 *  \see surf_init()
 */
XBT_PUBLIC(void) surf_exit(void);

SG_END_DECL()

#endif				/* _SURF_SURF_H */
