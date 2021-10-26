/* Copyright (c) 2017-2021. The SimGrid Team. All rights reserved.               */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "catch.hpp"

#include "src/kernel/resource/Resource.hpp"
#include "src/kernel/resource/profile/DatedValue.hpp"
#include "src/kernel/resource/profile/Event.hpp"
#include "src/kernel/resource/profile/Profile.hpp"
#include "src/kernel/resource/profile/StochasticDatedValue.hpp"
#include "src/surf/surf_interface.hpp"

#include "xbt/log.h"
#include "xbt/misc.h"
#include "xbt/random.hpp"

#include <cmath>

XBT_LOG_NEW_DEFAULT_CATEGORY(unit, "Unit tests of the Trace Manager");

class MockedResource : public simgrid::kernel::resource::Resource {
public:
  static double the_date;

  explicit MockedResource() : Resource("fake"){}
  void apply_event(simgrid::kernel::profile::Event* event, double value) override
  {
    XBT_VERB("t=%.1f: Change value to %lg (idx: %u)", the_date, value, event->idx);
    tmgr_trace_event_unref(&event);
  }
  bool is_used() const override { return true; }
};

double MockedResource::the_date;

static std::vector<simgrid::kernel::profile::DatedValue> trace2vector(const char* str)
{
  std::vector<simgrid::kernel::profile::DatedValue> res;
  simgrid::kernel::profile::Profile* trace = simgrid::kernel::profile::Profile::from_string("TheName", str, 0);
  XBT_VERB("---------------------------------------------------------");
  XBT_VERB("data>>\n%s<<data\n", str);
  for (auto const& evt : trace->get_event_list())
    XBT_VERB("event: d:%lg v:%lg", evt.date_, evt.value_);

  MockedResource daResource;
  simgrid::kernel::profile::FutureEvtSet fes;
  simgrid::kernel::profile::Event* insertedIt = trace->schedule(&fes, &daResource);

  while (fes.next_date() <= 20.0 && fes.next_date() >= 0) {
    MockedResource::the_date = fes.next_date();
    double value;
    simgrid::kernel::resource::Resource* resource;
    simgrid::kernel::profile::Event* it = fes.pop_leq(MockedResource::the_date, &value, &resource);
    if (it == nullptr)
      continue;

    REQUIRE(it == insertedIt); // Check that we find what we've put
    if (value >= 0) {
      res.emplace_back(MockedResource::the_date, value);
    } else {
      XBT_DEBUG("%.1f: ignore an event (idx: %u)\n", MockedResource::the_date, it->idx);
    }
    resource->apply_event(it, value);
  }
  tmgr_finalize();
  return res;
}

