/* Copyright (c) 2006-2010, 2012-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMDAG_SIMDAG_H
#define SIMDAG_SIMDAG_H

#include "xbt/misc.h"
#include "xbt/dynar.h"
#include "xbt/dict.h"

#include "simgrid/link.h"

SG_BEGIN_DECL()
/** @brief Workstation datatype
    @ingroup SD_datatypes_management

    A workstation is a place where a task can be executed.
    A workstation is represented as a <em>physical
    resource with computing capabilities</em> and has a <em>name</em>.

    @see SD_workstation_management */
typedef sg_host_t SD_workstation_t;

/** @brief Workstation access mode
    @ingroup SD_datatypes_management

    By default, a workstation resource is shared, i.e. several tasks
    can be executed at the same time on a workstation. The CPU power of
    the workstation is shared between the running tasks on the workstation.
    In sequential mode, only one task can use the workstation, and the other
    tasks wait in a FIFO.

    @see SD_workstation_get_access_mode(), SD_workstation_set_access_mode() */
typedef enum {
  SD_WORKSTATION_SHARED_ACCESS,     /**< @brief Several tasks can be executed at the same time */
  SD_WORKSTATION_SEQUENTIAL_ACCESS  /**< @brief Only one task can be executed, the others wait in a FIFO. */
} e_SD_workstation_access_mode_t;

/** @brief Link datatype
    @ingroup SD_datatypes_management

    A link is a network node represented as a <em>name</em>, a <em>current
    bandwidth</em> and a <em>current latency</em>. A route is a list of
    links between two workstations.

    @see SD_link_management */
typedef Link *SD_link_t;

/** @brief Task datatype
    @ingroup SD_datatypes_management

    A task is some <em>computing amount</em> that can be executed
    in parallel on several workstations. A task may depend on other
    tasks, this means that the task cannot start until the other tasks are done.
    Each task has a <em>\ref e_SD_task_state_t "state"</em> indicating whether
    the task is scheduled, running, done, etc.

    @see SD_task_management */
typedef struct SD_task *SD_task_t;

/** @brief Task states
    @ingroup SD_datatypes_management

    @see SD_task_management */
typedef enum {
  SD_NOT_SCHEDULED = 0,      /**< @brief Initial state (not valid for SD_watch and SD_unwatch). */
  SD_SCHEDULABLE = 0x0001,               /**< @brief A task becomes SD_SCHEDULABLE as soon as its dependencies are satisfied */
  SD_SCHEDULED = 0x0002,     /**< @brief A task becomes SD_SCHEDULED when you call function
                                  SD_task_schedule. SD_simulate will execute it when it becomes SD_RUNNABLE. */
  SD_RUNNABLE = 0x0004,      /**< @brief A scheduled task becomes runnable is SD_simulate as soon as its dependencies are satisfied. */
  SD_IN_FIFO = 0x0008,       /**< @brief A runnable task can have to wait in a workstation fifo if the workstation is sequential */
  SD_RUNNING = 0x0010,       /**< @brief An SD_RUNNABLE or SD_IN_FIFO becomes SD_RUNNING when it is launched. */
  SD_DONE = 0x0020,          /**< @brief The task is successfully finished. */
  SD_FAILED = 0x0040         /**< @brief A problem occurred during the execution of the task. */
} e_SD_task_state_t;

/** @brief Task kinds
    @ingroup SD_datatypes_management

    @see SD_task_management */
typedef enum {
  SD_TASK_NOT_TYPED = 0,      /**< @brief no specified type */
  SD_TASK_COMM_E2E = 1,       /**< @brief end to end communication */
  SD_TASK_COMP_SEQ = 2,        /**< @brief sequential computation */
  SD_TASK_COMP_PAR_AMDAHL = 3, /**< @brief parallel computation (Amdahl's law) */
  SD_TASK_COMM_PAR_MXN_1D_BLOCK = 4 /**< @brief MxN data redistribution (1D Block distribution) */
} e_SD_task_kind_t;


/** @brief Storage datatype
    @ingroup SD_datatypes_management

 TODO PV: comment it !

    @see SD_storage_management */
