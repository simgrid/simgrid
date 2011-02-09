/*
 * jedule_sd_binding.c
 *
 *  Created on: Dec 2, 2010
 *      Author: sascha
 */


#include "xbt/asserts.h"
#include "xbt/dynar.h"

void jedule_log_sd_event(SD_task_t task) {
	xbt_dynar_t host_list;
	jed_event_t event;
	int i;

	xbt_assert(task != NULL);

	host_list = xbt_dynar_new(sizeof(char*), NULL);

	for(i=0; i<task->workstation_nb; i++) {
		char *hostname = surf_resource_name(task->workstation_list[i]->surf_workstation);
		xbt_dynar_push(host_list, &hostname);
	}

	create_jed_event(&event, SD_task_get_name(task), task->start_time,
			task->finish_time, "SD");

	jed_event_add_resources(event, host_list);

	jedule_store_event(event);

	xbt_dynar_free(&host_list);
}


void jedule_setup_platform() {




}
