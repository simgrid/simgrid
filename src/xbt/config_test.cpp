/* Copyright (c) 2004-2020. The SimGrid Team. All rights reserved.     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <xbt/config.hpp>

#include "simgrid/Exception.hpp"
#include <string>
#include <xbt/log.h>

#include "catch.hpp"

XBT_PUBLIC_DATA simgrid::config::Config* simgrid_config;

TEST_CASE("xbt::config: Configuration support", "config")
{
  XBT_LOG_EXTERNAL_DEFAULT_CATEGORY(xbt_cfg);
  xbt_log_threshold_set(&_XBT_LOGV(xbt_cfg), xbt_log_priority_critical);

  auto temp      = simgrid_config;
  simgrid_config = nullptr;
  simgrid::config::declare_flag<int>("speed", "description", 0);
  simgrid::config::declare_flag<std::string>("peername", "description", "");
  simgrid::config::declare_flag<std::string>("user", "description", "");

  SECTION("Alloc and free a config set")
  {
    INFO("Alloc and free a config set");
    simgrid::config::set_parse("peername:veloce user:bidule");
  }

  SECTION("Data retrieving tests")
  {
    INFO("Get a single value");
    /* get_single_value */
    simgrid::config::set_parse("peername:toto:42 speed:42");
    int ival = simgrid::config::get_value<int>("speed");
    REQUIRE(ival == 42); // Unexpected value for speed

    INFO("Access to a non-existent entry");

    REQUIRE_THROWS_AS(simgrid::config::set_parse("color:blue"), std::out_of_range);
  }

  SECTION("C++ flags")
  {
    INFO("C++ declaration of flags");

    simgrid::config::Flag<int> int_flag("int", "", 0);
    simgrid::config::Flag<std::string> string_flag("string", "", "foo");
    simgrid::config::Flag<double> double_flag("double", "", 0.32);
    simgrid::config::Flag<bool> bool_flag1("bool1", "", false);
    simgrid::config::Flag<bool> bool_flag2("bool2", "", true);

    INFO("Parse values");
    simgrid::config::set_parse("int:42 string:bar double:8.0 bool1:true bool2:false");
    REQUIRE(int_flag == 42);       // Check int flag
    REQUIRE(string_flag == "bar"); // Check string flag
    REQUIRE(double_flag == 8.0);   // Check double flag
    REQUIRE(bool_flag1);           // Check bool1 flag
    REQUIRE(not bool_flag2);       // Check bool2 flag
  }

  simgrid::config::finalize();
  simgrid_config = temp;
}
