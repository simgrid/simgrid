/* Copyright (c) 2017-2020. The SimGrid Team. All rights reserved.               */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "catch.hpp"

#include "simgrid/kernel/resource/Resource.hpp"
#include "src/kernel/resource/profile/DatedValue.hpp"
#include "src/kernel/resource/profile/Event.hpp"
#include "src/kernel/resource/profile/Profile.hpp"
#include "src/surf/surf_interface.hpp"

#include "xbt/log.h"
#include "xbt/misc.h"

#include <cmath>

XBT_LOG_NEW_DEFAULT_CATEGORY(unit, "Unit tests of the Trace Manager");

double thedate;
class MockedResource : public simgrid::kernel::resource::Resource {
public:
  explicit MockedResource() : simgrid::kernel::resource::Resource(nullptr, "fake", nullptr) {}
  void apply_event(simgrid::kernel::profile::Event* event, double value) override
  {
    XBT_VERB("t=%.1f: Change value to %lg (idx: %u)", thedate, value, event->idx);
    tmgr_trace_event_unref(&event);
  }
  bool is_used() override { return true; }
};

static std::vector<simgrid::kernel::profile::DatedValue> trace2vector(const char* str)
{
  std::vector<simgrid::kernel::profile::DatedValue> res;
  simgrid::kernel::profile::Profile* trace = simgrid::kernel::profile::Profile::from_string("TheName", str, 0);
  XBT_VERB("---------------------------------------------------------");
  XBT_VERB("data>>\n%s<<data\n", str);
  for (auto const& evt : trace->event_list)
    XBT_VERB("event: d:%lg v:%lg", evt.date_, evt.value_);

  MockedResource daResource;
  simgrid::kernel::profile::FutureEvtSet fes;
  simgrid::kernel::profile::Event* insertedIt = trace->schedule(&fes, &daResource);

  while (fes.next_date() <= 20.0 && fes.next_date() >= 0) {
    thedate = fes.next_date();
    double value;
    simgrid::kernel::resource::Resource* resource;
    simgrid::kernel::profile::Event* it = fes.pop_leq(thedate, &value, &resource);
    if (it == nullptr)
      continue;

    REQUIRE(it == insertedIt); // Check that we find what we've put
    if (value >= 0) {
      res.push_back(simgrid::kernel::profile::DatedValue(thedate, value));
    } else {
      XBT_DEBUG("%.1f: ignore an event (idx: %u)\n", thedate, it->idx);
    }
    resource->apply_event(it, value);
  }
  tmgr_finalize();
  return res;
}

TEST_CASE("kernel::profile: Resource profiles, defining the external load", "kernel::profile")
{
  SECTION("No event, no loop")
  {
    std::vector<simgrid::kernel::profile::DatedValue> got = trace2vector("");
    std::vector<simgrid::kernel::profile::DatedValue> want;
    REQUIRE(want == got);
  }

  SECTION("One event no loop")
  {
    std::vector<simgrid::kernel::profile::DatedValue> got = trace2vector("9.0 3.0\n");

    std::vector<simgrid::kernel::profile::DatedValue> want;
    want.push_back(simgrid::kernel::profile::DatedValue(9, 3));
    REQUIRE(want == got);
  }

  SECTION("Two events, no loop")
  {
    std::vector<simgrid::kernel::profile::DatedValue> got = trace2vector("3.0 1.0\n"
                                                                         "9.0 3.0\n");

    std::vector<simgrid::kernel::profile::DatedValue> want;
    want.push_back(simgrid::kernel::profile::DatedValue(3, 1));
    want.push_back(simgrid::kernel::profile::DatedValue(9, 3));

    REQUIRE(want == got);
  }

  SECTION("Three events, no loop")
  {
    std::vector<simgrid::kernel::profile::DatedValue> got = trace2vector("3.0 1.0\n"
                                                                         "5.0 2.0\n"
                                                                         "9.0 3.0\n");

    std::vector<simgrid::kernel::profile::DatedValue> want;
    want.push_back(simgrid::kernel::profile::DatedValue(3, 1));
    want.push_back(simgrid::kernel::profile::DatedValue(5, 2));
    want.push_back(simgrid::kernel::profile::DatedValue(9, 3));

    REQUIRE(want == got);
  }

  SECTION("Two events, looping")
  {
    std::vector<simgrid::kernel::profile::DatedValue> got = trace2vector("1.0 1.0\n"
                                                                         "3.0 3.0\n"
                                                                         "LOOPAFTER 2\n");

    std::vector<simgrid::kernel::profile::DatedValue> want;
    want.push_back(simgrid::kernel::profile::DatedValue(1, 1));
    want.push_back(simgrid::kernel::profile::DatedValue(3, 3));
    want.push_back(simgrid::kernel::profile::DatedValue(6, 1));
    want.push_back(simgrid::kernel::profile::DatedValue(8, 3));
    want.push_back(simgrid::kernel::profile::DatedValue(11, 1));
    want.push_back(simgrid::kernel::profile::DatedValue(13, 3));
    want.push_back(simgrid::kernel::profile::DatedValue(16, 1));
    want.push_back(simgrid::kernel::profile::DatedValue(18, 3));

    REQUIRE(want == got);
  }

  SECTION("Two events, looping, start at 0")
  {
    std::vector<simgrid::kernel::profile::DatedValue> got = trace2vector("0.0 1\n"
                                                                         "5.0 2\n"
                                                                         "LOOPAFTER 5\n");

    std::vector<simgrid::kernel::profile::DatedValue> want;
    want.push_back(simgrid::kernel::profile::DatedValue(0, 1));
    want.push_back(simgrid::kernel::profile::DatedValue(5, 2));
    want.push_back(simgrid::kernel::profile::DatedValue(10, 1));
    want.push_back(simgrid::kernel::profile::DatedValue(15, 2));
    want.push_back(simgrid::kernel::profile::DatedValue(20, 1));

    REQUIRE(want == got);
  }
}
