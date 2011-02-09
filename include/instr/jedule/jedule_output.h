/*
 * jedule_output.h
 *
 *  Created on: Nov 30, 2010
 *      Author: sascha
 */

#ifndef JEDULE_OUTPUT_H_
#define JEDULE_OUTPUT_H_

#include "jedule_events.h"
#include "jedule_platform.h"

xbt_dynar_t jedule_event_list;

void init_jedule_output();

void cleanup_jedule();

void jedule_store_event(jed_event_t event);

void write_jedule_output(char *filename, jedule_t jedule,
		xbt_dynar_t event_list, xbt_dict_t meta_info_dict);

#endif /* JEDULE_OUTPUT_H_ */
