/* Copyright (c) 2017. The SimGrid Team. All rights reserved.               */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "SmpiHost.hpp"
#include "simgrid/s4u/VirtualMachine.hpp"
#include "smpi_utils.hpp"

#include <string>
#include <vector>
#include <xbt/log.h>

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(smpi_host, smpi, "Logging specific to SMPI (host)");

namespace simgrid {
namespace smpi {

simgrid::xbt::Extension<simgrid::s4u::Host, SmpiHost> SmpiHost::EXTENSION_ID;

double SmpiHost::orecv(size_t size)
{
  double current = orecv_parsed_values.empty() ? 0.0 : orecv_parsed_values.front().values[0] +
                                                           orecv_parsed_values.front().values[1] * size;

  // Iterate over all the sections that were specified and find the right value. (fact.factor represents the interval
  // sizes; we want to find the section that has fact.factor <= size and no other such fact.factor <= size)
  // Note: parse_factor() (used before) already sorts the vector we iterate over!
  for (auto const& fact : orecv_parsed_values) {
    if (size <= fact.factor) { // Values already too large, use the previously computed value of current!
      XBT_DEBUG("or : %zu <= %zu return %.10f", size, fact.factor, current);
      return current;
    } else {
      // If the next section is too large, the current section must be used.
      // Hence, save the cost, as we might have to use it.
      current=fact.values[0]+fact.values[1]*size;
    }
  }
  XBT_DEBUG("smpi_or: %zu is larger than largest boundary, return %.10f", size, current);

  return current;
}

double SmpiHost::osend(size_t size)
{
  double current =
      osend_parsed_values.empty() ? 0.0 : osend_parsed_values[0].values[0] + osend_parsed_values[0].values[1] * size;
  // Iterate over all the sections that were specified and find the right
  // value. (fact.factor represents the interval sizes; we want to find the
  // section that has fact.factor <= size and no other such fact.factor <= size)
  // Note: parse_factor() (used before) already sorts the vector we iterate over!
  for (auto const& fact : osend_parsed_values) {
    if (size <= fact.factor) { // Values already too large, use the previously computed value of current!
      XBT_DEBUG("os : %zu <= %zu return %.10f", size, fact.factor, current);
      return current;
    } else {
      // If the next section is too large, the current section must be used.
      // Hence, save the cost, as we might have to use it.
      current = fact.values[0] + fact.values[1] * size;
    }
  }
  XBT_DEBUG("Searching for smpi/os: %zu is larger than the largest boundary, return %.10f", size, current);

  return current;
}

double SmpiHost::oisend(size_t size)
{
  double current =
      oisend_parsed_values.empty() ? 0.0 : oisend_parsed_values[0].values[0] + oisend_parsed_values[0].values[1] * size;

  // Iterate over all the sections that were specified and find the right value. (fact.factor represents the interval
  // sizes; we want to find the section that has fact.factor <= size and no other such fact.factor <= size)
  // Note: parse_factor() (used before) already sorts the vector we iterate over!
  for (auto const& fact : oisend_parsed_values) {
    if (size <= fact.factor) { // Values already too large, use the previously  computed value of current!
      XBT_DEBUG("ois : %zu <= %zu return %.10f", size, fact.factor, current);
      return current;
    } else {
      // If the next section is too large, the current section must be used.
      // Hence, save the cost, as we might have to use it.
      current = fact.values[0] + fact.values[1] * size;
    }
  }
  XBT_DEBUG("Searching for smpi/ois: %zu is larger than the largest boundary, return %.10f", size, current);

  return current;
}

SmpiHost::SmpiHost(simgrid::s4u::Host *ptr) : host(ptr)
{
  if (not SmpiHost::EXTENSION_ID.valid())
    SmpiHost::EXTENSION_ID = simgrid::s4u::Host::extension_create<SmpiHost>();

  const char* orecv_string = host->getProperty("smpi/or");
  if (orecv_string != nullptr) {
    orecv_parsed_values = parse_factor(orecv_string);
  } else {
    orecv_parsed_values = parse_factor(xbt_cfg_get_string("smpi/or"));
  }

  const char* osend_string = host->getProperty("smpi/os");
  if (osend_string != nullptr) {
    osend_parsed_values = parse_factor(osend_string);
  } else {
    osend_parsed_values = parse_factor(xbt_cfg_get_string("smpi/os"));
  }

  const char* oisend_string = host->getProperty("smpi/ois");
  if (oisend_string != nullptr) {
    oisend_parsed_values = parse_factor(oisend_string);
  } else {
    oisend_parsed_values = parse_factor(xbt_cfg_get_string("smpi/ois"));
  }
}

SmpiHost::~SmpiHost()=default;
}
}
