/* Copyright (c) 2016. The SimGrid Team. All rights reserved.               */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SMPI_PRIVATE_HPP
#define SMPI_PRIVATE_HPP

#include <unordered_map>
#include "src/instr/instr_smpi.h"

#ifdef HAVE_PAPI
typedef 
    std::vector<std::pair</* counter name */std::string, /* counter value */long long>> papi_counter_t;
XBT_PRIVATE papi_counter_t& smpi_process_counter_data(void);
XBT_PRIVATE int smpi_process_event_set(void);
#endif

extern std::unordered_map<std::string, double> location2speedup;

/** @brief Returns the last call location (filename, linenumber). Process-specific. */
XBT_PUBLIC(smpi_trace_call_location_t*) smpi_process_get_call_location(void);
XBT_PUBLIC(smpi_trace_call_location_t*) smpi_trace_get_call_location();
#endif
