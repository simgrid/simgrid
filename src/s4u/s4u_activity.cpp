/* Copyright (c) 2006-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "xbt/asserts.h"
#include "xbt/log.h"

#include "simgrid/s4u/Activity.hpp"

XBT_LOG_EXTERNAL_CATEGORY(s4u);
XBT_LOG_NEW_DEFAULT_SUBCATEGORY(s4u_activity,s4u,"S4U activities");

namespace simgrid {
namespace s4u {

double Activity::getRemains()
{
  return remains_;
}

Activity* Activity::setRemains(double remains)
{
  xbt_assert(state_ == inited, "Cannot change the remaining amount of work once the Activity is started");
  remains_ = remains;
  return this;
}

}
}
