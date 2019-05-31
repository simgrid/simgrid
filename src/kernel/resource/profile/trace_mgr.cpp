/* Copyright (c) 2004-2019. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "xbt/log.h"
#include "xbt/sysdep.h"

#include "src/kernel/resource/profile/trace_mgr.hpp"
#include "src/surf/surf_interface.hpp"
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/join.hpp>
#include <boost/algorithm/string/split.hpp>
#include <cmath>
#include <fstream>
#include <sstream>
#include <unordered_map>

#include "src/kernel/resource/profile/Profile.cpp"

/** This file has been splitted into three different files:
  - Profile, defining the class Profile and functions that allow to use provided profiles;
  - DatedValue, the class of a DatedValue (a value and a timestamp);
  - FutureEvtSet, a set of events happening in the future. **/

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(profile, resource, "Surf profile management");
void tmgr_finalize()
{
  for (auto const& kv : trace_list)
    delete kv.second;
  trace_list.clear();
}

void tmgr_trace_event_unref(simgrid::kernel::profile::Event** event)
{
  if ((*event)->free_me) {
    delete *event;
    *event = nullptr;
  }
}