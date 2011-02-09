/*
 * jedule_sd_binding.h
 *
 *  Created on: Dec 2, 2010
 *      Author: sascha
 */

#ifndef JEDULE_SD_BINDING_H_
#define JEDULE_SD_BINDING_H_

#include "simgrid_config.h"

#include "simdag/private.h"
#include "simdag/datatypes.h"
#include "simdag/simdag.h"

#ifdef HAVE_JEDULE

void jedule_log_sd_event(SD_task_t task);

void jedule_setup_platform();

void jedule_sd_init();

void jedule_sd_cleanup();

void jedule_sd_dump();

#endif /* JEDULE_SD_BINDING_H_ */


#endif
