/* Copyright (c) 2010-2025. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef ACTIVITY_LIFECYCLE_HPP
#define ACTIVITY_LIFECYCLE_HPP

#include "src/3rd-party/catch.hpp"

#include <simgrid/s4u.hpp>
#include <xbt/log.h>

#include <vector>

XBT_LOG_EXTERNAL_DEFAULT_CATEGORY(s4u_test);

extern std::vector<simgrid::s4u::Host*> all_hosts;

/* Helper function easing the testing of actor's ending condition */
extern void assert_exit(bool exp_success, double duration);

/* Helper function in charge of doing some sanity checks after each test */
extern void assert_cleanup();

/* We need an extra actor here, so that it can sleep until the end of each test */
#define BEGIN_SECTION(descr)                                                                                           \
  SECTION(descr)                                                                                                       \
  { all_hosts[0]->add_actor(descr,  []()
#define END_SECTION                                                                                                    \
  })

#define RUN_SECTION(descr, ...)                                                                                        \
  SECTION(descr) all_hosts[0]->add_actor(descr, __VA_ARGS__)

// Normally, we should be able use Catch2's REQUIRE_THROWS_AS(...), but it generates errors with Address Sanitizer.
// They're certainly false positive. Nevermind and use this simpler replacement.
#define REQUIRE_NETWORK_FAILURE(...)                                                                                   \
  do {                                                                                                                 \
    try {                                                                                                              \
      __VA_ARGS__;                                                                                                     \
      FAIL("Expected exception NetworkFailureException not caught");                                                   \
    } catch (simgrid::NetworkFailureException const&) {                                                                \
      XBT_VERB("got expected NetworkFailureException");                                                                \
    }                                                                                                                  \
  } while (0)

#define REQUIRE_STORAGE_FAILURE(...)                                                                                   \
  do {                                                                                                                 \
    try {                                                                                                              \
      __VA_ARGS__;                                                                                                     \
      FAIL("Expected exception StorageFailureException not caught");                                                   \
    } catch (simgrid::StorageFailureException const&) {                                                                \
      XBT_VERB("got expected StorageFailureException");                                                                \
    }                                                                                                                  \
  } while (0)

#endif // ACTIVITY_LIFECYCLE_HPP