typedef xbt_dictelm_t SD_storage_t;

/************************** AS handling *************************************/
XBT_PUBLIC(xbt_dict_t) SD_as_router_get_properties(const char *as);
XBT_PUBLIC(const char*) SD_as_router_get_property_value(const char * as,
                                                  const char *name);

/************************** Link handling ***********************************/
/** @defgroup SD_link_management Links
 *  @brief Functions for managing the network links
 *
 *  This section describes the functions for managing the network links.
 *
 *  A link is a network node represented as a <em>name</em>, a <em>current
 *  bandwidth</em> and a <em>current latency</em>. The links are created
 *  when you call the function SD_create_environment.
 *
 *  @see SD_link_t
 *  @{
 */
XBT_PUBLIC(const SD_link_t *) SD_link_get_list(void);
/** @brief Returns the number of links in the whole platform */
static inline int SD_link_get_number(void) {
  return sg_link_amount();
}

/** @brief Returns the user data of a link */
static inline void *SD_link_get_data(SD_link_t link) {
  return sg_link_data(link);
}

/** @brief Sets the user data of a link
 *
 * The new data can be \c NULL. The old data should have been freed first
 * if it was not \c NULL.
 */
static inline void SD_link_set_data(SD_link_t link, void *data) {
	sg_link_data_set(link, data);
}
/** @brief Returns the name of a link  */
static inline const char *SD_link_get_name(SD_link_t link) {
  return sg_link_name(link);
}
/** @brief Returns the current bandwidth of a link (in bytes per second) */
static inline double SD_link_get_current_bandwidth(SD_link_t link) {
  return sg_link_bandwidth(link);
}
/** @brief Returns the current latency of a link (in seconds) */
static inline double SD_link_get_current_latency(SD_link_t link){
  return sg_link_latency(link);
}
/** @brief Returns the sharing policy of this workstation.
 *  @return true if the link is shared, and false if it's a fatpipe
 */
static inline int SD_link_is_shared(SD_link_t link) {
  return sg_link_is_shared(link);
}
/** @} */

/************************** Workstation handling ****************************/

/** @defgroup SD_workstation_management Workstations
 *  @brief Functions for managing the workstations
 *
 *  This section describes the functions for managing the workstations.
 *
 *  A workstation is a place where a task can be executed.
 *  A workstation is represented as a <em>physical
 *  resource with computing capabilities</em> and has a <em>name</em>.
 *
 *  The workstations are created when you call the function SD_create_environment.
 *
 *  @see SD_workstation_t
 *  @{
 */
XBT_PUBLIC(SD_workstation_t) SD_workstation_get_by_name(const char *name);
XBT_PUBLIC(const SD_workstation_t *) SD_workstation_get_list(void);
XBT_PUBLIC(int) SD_workstation_get_number(void);
XBT_PUBLIC(void) SD_workstation_set_data(SD_workstation_t workstation,
                                         void *data);
XBT_PUBLIC(void *) SD_workstation_get_data(SD_workstation_t workstation);
XBT_PUBLIC(const char *) SD_workstation_get_name(SD_workstation_t
                                                 workstation);
/*property handling functions*/
XBT_PUBLIC(xbt_dict_t) SD_workstation_get_properties(SD_workstation_t
                                                     workstation);
XBT_PUBLIC(const char *) SD_workstation_get_property_value(SD_workstation_t
                                                           workstation,
                                                           const char
                                                           *name);
XBT_PUBLIC(void) SD_workstation_dump(SD_workstation_t ws);
XBT_PUBLIC(const SD_link_t *) SD_route_get_list(SD_workstation_t src,
                                                SD_workstation_t dst);
XBT_PUBLIC(int) SD_route_get_size(SD_workstation_t src,
                                  SD_workstation_t dst);
XBT_PUBLIC(double) SD_workstation_get_power(SD_workstation_t workstation);
XBT_PUBLIC(double) SD_workstation_get_available_power(SD_workstation_t
                                                      workstation);
