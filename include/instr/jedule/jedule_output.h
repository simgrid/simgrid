/*
 * jedule_output.h
 *
 *  Created on: Nov 30, 2010
 *      Author: sascha
 */

#ifndef JEDULE_OUTPUT_H_
#define JEDULE_OUTPUT_H_

#include "simgrid_config.h"

#include <stdio.h>

#include "jedule_events.h"
#include "jedule_platform.h"

#ifdef HAVE_JEDULE

extern xbt_dynar_t jedule_event_list;

void jedule_init_output(void);

void jedule_cleanup_output(void);

void jedule_store_event(jed_event_t event);

void write_jedule_output(FILE *file, jedule_t jedule,
    xbt_dynar_t event_list, xbt_dict_t meta_info_dict);

#endif

#endif /* JEDULE_OUTPUT_H_ */
