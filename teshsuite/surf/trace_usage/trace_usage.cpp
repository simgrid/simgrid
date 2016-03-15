/* A few tests for the trace library                                       */

/* Copyright (c) 2004-2006, 2009-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "surf/surf.h"
#include "xbt/log.h"
#include "src/surf/network_interface.hpp"
#include "src/surf/trace_mgr.hpp"

XBT_LOG_NEW_DEFAULT_CATEGORY(surf_test, "Messages specific for surf example");

class DummyTestResource
    : public simgrid::surf::Resource {
public:
  DummyTestResource(const char *name) : Resource(nullptr,name) {}
  bool isUsed() override {return false;}
  void apply_event(tmgr_trace_iterator_t it, double value) override {}
};

int main(int argc, char **argv)
{
  surf_init(&argc, argv);

  simgrid::trace_mgr::future_evt_set *fes = new simgrid::trace_mgr::future_evt_set();
  tmgr_trace_t trace_A = tmgr_trace_new_from_file("trace_A.txt");
  tmgr_trace_t trace_B = tmgr_trace_new_from_file("trace_B.txt");
  double next_event_date = -1.0;
  double value = -1.0;
  simgrid::surf::Resource *resource = NULL;
  simgrid::surf::Resource *hostA = new DummyTestResource("Host A");
  simgrid::surf::Resource *hostB = new DummyTestResource("Host B");

  fes->add_trace(trace_A, 1.0, hostA);
  fes->add_trace(trace_B, 0.0, hostB);

  while ((next_event_date = fes->next_date()) != -1.0) {
    XBT_INFO("%g:", next_event_date);
    while (fes->pop_leq(next_event_date, &value, &resource)) {
      XBT_INFO("   %s: %g", resource->getName(), value);
    }
    if (next_event_date > 100)
      break;
  }

  delete fes;
  delete hostA;
  delete hostB;

  surf_exit();
  return 0;
}