XBT_PUBLIC(int) SD_workstation_get_cores(SD_workstation_t workstation);
XBT_PUBLIC(e_SD_workstation_access_mode_t)
    SD_workstation_get_access_mode(SD_workstation_t workstation);
XBT_PUBLIC(void) SD_workstation_set_access_mode(SD_workstation_t
                                                workstation,
                                                e_SD_workstation_access_mode_t
                                                access_mode);

XBT_PUBLIC(double) SD_workstation_get_computation_time(SD_workstation_t workstation,
                                                       double flops_amount);
XBT_PUBLIC(double) SD_route_get_current_latency(SD_workstation_t src,
                                                SD_workstation_t dst);
XBT_PUBLIC(double) SD_route_get_current_bandwidth(SD_workstation_t src,
                                                  SD_workstation_t dst);
XBT_PUBLIC(double) SD_route_get_communication_time(SD_workstation_t src,
                                                   SD_workstation_t dst,
                                                   double bytes_amount);

XBT_PUBLIC(SD_task_t) SD_workstation_get_current_task(SD_workstation_t workstation);
XBT_PUBLIC(xbt_dict_t)
    SD_workstation_get_mounted_storage_list(SD_workstation_t workstation);
XBT_PUBLIC(xbt_dynar_t)
    SD_workstation_get_attached_storage_list(SD_workstation_t workstation);
XBT_PUBLIC(const char*) SD_storage_get_host(SD_storage_t storage);
/** @} */

/************************** Task handling ************************************/

/** @defgroup SD_task_management Tasks
 *  @brief Functions for managing the tasks
 *
 *  This section describes the functions for managing the tasks.
 *
 *  A task is some <em>working amount</em> that can be executed
 *  in parallel on several workstations. A task may depend on other
 *  tasks, this means that the task cannot start until the other tasks are done.
 *  Each task has a <em>\ref e_SD_task_state_t "state"</em> indicating whether
 *  the task is scheduled, running, done, etc.
 *
 *  @see SD_task_t, SD_task_dependency_management
 *  @{
 */
XBT_PUBLIC(SD_task_t) SD_task_create(const char *name, void *data,
                                     double amount);
XBT_PUBLIC(void *) SD_task_get_data(SD_task_t task);
XBT_PUBLIC(void) SD_task_set_data(SD_task_t task, void *data);
XBT_PUBLIC(e_SD_task_state_t) SD_task_get_state(SD_task_t task);
XBT_PUBLIC(const char *) SD_task_get_name(SD_task_t task);
XBT_PUBLIC(void) SD_task_set_name(SD_task_t task, const char *name);
XBT_PUBLIC(void) SD_task_set_rate(SD_task_t task, double rate);

XBT_PUBLIC(void) SD_task_watch(SD_task_t task, e_SD_task_state_t state);
XBT_PUBLIC(void) SD_task_unwatch(SD_task_t task, e_SD_task_state_t state);
XBT_PUBLIC(double) SD_task_get_amount(SD_task_t task);
XBT_PUBLIC(void) SD_task_set_amount(SD_task_t task, double amount);
XBT_PUBLIC(double) SD_task_get_alpha(SD_task_t task);
XBT_PUBLIC(double) SD_task_get_remaining_amount(SD_task_t task);
XBT_PUBLIC(double) SD_task_get_execution_time(SD_task_t task,
                                              int workstation_nb,
                                              const SD_workstation_t *
                                              workstation_list,
                                              const double *flops_amount,
											  const double *bytes_amount);
XBT_PUBLIC(int) SD_task_get_kind(SD_task_t task);
XBT_PUBLIC(void) SD_task_schedule(SD_task_t task, int workstation_nb,
                                  const SD_workstation_t *
                                  workstation_list,
                                  const double *flops_amount,
                                  const double *bytes_amount,
                                  double rate);
XBT_PUBLIC(void) SD_task_unschedule(SD_task_t task);
XBT_PUBLIC(double) SD_task_get_start_time(SD_task_t task);
XBT_PUBLIC(double) SD_task_get_finish_time(SD_task_t task);
XBT_PUBLIC(xbt_dynar_t) SD_task_get_parents(SD_task_t task);
XBT_PUBLIC(xbt_dynar_t) SD_task_get_children(SD_task_t task);
XBT_PUBLIC(int) SD_task_get_workstation_count(SD_task_t task);
XBT_PUBLIC(SD_workstation_t *) SD_task_get_workstation_list(SD_task_t
                                                            task);
