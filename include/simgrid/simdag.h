/* Copyright (c) 2006-2010, 2012-2016. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMDAG_SIMDAG_H
#define SIMDAG_SIMDAG_H

#include "xbt/misc.h"
#include "xbt/dynar.h"
#include "xbt/dict.h"
#include "xbt/asserts.h"
#include "xbt/log.h"
#include "simgrid/link.h"
#include "simgrid/host.h"

SG_BEGIN_DECL()

/** @brief Link opaque datatype
    @ingroup SD_link_api

    A link is a network node represented as a <em>name</em>, a <em>bandwidth</em> and a <em>latency</em>.
    A route is a list of links between two hosts.
 */
typedef Link *SD_link_t;

/** @brief Task opaque datatype
    @ingroup SD_task_api

    A task is some <em>computing amount</em> that can be executed in parallel on several hosts.
    A task may depend on other tasks, which means that the task cannot start until the other tasks are done.
    Each task has a <em>\ref e_SD_task_state_t "state"</em> indicating whether the task is scheduled, running, done, ...

    */
typedef struct SD_task *SD_task_t;

/** @brief Task states
    @ingroup SD_task_api */
typedef enum {
  SD_NOT_SCHEDULED = 0,      /**< @brief Initial state (not valid for SD_watch and SD_unwatch). */
  SD_SCHEDULABLE = 0x0001,   /**< @brief A task becomes SD_SCHEDULABLE as soon as its dependencies are satisfied */
  SD_SCHEDULED = 0x0002,     /**< @brief A task becomes SD_SCHEDULED when you call function
                                  SD_task_schedule. SD_simulate will execute it when it becomes SD_RUNNABLE. */
  SD_RUNNABLE = 0x0004,      /**< @brief A scheduled task becomes runnable is SD_simulate as soon as its dependencies are satisfied. */
  SD_RUNNING = 0x0008,       /**< @brief An SD_RUNNABLE task becomes SD_RUNNING when it is launched. */
  SD_DONE = 0x0010,          /**< @brief The task is successfully finished. */
  SD_FAILED = 0x0020         /**< @brief A problem occurred during the execution of the task. */
} e_SD_task_state_t;

/** @brief Task kinds
    @ingroup SD_task_api */
typedef enum {
  SD_TASK_NOT_TYPED = 0,      /**< @brief no specified type */
  SD_TASK_COMM_E2E = 1,       /**< @brief end to end communication */
  SD_TASK_COMP_SEQ = 2,        /**< @brief sequential computation */
  SD_TASK_COMP_PAR_AMDAHL = 3, /**< @brief parallel computation (Amdahl's law) */
  SD_TASK_COMM_PAR_MXN_1D_BLOCK = 4 /**< @brief MxN data redistribution (1D Block distribution) */
} e_SD_task_kind_t;

/** @brief Storage datatype
    @ingroup SD_storage_api */
typedef xbt_dictelm_t SD_storage_t;

/************************** Workstation handling ****************************/
/** @addtogroup SD_host_api
 *
 *  A host is a place where a task can be executed.
 *  A host is represented as a <em>physical resource with computing capabilities</em> and has a <em>name</em>.
 *
 *  The hosts are created from the description file when you call the function SD_create_environment.
 *
 *  @see sg_host_t
 *  @{
 */
XBT_PUBLIC(SD_link_t *) SD_route_get_list(sg_host_t src, sg_host_t dst);
XBT_PUBLIC(int) SD_route_get_size(sg_host_t src, sg_host_t dst);

XBT_PUBLIC(double) SD_route_get_latency(sg_host_t src, sg_host_t dst);
XBT_PUBLIC(double) SD_route_get_bandwidth(sg_host_t src, sg_host_t dst);

XBT_PUBLIC(const char*) SD_storage_get_host(SD_storage_t storage);
/** @} */

/************************** Task handling ************************************/
/** @defgroup SD_task_api Tasks
 *  @brief Functions for managing the tasks
 *
 *  This section describes the functions for managing the tasks.
 *
 *  A task is some <em>working amount</em> that can be executed in parallel on several hosts.
 *  A task may depend on other tasks, which means that the task cannot start until the other tasks are done.
 *  Each task has a <em>\ref e_SD_task_state_t "state"</em> indicating whether the task is scheduled, running, done, ...
 *
 *  @see SD_task_t, @see SD_task_dependency_api
 *  @{
 */
XBT_PUBLIC(SD_task_t) SD_task_create(const char *name, void *data, double amount);
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
XBT_PUBLIC(double) SD_task_get_execution_time(SD_task_t task, int workstation_nb, const sg_host_t *workstation_list,
                                              const double *flops_amount, const double *bytes_amount);
XBT_PUBLIC(e_SD_task_kind_t) SD_task_get_kind(SD_task_t task);
XBT_PUBLIC(void) SD_task_schedule(SD_task_t task, int workstation_nb, const sg_host_t *workstation_list,
                                  const double *flops_amount, const double *bytes_amount, double rate);