static std::vector<simgrid::kernel::profile::StochasticDatedValue> trace2selist(const char* str)
{
  const simgrid::kernel::profile::Profile* trace = simgrid::kernel::profile::Profile::from_string("TheName", str, 0);
  std::vector<simgrid::kernel::profile::StochasticDatedValue> stocevlist = trace->get_stochastic_event_list();
  tmgr_finalize();
  return stocevlist;
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
    want.emplace_back(9, 3);
    REQUIRE(want == got);
  }

  SECTION("Two events, no loop")
  {
    std::vector<simgrid::kernel::profile::DatedValue> got = trace2vector("3.0 1.0\n"
                                                                         "9.0 3.0\n");

    std::vector<simgrid::kernel::profile::DatedValue> want;
    want.emplace_back(3, 1);
    want.emplace_back(9, 3);

    REQUIRE(want == got);
  }

  SECTION("Three events, no loop")
  {
    std::vector<simgrid::kernel::profile::DatedValue> got = trace2vector("3.0 1.0\n"
                                                                         "5.0 2.0\n"
                                                                         "9.0 3.0\n");

    std::vector<simgrid::kernel::profile::DatedValue> want;
    want.emplace_back(3, 1);
    want.emplace_back(5, 2);
    want.emplace_back(9, 3);

    REQUIRE(want == got);
  }

  SECTION("Two events, looping")
  {
    std::vector<simgrid::kernel::profile::DatedValue> got = trace2vector("1.0 1.0\n"
                                                                         "3.0 3.0\n"
                                                                         "LOOPAFTER 2\n");

    std::vector<simgrid::kernel::profile::DatedValue> want;
    want.emplace_back(1, 1);
    want.emplace_back(3, 3);
    want.emplace_back(6, 1);
    want.emplace_back(8, 3);
    want.emplace_back(11, 1);
    want.emplace_back(13, 3);
    want.emplace_back(16, 1);
    want.emplace_back(18, 3);

    REQUIRE(want == got);
  }

  SECTION("Two events, looping, start at 0")
  {
    std::vector<simgrid::kernel::profile::DatedValue> got = trace2vector("0.0 1\n"
                                                                         "5.0 2\n"
                                                                         "LOOPAFTER 5\n");

    std::vector<simgrid::kernel::profile::DatedValue> want;
    want.emplace_back(0, 1);
    want.emplace_back(5, 2);
    want.emplace_back(10, 1);
    want.emplace_back(15, 2);
    want.emplace_back(20, 1);

    REQUIRE(want == got);
  }

  SECTION("One stochastic event (parsing)")
  {
    using simgrid::kernel::profile::Distribution;
    std::vector<simgrid::kernel::profile::StochasticDatedValue> got = trace2selist("STOCHASTIC\n"
                                                                                   "DET 0 UNIF 10 20");

    std::vector<simgrid::kernel::profile::StochasticDatedValue> want;
    want.emplace_back(0, -1); // The initial fake event
    want.emplace_back(Distribution::DET, std::vector<double>{0}, Distribution::UNIF, std::vector<double>{10, 20});

    REQUIRE(want == got);
  }

  SECTION("Several stochastic events (all possible parsing forms)")
  {
    using simgrid::kernel::profile::Distribution;
    std::vector<simgrid::kernel::profile::StochasticDatedValue> got = trace2selist("STOCHASTIC\n"
                                                                                   "DET 0 DET 4\n"
                                                                                   "NORMAL 25 10 DET 3\n"
                                                                                   "UNIF 10 20 NORMAL 25 10\n"
                                                                                   "DET 5 UNIF 5 25");

    std::vector<simgrid::kernel::profile::StochasticDatedValue> want;
    want.emplace_back(0, -1);
    want.emplace_back(Distribution::DET, std::vector<double>{0}, Distribution::DET, std::vector<double>{4});
    want.emplace_back(Distribution::NORM, std::vector<double>{25, 10}, Distribution::DET, std::vector<double>{3});
    want.emplace_back(Distribution::UNIF, std::vector<double>{10, 20}, Distribution::NORM, std::vector<double>{25, 10});
    want.emplace_back(Distribution::DET, std::vector<double>{5}, Distribution::UNIF, std::vector<double>{5, 25});

    REQUIRE(want == got);
  }

  SECTION("Two stochastic events (drawing each distribution)")
  {
    simgrid::xbt::random::set_implem_xbt();
    simgrid::xbt::random::set_mersenne_seed(12345);
    std::vector<simgrid::kernel::profile::DatedValue> got = trace2vector("STOCHASTIC\n"
                                                                         "DET 0 UNIF 10 20\n"
                                                                         "EXP 0.05 NORMAL 15 5");

    std::vector<simgrid::kernel::profile::DatedValue> want;
    // The following values were drawn using the XBT_RNG_xbt method /outside/ the testcase.
    want.emplace_back(0, 19.29616086867082813683);
    want.emplace_back(2.32719992449416279712, 20.16807234800742065772);

    REQUIRE(want == got);
  }

  SECTION("Two stochastic events, with a loop")
  {
    simgrid::xbt::random::set_implem_xbt();
    simgrid::xbt::random::set_mersenne_seed(12345);
    std::vector<simgrid::kernel::profile::DatedValue> got = trace2vector("STOCHASTIC LOOP\n"
                                                                         "DET 0 UNIF 10 20\n"
                                                                         "EXP 0.05 NORMAL 15 5\n"
                                                                         "UNIF 1 2 DET 0");

    // In this case, the main use of the last stochastic event is to set when the first event takes place.

    std::vector<simgrid::kernel::profile::DatedValue> want;
    want.emplace_back(0, 19.29616086867082813683);
    want.emplace_back(2.32719992449416279712, 20.16807234800742065772);
    want.emplace_back(3.51111873684917075167, 0);
    want.emplace_back(3.51111873684917075167, 10.39759496468994726115);

    REQUIRE(want == got);
  }
}
