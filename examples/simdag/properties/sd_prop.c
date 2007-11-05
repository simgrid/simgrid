/* 	$Id$	 */

#include <stdio.h>
#include <stdlib.h>
#include "simdag/simdag.h"
#include "xbt/ex.h"
#include "xbt/log.h"
#include "xbt/dynar.h"
#include "xbt/dict.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(test,
			     "Property test");

int main(int argc, char **argv) {
  int i;

  /* initialisation of SD */
  SD_init(&argc, argv);

  /*  xbt_log_control_set("sd.thres=debug"); */

  if (argc < 2) {
    INFO1("Usage: %s platform_file", argv[0]);
    INFO1("example: %s sd_platform.xml", argv[0]);
    exit(1);
  }

  /* creation of the environment */
  const char * platform_file = argv[1];
  SD_create_environment(platform_file);

  /* test the estimation functions */
  const SD_workstation_t *workstations = SD_workstation_get_list();
  SD_workstation_t w1 = workstations[0];
  SD_workstation_t w2 = workstations[1];
  SD_workstation_set_access_mode(w2, SD_WORKSTATION_SEQUENTIAL_ACCESS);
  const char *name1 = SD_workstation_get_name(w1);
  const char *name2 = SD_workstation_get_name(w2);

  /* The host properties can be retrived from all interfaces */
  xbt_dict_t props;
  INFO1("Property list for workstation %s", name1);
  /* Get the property list of the workstation 1 */
  props = SD_workstation_get_properties(w1);
  xbt_dict_cursor_t cursor = NULL;
  char *key,*data;

    /* Trying to set a new property */
  xbt_dict_set(props, xbt_strdup("NewProp"), strdup("newValue"), free);

  /* Print the properties of the workstation 1 */
  xbt_dict_foreach(props,cursor,key,data) {
    INFO2("\tProperty: %s has value: %s",key,data);
  }
 
  /* Try to get a property that does not exist */
  char noexist[]="NoProp";
  const char *value = SD_workstation_get_property_value(w1,noexist);
  if ( value == NULL) 
    INFO1("\tProperty: %s is undefined", noexist);
  else
    INFO2("\tProperty: %s has value: %s", noexist, value);


  INFO1("Property list for workstation %s", name2);
  /* Get the property list of the workstation 2 */
  props = SD_workstation_get_properties(w2);
  cursor = NULL;

  /* Print the properties of the workstation 2 */
  xbt_dict_foreach(props,cursor,key,data) {
    INFO2("\tProperty: %s on host: %s",key,data);
  }

  /* Modify an existing property test. First check it exists */\
  INFO0("Modify an existing property");
  char exist[]="Hdd";
  value = SD_workstation_get_property_value(w2,exist);
  if ( value == NULL) 
    INFO1("\tProperty: %s is undefined", exist);
  else {
    INFO2("\tProperty: %s old value: %s", exist, value);
    xbt_dict_set(props, exist, strdup("250"), free);  
  }
 
  /* Test if we have changed the value */
  value = SD_workstation_get_property_value(w2,exist);
  if ( value == NULL) 
    INFO1("\tProperty: %s is undefined", exist);
  else
    INFO2("\tProperty: %s new value: %s", exist, value);
    
  

  const double computation_amount1 = 2000000;
  const double computation_amount2 = 1000000;
  const double communication_amount12 = 2000000;
  const double communication_amount21 = 3000000;

  /* NOTE: The link properties can be retrieved only from the SimDag interface */
  const SD_link_t *route = SD_route_get_list(w1, w2);
  int route_size = SD_route_get_size(w1, w2);
  for (i = 0; i < route_size; i++) {
     
    props = SD_link_get_properties(route[i]);
    xbt_dict_cursor_t cursor = NULL;
    char *key,*data;

    /* Print the properties of the current link */
    xbt_dict_foreach(props,cursor,key,data) {
    INFO3("\tLink %s property: %s has value: %s",SD_link_get_name(route[i]),key,data);

    /* Try to get a property that does not exist */
    char noexist1="Other";
    value = SD_link_get_property_value(route[i], noexist1);
    if ( value == NULL) 
      INFO2("\tProperty: %s for link %s is undefined", noexist, SD_link_get_name(route[i]));
    else
      INFO3("\tLink %s property: %s has value: %s",SD_link_get_name(route[i]),noexist,value);
  }

  }
  /* creation of the tasks and their dependencies */
  SD_task_t taskA = SD_task_create("Task A", NULL, 10.0);
  SD_task_t taskB = SD_task_create("Task B", NULL, 40.0);
  SD_task_t taskC = SD_task_create("Task C", NULL, 30.0);
  SD_task_t taskD = SD_task_create("Task D", NULL, 60.0);
  

  SD_task_dependency_add(NULL, NULL, taskB, taskA);
  SD_task_dependency_add(NULL, NULL, taskC, taskA);
  SD_task_dependency_add(NULL, NULL, taskD, taskB);
  SD_task_dependency_add(NULL, NULL, taskD, taskC);

  /* watch points */
  SD_task_watch(taskD, SD_DONE);
  SD_task_watch(taskB, SD_DONE);
  SD_task_unwatch(taskD, SD_DONE);
  

  /* scheduling parameters */

  const int workstation_number = 2;
  const SD_workstation_t workstation_list[] = {w1, w2};
  double computation_amount[] = {computation_amount1, computation_amount2};
  double communication_amount[] =
    {
      0, communication_amount12,
      communication_amount21, 0
    };
  double rate = -1.0;

  /* estimated time */
  SD_task_t task = taskD;
  INFO2("Estimated time for '%s': %f", SD_task_get_name(task),
	SD_task_get_execution_time(task, workstation_number, workstation_list,
				   computation_amount, communication_amount, rate));

  /* let's launch the simulation! */

  SD_task_schedule(taskA, workstation_number, workstation_list,
		   computation_amount, communication_amount, rate);
  SD_task_schedule(taskB, workstation_number, workstation_list,
		   computation_amount, communication_amount, rate);
  SD_task_schedule(taskC, workstation_number, workstation_list,
		   computation_amount, communication_amount, rate);
  SD_task_schedule(taskD, workstation_number, workstation_list,
		   computation_amount, communication_amount, rate);

  SD_task_t *changed_tasks;

  changed_tasks = SD_simulate(-1.0);
  for (i = 0; changed_tasks[i] != NULL; i++) {
    INFO3("Task '%s' start time: %f, finish time: %f",
	  SD_task_get_name(changed_tasks[i]),
	  SD_task_get_start_time(changed_tasks[i]),
	  SD_task_get_finish_time(changed_tasks[i]));
  }
  
  xbt_assert0(changed_tasks[0] == taskD &&
	      changed_tasks[1] == taskB &&
	      changed_tasks[2] == NULL,
	      "Unexpected simulation results");

  xbt_free(changed_tasks);

  DEBUG0("Destroying tasks...");

  SD_task_destroy(taskA);
  SD_task_destroy(taskB);
  SD_task_destroy(taskC);
  SD_task_destroy(taskD);

  DEBUG0("Tasks destroyed. Exiting SimDag...");

  SD_exit();
  return 0;
}

