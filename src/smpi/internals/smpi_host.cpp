/* Copyright (c) 2017-2022. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "smpi_host.hpp"
#include "private.hpp"
#include "simgrid/s4u/VirtualMachine.hpp"
#include "smpi/smpi.h"
#include "smpi_utils.hpp"
#include "xbt/config.hpp"

#include <string>
#include <vector>
#include <xbt/log.h>

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(smpi_host, smpi, "Logging specific to SMPI (host)");

/* save user's cost callbacks for SMPI operations */
static std::unordered_map<SmpiOperation, SmpiOpCostCb> cost_cbs;

void smpi_register_op_cost_callback(SmpiOperation op, const SmpiOpCostCb& cb)
{
  cost_cbs[op] = cb;
}

void smpi_cleanup_op_cost_callback()
{
  cost_cbs.clear();
}

namespace simgrid {
namespace smpi {

xbt::Extension<s4u::Host, smpi::Host> Host::EXTENSION_ID;

double Host::orecv(size_t size, s4u::Host* src, s4u::Host* dst)
{
  /* return user's callback if available */
  auto it = cost_cbs.find(SmpiOperation::RECV);
  if (it != cost_cbs.end())
    return it->second(size, src, dst);

  /* fallback to smpi/or config */
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

double Host::osend(size_t size, s4u::Host* src, s4u::Host* dst)
{
  /* return user's callback if available */
  auto it = cost_cbs.find(SmpiOperation::SEND);
  if (it != cost_cbs.end())
    return it->second(size, src, dst);

  /* fallback to smpi/os config */
  double current =
      osend_parsed_values.empty() ? 0.0 : osend_parsed_values[0].values[0] + osend_parsed_values[0].values[1] * size;
  // Iterate over all the sections that were specified and find the right value. (fact.factor represents the interval
  // sizes; we want to find the section that has fact.factor <= size and no other such fact.factor <= size)
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

double Host::oisend(size_t size, s4u::Host* src, s4u::Host* dst)
{
  /* return user's callback if available */
  auto it = cost_cbs.find(SmpiOperation::ISEND);
  if (it != cost_cbs.end())
    return it->second(size, src, dst);

  /* fallback to smpi/ois config */
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

void Host::check_factor_configs(const std::string& op) const
{
  static const std::unordered_map<std::string, SmpiOperation> name_to_op_enum{
      {"smpi/or", SmpiOperation::RECV}, {"smpi/os", SmpiOperation::SEND}, {"smpi/ois", SmpiOperation::ISEND}};
  if (cost_cbs.find(name_to_op_enum.at(op)) != cost_cbs.end() &&
      (host->get_property(op) || not config::is_default(op.c_str()))) {
    XBT_WARN("SMPI (host: %s): mismatch cost functions for %s. Only user's callback will be used.", host->get_cname(),
             op.c_str());
  }
}

Host::Host(s4u::Host* ptr) : host(ptr)
{
  if (not smpi::Host::EXTENSION_ID.valid())
    smpi::Host::EXTENSION_ID = s4u::Host::extension_create<Host>();

  check_factor_configs("smpi/or");
  const char* orecv_string = host->get_property("smpi/or");
  if (orecv_string != nullptr) {
    orecv_parsed_values = simgrid::smpi::utils::parse_factor(orecv_string);
  } else {
    orecv_parsed_values = simgrid::smpi::utils::parse_factor(config::get_value<std::string>("smpi/or"));
  }

  check_factor_configs("smpi/os");
  const char* osend_string = host->get_property("smpi/os");
  if (osend_string != nullptr) {
    osend_parsed_values = simgrid::smpi::utils::parse_factor(osend_string);
  } else {
    osend_parsed_values = simgrid::smpi::utils::parse_factor(config::get_value<std::string>("smpi/os"));
  }

  check_factor_configs("smpi/ois");
  const char* oisend_string = host->get_property("smpi/ois");
  if (oisend_string != nullptr) {
    oisend_parsed_values = simgrid::smpi::utils::parse_factor(oisend_string);
  } else {
    oisend_parsed_values = simgrid::smpi::utils::parse_factor(config::get_value<std::string>("smpi/ois"));
  }
}

} // namespace smpi
} // namespace simgrid
