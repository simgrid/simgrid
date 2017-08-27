/* Copyright (c) 2017. The SimGrid Team. All rights reserved.               */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#define BOOST_TEST_MODULE Trace Manager tests
bool init_unit_test(); // boost forget to give this prototype on NetBSD, which does not fit our paranoid flags
#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_NO_MAIN
#include <boost/test/unit_test.hpp>

#include "src/surf/surf_interface.hpp"
#include "src/surf/trace_mgr.hpp"

#include "xbt/log.h"
#include "xbt/misc.h"

#include <cmath>

namespace utf  = boost::unit_test;
namespace tmgr = simgrid::trace_mgr;

XBT_LOG_NEW_DEFAULT_CATEGORY(unit, "Unit tests of the Trace Manager");

double thedate;
class MockedResource : public simgrid::surf::Resource {
public:
  explicit MockedResource() : simgrid::surf::Resource(nullptr, "fake", nullptr) {}
  void apply_event(tmgr_trace_event_t event, double value)
  {
    XBT_VERB("t=%.1f: Change value to %lg (idx: %u)", thedate, value, event->idx);
    tmgr_trace_event_unref(&event);
  }
  bool isUsed() { return true; }
};

static void trace2vector(const char* str, std::vector<tmgr::DatedValue>* whereto)
{
  simgrid::trace_mgr::trace* trace = tmgr_trace_new_from_string("TheName", str, 0);
  XBT_VERB("---------------------------------------------------------");
  XBT_VERB("data>>\n%s<<data\n", str);
  for (auto const& evt : trace->event_list)
    XBT_VERB("event: d:%lg v:%lg", evt.date_, evt.value_);

  MockedResource daResource;
  simgrid::trace_mgr::future_evt_set fes;
  tmgr_trace_event_t insertedIt = fes.add_trace(trace, &daResource);

  while (fes.next_date() <= 20.0 && fes.next_date() >= 0) {
    thedate = fes.next_date();
    double value;
    simgrid::surf::Resource* res;
    tmgr_trace_event_t it = fes.pop_leq(thedate, &value, &res);
    if (it == nullptr)
      continue;

    BOOST_CHECK_EQUAL(it, insertedIt); // Check that we find what we've put
    if (value >= 0) {
      res->apply_event(it, value);
      whereto->push_back(tmgr::DatedValue(thedate, value));
    } else {
      XBT_DEBUG("%.1f: ignore an event (idx: %u)\n", thedate, it->idx);
    }
  }
  tmgr_finalize();
}

/* Fails in a way that is difficult to test: xbt_assert should become throw
BOOST_AUTO_TEST_CASE(no_evt_noloop) {
  std::vector<Evt> got;
  trace2vector("", &got);
  std::vector<Evt> want;
  BOOST_CHECK_EQUAL_COLLECTIONS(want.begin(), want.end(), got.begin(), got.end());
}*/
BOOST_AUTO_TEST_CASE(one_evt_noloop)
{
  std::vector<tmgr::DatedValue> got;
  trace2vector("9.0 3.0\n", &got);

  std::vector<tmgr::DatedValue> want;
  want.push_back(tmgr::DatedValue(9, 3));
  BOOST_CHECK_EQUAL_COLLECTIONS(want.begin(), want.end(), got.begin(), got.end());
}
BOOST_AUTO_TEST_CASE(two_evt_noloop)
{
  std::vector<tmgr::DatedValue> got;
  trace2vector("3.0 1.0\n"
               "9.0 3.0\n",
               &got);

  std::vector<tmgr::DatedValue> want;
  want.push_back(tmgr::DatedValue(3, 1));
  want.push_back(tmgr::DatedValue(9, 3));

  BOOST_CHECK_EQUAL_COLLECTIONS(want.begin(), want.end(), got.begin(), got.end());
}
BOOST_AUTO_TEST_CASE(three_evt_noloop)
{
  std::vector<tmgr::DatedValue> got;
  trace2vector("3.0 1.0\n"
               "5.0 2.0\n"
               "9.0 3.0\n",
               &got);

  std::vector<tmgr::DatedValue> want;
  want.push_back(tmgr::DatedValue(3, 1));
  want.push_back(tmgr::DatedValue(5, 2));
  want.push_back(tmgr::DatedValue(9, 3));

  BOOST_CHECK_EQUAL_COLLECTIONS(want.begin(), want.end(), got.begin(), got.end());
}

BOOST_AUTO_TEST_CASE(two_evt_loop)
{
  std::vector<tmgr::DatedValue> got;
  trace2vector("1.0 1.0\n"
               "3.0 3.0\n"
               "LOOPAFTER 2\n",
               &got);

  std::vector<tmgr::DatedValue> want;
  want.push_back(tmgr::DatedValue(1, 1));
  want.push_back(tmgr::DatedValue(3, 3));
  want.push_back(tmgr::DatedValue(6, 1));
  want.push_back(tmgr::DatedValue(8, 3));
  want.push_back(tmgr::DatedValue(11, 1));
  want.push_back(tmgr::DatedValue(13, 3));
  want.push_back(tmgr::DatedValue(16, 1));
  want.push_back(tmgr::DatedValue(18, 3));

  BOOST_CHECK_EQUAL_COLLECTIONS(want.begin(), want.end(), got.begin(), got.end());
}
BOOST_AUTO_TEST_CASE(two_evt_start0_loop)
{
  std::vector<tmgr::DatedValue> got;
  trace2vector("0.0 1\n"
               "5.0 2\n"
               "LOOPAFTER 5\n",
               &got);

  std::vector<tmgr::DatedValue> want;
  want.push_back(tmgr::DatedValue(0, 1));
  want.push_back(tmgr::DatedValue(5, 2));
  want.push_back(tmgr::DatedValue(10, 1));
  want.push_back(tmgr::DatedValue(15, 2));
  want.push_back(tmgr::DatedValue(20, 1));

  BOOST_CHECK_EQUAL_COLLECTIONS(want.begin(), want.end(), got.begin(), got.end());
}

static bool init_function()
{
  // do your own initialization here (and return true on success)
  // But, you CAN'T use testing tools here
  return true;
}

int main(int argc, char** argv)
{
  XBT_LOG_CONNECT(unit);
  xbt_log_init(&argc, argv);
  return ::boost::unit_test::unit_test_main(&init_function, argc, argv);
}