XBT_PUBLIC(void) SD_task_destroy(SD_task_t task);
XBT_PUBLIC(void) SD_task_dump(SD_task_t task);
XBT_PUBLIC(void) SD_task_dotty(SD_task_t task, void *out_FILE);

XBT_PUBLIC(SD_task_t) SD_task_create_comp_seq(const char *name, void *data,
                                              double amount);
XBT_PUBLIC(SD_task_t) SD_task_create_comp_par_amdahl(const char *name,
                                                     void *data,
                                                     double amount,
                                                     double alpha);
XBT_PUBLIC(SD_task_t) SD_task_create_comm_e2e(const char *name, void *data,
                                              double amount);
XBT_PUBLIC(SD_task_t) SD_task_create_comm_par_mxn_1d_block(const char *name,
                                                           void *data,
                                                           double amount);

XBT_PUBLIC(void) SD_task_distribute_comp_amdahl(SD_task_t task, int ws_count);
XBT_PUBLIC(void) SD_task_schedulev(SD_task_t task, int count,
                                   const SD_workstation_t * list);
XBT_PUBLIC(void) SD_task_schedulel(SD_task_t task, int count, ...);


/** @brief A constant to use in SD_task_schedule to mean that there is no cost.
 *
 *  For example, create a pure computation task (no comm) like this:
 *
 *  SD_task_schedule(task, my_workstation_nb,
 *                   my_workstation_list,
 *                   my_flops_amount,
 *                   SD_TASK_SCHED_NO_COST,
 *                   my_rate);
 */
#define SD_SCHED_NO_COST NULL

/** @} */


/** @defgroup SD_task_dependency_management Tasks dependencies
 *  @brief Functions for managing the task dependencies
 *
 *  This section describes the functions for managing the dependencies between the tasks.
 *
 *  @see SD_task_management
 *  @{
 */
XBT_PUBLIC(void) SD_task_dependency_add(const char *name, void *data,
                                        SD_task_t src, SD_task_t dst);
XBT_PUBLIC(void) SD_task_dependency_remove(SD_task_t src, SD_task_t dst);
XBT_PUBLIC(const char *) SD_task_dependency_get_name(SD_task_t src,
                                                     SD_task_t dst);
XBT_PUBLIC(void *) SD_task_dependency_get_data(SD_task_t src,
                                               SD_task_t dst);
XBT_PUBLIC(int) SD_task_dependency_exists(SD_task_t src, SD_task_t dst);
/** @} */

/************************** Global *******************************************/

/** @defgroup SD_simulation Simulation
 *  @brief Functions for creating the environment and launching the simulation
 *
 *  This section describes the functions for initializing SimDag, launching
 *  the simulation and exiting SimDag.
 *
 *  @{
 */
XBT_PUBLIC(void) SD_init(int *argc, char **argv);
XBT_PUBLIC(void) SD_config(const char *key, const char *value);
XBT_PUBLIC(void) SD_application_reinit(void);
XBT_PUBLIC(void) SD_create_environment(const char *platform_file);
XBT_PUBLIC(xbt_dynar_t) SD_simulate(double how_long);
XBT_PUBLIC(double) SD_get_clock(void);
XBT_PUBLIC(void) SD_exit(void);
XBT_PUBLIC(xbt_dynar_t) SD_daxload(const char *filename);
XBT_PUBLIC(xbt_dynar_t) SD_dotload(const char *filename);
XBT_PUBLIC(xbt_dynar_t) SD_PTG_dotload(const char *filename);
XBT_PUBLIC(xbt_dynar_t) SD_dotload_with_sched(const char *filename);
XBT_PUBLIC(void) uniq_transfer_task_name(SD_task_t task);

/** @} */

SG_END_DECL()

#include "simgrid/instr.h"

#endif
