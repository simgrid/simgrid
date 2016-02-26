/* Copyright (c) 2006-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "xbt/log.h"
#include "src/msg/msg_private.h"

#include "simgrid/s4u/async.hpp"

XBT_LOG_EXTERNAL_CATEGORY(s4u);
XBT_LOG_NEW_DEFAULT_SUBCATEGORY(s4u_async,s4u,"S4U asynchronous actions");
using namespace simgrid;

s4u::Async::Async() {

}
s4u::Async::~Async() {

}

void s4u::Async::setRemains(double remains) {
  xbt_assert(p_state == inited, "Cannot change the remaining amount of work once the Async is started");
  p_remains = remains;
}

