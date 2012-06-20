/*
 * jedule_events.h
 *
 *  Created on: Nov 30, 2010
 *      Author: sascha
 */

#ifndef JEDULE_EVENTS_H_
#define JEDULE_EVENTS_H_

#include "simgrid_config.h"

#include "xbt/dynar.h"
#include "xbt/dict.h"

#include "instr/jedule/jedule_platform.h"


#ifdef HAVE_JEDULE

struct jed_event {
  int event_id;
  char *name;
  double start_time;
  double end_time;
  char *type;
  xbt_dynar_t resource_subsets;
  xbt_dynar_t characteristics_list; /* just a list of names (strings) */
  xbt_dict_t info_hash;     /* key/value pairs */
};

typedef struct jed_event s_jed_event_t, *jed_event_t;

/************************************************************/

void create_jed_event(jed_event_t *event, char *name, double start_time, double end_time, const char *type);

void jed_event_free(jed_event_t event);

void jed_event_add_resources(jed_event_t event, xbt_dynar_t host_selection);

void jed_event_add_characteristic(jed_event_t event, char *characteristic);

void jed_event_add_info(jed_event_t event, char *key, char *value);

#endif

#endif /* JEDULE_EVENTS_H_ */
