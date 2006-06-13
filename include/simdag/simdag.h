#ifndef SIMDAG_H
#define SIMDAG_H

#include "simdag/datatypes.h"
#include "xbt/misc.h"

SG_BEGIN_DECL()

/************************** Link handling ***********************************/

SG_link_t    SG_link_create(void *data, const char *name,
			  double capacity, double bandwidth, double latency);
void*        SG_link_get_data(SG_link_t link);
void         SG_link_set_data(SG_link_t link, void *data);
const char*  SG_link_get_name(SG_link_t link);
double       SG_link_get_capacity(SG_link_t link);
double       SG_link_get_current_bandwidth(SG_link_t link);
double       SG_link_get_current_latency(SG_link_t link);
void         SG_link_destroy(SG_link_t link);

/************************** Workstation handling ****************************/

SG_workstation_t   SG_workstation_create(void *data, const char *name, double power,
					 double available_power);
SG_workstation_t   SG_workstation_get_by_name(const char *name);
SG_workstation_t*  SG_workstation_get_list(void);
int                SG_workstation_get_number(void);
void               SG_workstation_set_data(SG_workstation_t workstation, void *data);
void*              SG_workstation_get_data(SG_workstation_t workstation);
const char*        SG_workstation_get_name(SG_workstation_t workstation);
SG_link_t*         SG_workstation_route_get_list(SG_workstation_t src, SG_workstation_t dst);
int                SG_workstation_route_get_size(SG_workstation_t src, SG_workstation_t dst);
double             SG_workstation_get_power(SG_workstation_t workstation);
double             SG_workstation_get_available_power(SG_workstation_t workstation);
void               SG_workstation_destroy(SG_workstation_t workstation);

/************************** Task handling ************************************/

SG_task_t         SG_task_create(const char *name, void *data, double amount);
int               SG_task_schedule(SG_task_t task, int workstation_nb,
				   SG_workstation_t **workstation_list, double *computation_amount,
				   double *communication_amount, double rate);

void*             SG_task_get_data(SG_task_t task);
void              SG_task_set_data(SG_task_t task, void *data);
const char*       SG_task_get_name(SG_task_t task);
double            SG_task_get_amount(SG_task_t task);
double            SG_task_get_remaining_amount(SG_task_t task);
void              SG_task_dependency_add(const char *name, void *data, SG_task_t src, SG_task_t dst);
void              SG_task_dependency_remove(SG_task_t src, SG_task_t dst); 
SG_task_state_t   SG_task_get_state(SG_task_t task);
/* SG_task_state_t can be either SG_SCHEDULED, SG_RUNNING, SG_DONE, or SG_FAILED */

void              SG_task_watch(SG_task_t task, SG_task_state_t state);
/* SG_simulate will stop as soon as the state of this task is the one given in argument.
   Watch-point is then automatically removed */

void              SG_task_unwatch(SG_task_t task, SG_task_state_t state);
void              SG_task_unschedule(SG_task_t task); /* change state and rerun */
void              SG_task_destroy(SG_task_t task);

/************************** Global *******************************************/

SG_task_t        *SG_simulate(double how_long); /* returns a NULL-terminated array of SG_task_t whose state has changed */

SG_END_DECL()

#endif
