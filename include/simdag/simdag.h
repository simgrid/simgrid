/* Copyright (c) 2006, 2007, 2008, 2009, 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMDAG_SIMDAG_H
#define SIMDAG_SIMDAG_H

#include "simdag/datatypes.h"
#include "xbt/misc.h"
#include "xbt/dynar.h"
#include "xbt/dict.h"


SG_BEGIN_DECL()

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
XBT_PUBLIC(int) SD_link_get_number(void);
XBT_PUBLIC(void *) SD_link_get_data(SD_link_t link);
XBT_PUBLIC(void) SD_link_set_data(SD_link_t link, void *data);
XBT_PUBLIC(const char *) SD_link_get_name(SD_link_t link);
XBT_PUBLIC(double) SD_link_get_current_bandwidth(SD_link_t link);
XBT_PUBLIC(double) SD_link_get_current_latency(SD_link_t link);
XBT_PUBLIC(e_SD_link_sharing_policy_t) SD_link_get_sharing_policy(SD_link_t
                                                                  link);
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

XBT_PUBLIC(const SD_link_t *) SD_route_get_list(SD_workstation_t src,
                                                SD_workstation_t dst);
XBT_PUBLIC(int) SD_route_get_size(SD_workstation_t src,
                                  SD_workstation_t dst);
XBT_PUBLIC(double) SD_workstation_get_power(SD_workstation_t workstation);
XBT_PUBLIC(double) SD_workstation_get_available_power(SD_workstation_t
                                                      workstation);
XBT_PUBLIC(e_SD_workstation_access_mode_t)
    SD_workstation_get_access_mode(SD_workstation_t workstation);
XBT_PUBLIC(void) SD_workstation_set_access_mode(SD_workstation_t
                                                workstation,
                                                e_SD_workstation_access_mode_t
                                                access_mode);

XBT_PUBLIC(double) SD_workstation_get_computation_time(SD_workstation_t
                                                       workstation,
                                                       double
                                                       computation_amount);
XBT_PUBLIC(double) SD_route_get_current_latency(SD_workstation_t src,
                                                SD_workstation_t dst);
XBT_PUBLIC(double) SD_route_get_current_bandwidth(SD_workstation_t src,
                                                  SD_workstation_t dst);
XBT_PUBLIC(double) SD_route_get_communication_time(SD_workstation_t src,
                                                   SD_workstation_t dst,
                                                   double
                                                   communication_amount);

XBT_PUBLIC(SD_task_t) SD_workstation_get_current_task(SD_workstation_t
                                                      workstation);
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
XBT_PUBLIC(void) SD_task_watch(SD_task_t task, e_SD_task_state_t state);
XBT_PUBLIC(void) SD_task_unwatch(SD_task_t task, e_SD_task_state_t state);
XBT_PUBLIC(double) SD_task_get_amount(SD_task_t task);
XBT_PUBLIC(double) SD_task_get_remaining_amount(SD_task_t task);
XBT_PUBLIC(double) SD_task_get_execution_time(SD_task_t task,
                                              int workstation_nb,
                                              const SD_workstation_t *
                                              workstation_list,
                                              const double
                                              *computation_amount, const double
                                              *communication_amount);
XBT_PUBLIC(int) SD_task_get_kind(SD_task_t task);
XBT_PUBLIC(void) SD_task_schedule(SD_task_t task, int workstation_nb,
                                  const SD_workstation_t *
                                  workstation_list,
                                  const double *computation_amount,
                                  const double *communication_amount,
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
XBT_PUBLIC(SD_task_t) SD_task_create_comm_e2e(const char *name, void *data,
                                              double amount);
XBT_PUBLIC(void) SD_task_schedulev(SD_task_t task, int count,
                                   const SD_workstation_t * list);
XBT_PUBLIC(void) SD_task_schedulel(SD_task_t task, int count, ...);

/** @brief A constant to use in SD_task_schedule to mean that there is no cost.
 *
 *  For example, create a pure computation task (no comm) like this:
 *
 *  SD_task_schedule(task, my_workstation_nb,
 *                   my_workstation_list,
 *                   my_computation_amount,
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
XBT_PUBLIC(void *) SD_task_dependency_get_data(SD_task_t src,
                                               SD_task_t dst);
XBT_PUBLIC(int) SD_task_dependency_exists(SD_task_t src, SD_task_t dst);
/** @} */

/************************** Global *******************************************/

/** @defgroup SD_simulation Simulation
 *  @brief Functions for creating the environment and launching the simulation
 *
 *  This section describes the functions for initialising SimDag, launching
 *  the simulation and exiting SimDag.
 *
 *  @{
 */
XBT_PUBLIC(void) SD_init(int *argc, char **argv);
XBT_PUBLIC(void) SD_application_reinit(void);
XBT_PUBLIC(void) SD_create_environment(const char *platform_file);
XBT_PUBLIC(void) SD_load_environment_script(const char *script_file);
XBT_PUBLIC(xbt_dynar_t) SD_simulate(double how_long);
XBT_PUBLIC(double) SD_get_clock(void);
XBT_PUBLIC(void) SD_exit(void);
XBT_PUBLIC(xbt_dynar_t) SD_daxload(const char *filename);
XBT_PUBLIC(xbt_dynar_t) SD_dotload(const char *filename);
XBT_PUBLIC(void) uniq_transfer_task_name(SD_task_t task);

/** @} */

#include "instr/instr.h"

SG_END_DECL()
#endif
