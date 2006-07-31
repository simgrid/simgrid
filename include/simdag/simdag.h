#ifndef SIMDAG_SIMDAG_H
#define SIMDAG_SIMDAG_H

#include "simdag/datatypes.h"
#include "xbt/misc.h"

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
const SD_link_t*   SD_link_get_list(void);
int                SD_link_get_number(void);
void*              SD_link_get_data(SD_link_t link);
void               SD_link_set_data(SD_link_t link, void *data);
const char*        SD_link_get_name(SD_link_t link);
double             SD_link_get_current_bandwidth(SD_link_t link);
double             SD_link_get_current_latency(SD_link_t link);
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
SD_workstation_t        SD_workstation_get_by_name(const char *name);
const SD_workstation_t* SD_workstation_get_list(void);
int                     SD_workstation_get_number(void);
void                    SD_workstation_set_data(SD_workstation_t workstation, void *data);
void*                   SD_workstation_get_data(SD_workstation_t workstation);
const char*             SD_workstation_get_name(SD_workstation_t workstation);
SD_link_t*              SD_route_get_list(SD_workstation_t src, SD_workstation_t dst);
int                     SD_route_get_size(SD_workstation_t src, SD_workstation_t dst);
double                  SD_workstation_get_power(SD_workstation_t workstation);
double                  SD_workstation_get_available_power(SD_workstation_t workstation);
e_SD_workstation_access_mode_t SD_workstation_get_access_mode(SD_workstation_t workstation);
void                    SD_workstation_set_access_mode(SD_workstation_t workstation,
						       e_SD_workstation_access_mode_t access_mode);

double    SD_workstation_get_computation_time(SD_workstation_t workstation, double computation_amount);
double    SD_route_get_current_latency(SD_workstation_t src, SD_workstation_t dst);
double    SD_route_get_current_bandwidth(SD_workstation_t src, SD_workstation_t dst);
double    SD_route_get_communication_time(SD_workstation_t src, SD_workstation_t dst,
						      double communication_amount);

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
SD_task_t         SD_task_create(const char *name, void *data, double amount);
void*             SD_task_get_data(SD_task_t task);
void              SD_task_set_data(SD_task_t task, void *data);
e_SD_task_state_t SD_task_get_state(SD_task_t task);
const char*       SD_task_get_name(SD_task_t task);
void              SD_task_watch(SD_task_t task, e_SD_task_state_t state);
void              SD_task_unwatch(SD_task_t task, e_SD_task_state_t state);
double            SD_task_get_amount(SD_task_t task);
double            SD_task_get_remaining_amount(SD_task_t task);
double            SD_task_get_execution_time(SD_task_t task, int workstation_nb,
					     const SD_workstation_t *workstation_list,
					     const double *computation_amount, const double *communication_amount,
					     double rate);
void              SD_task_schedule(SD_task_t task, int workstation_nb,
				   const SD_workstation_t *workstation_list, const double *computation_amount,
				   const double *communication_amount, double rate);
void              SD_task_unschedule(SD_task_t task);
double            SD_task_get_start_time(SD_task_t task);
double            SD_task_get_finish_time(SD_task_t task);
void              SD_task_destroy(SD_task_t task);
/** @} */


/** @defgroup SD_task_dependency_management Tasks dependencies
 *  @brief Functions for managing the task dependencies
 *
 *  This section describes the functions for managing the dependencies between the tasks.
 *
 *  @see SD_task_management
 *  @{
 */
void              SD_task_dependency_add(const char *name, void *data, SD_task_t src, SD_task_t dst);
void              SD_task_dependency_remove(SD_task_t src, SD_task_t dst);
void*             SD_task_dependency_get_data(SD_task_t src, SD_task_t dst);
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
void              SD_init(int *argc, char **argv);
void              SD_create_environment(const char *platform_file);
SD_task_t*        SD_simulate(double how_long);
double            SD_get_clock(void);
void              SD_exit(void);
/** @} */

SG_END_DECL()

#endif