XBT_PUBLIC(void) SD_task_unschedule(SD_task_t task);
XBT_PUBLIC(double) SD_task_get_start_time(SD_task_t task);
XBT_PUBLIC(double) SD_task_get_finish_time(SD_task_t task);
XBT_PUBLIC(xbt_dynar_t) SD_task_get_parents(SD_task_t task);
XBT_PUBLIC(xbt_dynar_t) SD_task_get_children(SD_task_t task);
XBT_PUBLIC(int) SD_task_get_workstation_count(SD_task_t task);
XBT_PUBLIC(sg_host_t *) SD_task_get_workstation_list(SD_task_t task);
XBT_PUBLIC(void) SD_task_destroy(SD_task_t task);
XBT_PUBLIC(void) SD_task_dump(SD_task_t task);
XBT_PUBLIC(void) SD_task_dotty(SD_task_t task, void *out_FILE);

XBT_PUBLIC(SD_task_t) SD_task_create_comp_seq(const char *name, void *data, double amount);
XBT_PUBLIC(SD_task_t) SD_task_create_comp_par_amdahl(const char *name, void *data, double amount, double alpha);
XBT_PUBLIC(SD_task_t) SD_task_create_comm_e2e(const char *name, void *data, double amount);
XBT_PUBLIC(SD_task_t) SD_task_create_comm_par_mxn_1d_block(const char *name, void *data, double amount);

XBT_PUBLIC(void) SD_task_distribute_comp_amdahl(SD_task_t task, int ws_count);
XBT_PUBLIC(void) SD_task_schedulev(SD_task_t task, int count, const sg_host_t * list);
XBT_PUBLIC(void) SD_task_schedulel(SD_task_t task, int count, ...);


/** @brief A constant to use in SD_task_schedule to mean that there is no cost.
 *
 *  For example, create a pure computation task (no comm) like this:
 *
 *  SD_task_schedule(task, my_host_count, my_host_list, my_flops_amount, SD_TASK_SCHED_NO_COST, my_rate);
 */
#define SD_SCHED_NO_COST NULL

/** @} */

/** @addtogroup SD_task_dependency_api 
 * 
 *  This section describes the functions for managing the dependencies between the tasks.
 *
 *  @see SD_task_api
 *  @{
 */
XBT_PUBLIC(void) SD_task_dependency_add(const char *name, void *data, SD_task_t src, SD_task_t dst);
XBT_PUBLIC(void) SD_task_dependency_remove(SD_task_t src, SD_task_t dst);
XBT_PUBLIC(const char *) SD_task_dependency_get_name(SD_task_t src, SD_task_t dst);
XBT_PUBLIC(void *) SD_task_dependency_get_data(SD_task_t src, SD_task_t dst);
XBT_PUBLIC(int) SD_task_dependency_exists(SD_task_t src, SD_task_t dst);
/** @} */

/************************** Global *******************************************/
/** @addtogroup SD_simulation Simulation
 *
 *  This section describes the functions for initializing SimDag, launching the simulation and exiting SimDag.
 *
 *  @{
 */
XBT_PUBLIC(void) SD_init(int *argc, char **argv);
XBT_PUBLIC(void) SD_config(const char *key, const char *value);
XBT_PUBLIC(void) SD_create_environment(const char *platform_file);
XBT_PUBLIC(xbt_dynar_t) SD_simulate(double how_long);
XBT_PUBLIC(double) SD_get_clock(void);
XBT_PUBLIC(void) SD_exit(void);
XBT_PUBLIC(xbt_dynar_t) SD_daxload(const char *filename);
XBT_PUBLIC(xbt_dynar_t) SD_dotload(const char *filename);
XBT_PUBLIC(xbt_dynar_t) SD_dotload_with_sched(const char *filename);
XBT_PUBLIC(xbt_dynar_t) SD_PTG_dotload(const char *filename);

/** @} */

/* Support some backward compatibility */
#define SD_workstation_t sg_host_t

#define SD_link_get_name sg_link_name
#define SD_link_get_current_latency sg_link_latency
#define SD_link_get_current_bandwidth sg_link_bandwidth

#define SD_route_get_current_latency SD_route_get_latency
#define SD_route_get_current_bandwidth SD_route_get_bandwidth

#define SD_workstation_get_list sg_host_list
#define SD_workstation_get_number sg_host_count

#define SD_workstation_get_name sg_host_get_name
#define SD_workstation_get_by_name sg_host_by_name
#define SD_workstation_dump sg_host_dump
#define SD_workstation_get_data sg_host_user
#define SD_workstation_set_data sg_host_user_set
#define SD_workstation_get_properties sg_host_get_properties
#define SD_workstation_get_property_value sg_host_get_property_value
#define SD_workstation_get_power sg_host_speed
#define SD_workstation_get_available_power sg_host_get_available_speed

#define SD_workstation_get_mounted_storage_list sg_host_get_mounted_storage_list
// Lost functions
//SD_workstation_get_access_mode
//SD_workstation_set_access_mode
//SD_workstation_get_current_task
//SD_route_get_communication_time => SG_route_get_latency() + amount / SD_route_get_bandwidth()
//SD_workstation_get_computation_time => amount / sg_host_speed()
//TRACE_sd_set_task_category

SG_END_DECL()
#endif
