#ifndef SIMDAG_SIMDAG_H
#define SIMDAG_SIMDAG_H

#include "simdag/datatypes.h"
#include "xbt/misc.h"

SG_BEGIN_DECL()

/************************** Link handling ***********************************/

/* private (called by SD_environment_create)
SD_link_t    SD_link_create(void *data, const char *name,
double bandwidth, double latency);*/
void*        SD_link_get_data(SD_link_t link);
void         SD_link_set_data(SD_link_t link, void *data);
const char*  SD_link_get_name(SD_link_t link);
double       SD_link_get_capacity(SD_link_t link);
double       SD_link_get_current_bandwidth(SD_link_t link);
double       SD_link_get_current_latency(SD_link_t link);
/* private (called by SD_clean)
void         SD_link_destroy(SD_link_t link);
*/

/************************** Workstation handling ****************************/

/* private (called by SD_environment_create)
SD_workstation_t   SD_workstation_create(void *data, const char *name, double power,
  double available_power);*/
SD_workstation_t   SD_workstation_get_by_name(const char *name);
SD_workstation_t*  SD_workstation_get_list(void);
int                SD_workstation_get_number(void);
void               SD_workstation_set_data(SD_workstation_t workstation, void *data);
void*              SD_workstation_get_data(SD_workstation_t workstation);
const char*        SD_workstation_get_name(SD_workstation_t workstation);
SD_link_t*         SD_workstation_route_get_list(SD_workstation_t src, SD_workstation_t dst);
int                SD_workstation_route_get_size(SD_workstation_t src, SD_workstation_t dst);
double             SD_workstation_get_power(SD_workstation_t workstation);
double             SD_workstation_get_available_power(SD_workstation_t workstation);
/* private (called by SD_clean)
void               SD_workstation_destroy(SD_workstation_t workstation);
*/

/************************** Task handling ************************************/

SD_task_t         SD_task_create(const char *name, void *data, double amount);
void              SD_task_schedule(SD_task_t task, int workstation_nb,
				   const SD_workstation_t *workstation_list, double *computation_amount,
				   double *communication_amount, double rate);
void              SD_task_reset(SD_task_t task);
void*             SD_task_get_data(SD_task_t task);
void              SD_task_set_data(SD_task_t task, void *data);
const char*       SD_task_get_name(SD_task_t task);
double            SD_task_get_amount(SD_task_t task);
double            SD_task_get_remaining_amount(SD_task_t task);
void              SD_task_dependency_add(const char *name, void *data, SD_task_t src, SD_task_t dst);
void              SD_task_dependency_remove(SD_task_t src, SD_task_t dst);
void*             SD_task_dependency_get_data(SD_task_t src, SD_task_t dst);
e_SD_task_state_t SD_task_get_state(SD_task_t task);
/* e_SD_task_state_t can be either SD_SCHEDULED, SD_RUNNING, SD_DONE, or SD_FAILED */

void              SD_task_watch(SD_task_t task, e_SD_task_state_t state);
/* SD_simulate will stop as soon as the state of this task is the one given in argument.
   Watch-point is then automatically removed */

void              SD_task_unwatch(SD_task_t task, e_SD_task_state_t state);
void              SD_task_unschedule(SD_task_t task); /* change state and rerun */
void              SD_task_destroy(SD_task_t task);

/************************** Global *******************************************/

void              SD_init(int *argc, char **argv);
void              SD_create_environment(const char *platform_file);
SD_task_t*        SD_simulate(double how_long); /* returns a NULL-terminated array of SD_task_t whose state has changed */
void              SD_exit(); /* cleans everything */

SG_END_DECL()

#endif
