/* Copyright (c) 2004-2018. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SURF_MODEL_H_
#define SURF_MODEL_H_

#include "xbt/signal.hpp"
#include "xbt/utility.hpp"

#include "src/surf/surf_private.hpp"
#include "surf/surf.hpp"
#include "xbt/str.h"

#include <boost/heap/pairing_heap.hpp>
#include <boost/intrusive/list.hpp>
#include <boost/optional.hpp>
#include <cmath>
#include <set>
#include <string>
#include <unordered_map>

/*********
 * Utils *
 *********/

/* user-visible parameters */
XBT_PUBLIC_DATA double sg_maxmin_precision;
XBT_PUBLIC_DATA double sg_surf_precision;
XBT_PUBLIC_DATA int sg_concurrency_limit;

extern XBT_PRIVATE double sg_tcp_gamma;
extern XBT_PRIVATE double sg_latency_factor;
extern XBT_PRIVATE double sg_bandwidth_factor;
extern XBT_PRIVATE double sg_weight_S_parameter;
extern XBT_PRIVATE int sg_network_crosstraffic;
extern XBT_PRIVATE std::vector<std::string> surf_path;
extern XBT_PRIVATE std::unordered_map<std::string, tmgr_trace_t> traces_set_list;
extern XBT_PRIVATE std::set<std::string> watched_hosts;

static inline void double_update(double* variable, double value, double precision)
{
  // printf("Updating %g -= %g +- %g\n",*variable,value,precision);
  // xbt_assert(value==0  || value>precision);
  // Check that precision is higher than the machine-dependent size of the mantissa. If not, brutal rounding  may
  // happen, and the precision mechanism is not active...
  // xbt_assert(*variable< (2<<DBL_MANT_DIG)*precision && FLT_RADIX==2);
  *variable -= value;
  if (*variable < precision)
    *variable = 0.0;
}

static inline int double_positive(double value, double precision)
{
  return (value > precision);
}

static inline int double_equals(double value1, double value2, double precision)
{
  return (fabs(value1 - value2) < precision);
}

extern "C" {
XBT_PUBLIC double surf_get_clock();
}
/** \ingroup SURF_simulation
 *  \brief List of hosts that have just restarted and whose autorestart process should be restarted.
 */
XBT_PUBLIC_DATA std::vector<sg_host_t> host_that_restart;

int XBT_PRIVATE __surf_is_absolute_file_path(const char *file_path);

/**********
 * Action *
 **********/

/** \ingroup SURF_models
 *  \brief List of initialized models
 */
XBT_PUBLIC_DATA std::vector<simgrid::kernel::resource::Model*>* all_existing_models;

#endif /* SURF_MODEL_H_ */
