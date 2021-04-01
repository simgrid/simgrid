/* Copyright (c) 2016-2021. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/mc/checker/UdporChecker.hpp"
#include <xbt/log.h>

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(mc_udpor, mc, "Logging specific to MC safety verification ");

namespace simgrid {
namespace mc {

UdporChecker::UdporChecker(Session* session) : Checker(session) {}

void UdporChecker::run() {}

RecordTrace UdporChecker::get_record_trace()
{
  RecordTrace res;
  return res;
}

std::vector<std::string> UdporChecker::get_textual_trace()
{
  std::vector<std::string> trace;
  return trace;
}

void UdporChecker::log_state() {}

Checker* create_udpor_checker(Session* session)
{
  return new UdporChecker(session);
}

} // namespace mc
} // namespace simgrid