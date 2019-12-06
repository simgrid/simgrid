/* Copyright (c) 2018-2019. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SMPI_CONFIG_HPP
#define SMPI_CONFIG_HPP

#include "src/internal_config.h" // HAVE_PAPI
#include <xbt/config.hpp>

XBT_PRIVATE void smpi_check_options();
/********************************** Configuration of SMPI **************************************/
extern XBT_PRIVATE simgrid::config::Flag<double> _smpi_cfg_host_speed;
extern XBT_PRIVATE simgrid::config::Flag<bool> _smpi_cfg_simulate_computation;
extern XBT_PRIVATE simgrid::config::Flag<std::string> _smpi_cfg_shared_malloc_string;
extern XBT_PRIVATE simgrid::config::Flag<double> _smpi_cfg_cpu_thresh;
extern XBT_PRIVATE simgrid::config::Flag<std::string> _smpi_cfg_privatization_string;
extern XBT_PRIVATE simgrid::config::Flag<int> _smpi_cfg_async_small_thresh;
extern XBT_PRIVATE simgrid::config::Flag<int> _smpi_cfg_detached_send_thresh;
extern XBT_PRIVATE simgrid::config::Flag<bool> _smpi_cfg_grow_injected_times;
extern XBT_PRIVATE simgrid::config::Flag<double> _smpi_cfg_iprobe_cpu_usage;
extern XBT_PRIVATE simgrid::config::Flag<bool> _smpi_cfg_trace_call_use_absolute_path;
extern XBT_PRIVATE simgrid::config::Flag<bool> _smpi_cfg_trace_call_location;
extern XBT_PRIVATE simgrid::config::Flag<std::string> _smpi_cfg_comp_adjustment_file;
#if HAVE_PAPI
extern XBT_PRIVATE simgrid::config::Flag<std::string> _smpi_cfg_papi_events_file;
#endif
extern XBT_PRIVATE simgrid::config::Flag<double> _smpi_cfg_auto_shared_malloc_thresh;
#endif
