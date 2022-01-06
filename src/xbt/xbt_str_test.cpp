/* xbt_str.cpp - various helping functions to deal with strings             */

/* Copyright (c) 2007-2022. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "xbt/parse_units.hpp"
#include "xbt/str.h"

#include "simgrid/Exception.hpp"

#include "catch.hpp"
#include <string>
#include <vector>

namespace {
template <typename F> void test_parse_error(F function, const std::string& name, const char* str)
{
  INFO(name);
  REQUIRE_THROWS_AS(function(str, "Parse error"), std::invalid_argument);
}

template <typename F, typename T> void test_parse_ok(F function, const std::string& name, const char* str, T value)
{
  INFO(name);
  T variable;
  REQUIRE_NOTHROW(variable = function(str, "Parse error"));
  REQUIRE(variable == value); /* Fail to parse str */
}
}

TEST_CASE("xbt::str: String Handling", "xbt_str")
{
  SECTION("Test the parsing functions")
  {
    test_parse_ok(xbt_str_parse_int, "Parse int", "42", 42);
    test_parse_ok(xbt_str_parse_int, "Parse 0 as an int", "0", 0);
    test_parse_ok(xbt_str_parse_int, "Parse -1 as an int", "-1", -1);

    test_parse_error(xbt_str_parse_int, "Parse int + noise", "342 cruft");
    test_parse_error(xbt_str_parse_int, "Parse nullptr as an int", nullptr);
    test_parse_error(xbt_str_parse_int, "Parse '' as an int", "");
    test_parse_error(xbt_str_parse_int, "Parse cruft as an int", "cruft");

    test_parse_ok(xbt_str_parse_double, "Parse 42 as a double", "42", 42);
    test_parse_ok(xbt_str_parse_double, "Parse 42.5 as a double", "42.5", 42.5);
    test_parse_ok(xbt_str_parse_double, "Parse 0 as a double", "0", 0);
    test_parse_ok(xbt_str_parse_double, "Parse -1 as a double", "-1", -1);

    test_parse_error(xbt_str_parse_double, "Parse double + noise", "342 cruft");
    test_parse_error(xbt_str_parse_double, "Parse nullptr as a double", nullptr);
    test_parse_error(xbt_str_parse_double, "Parse '' as a double", "");
    test_parse_error(xbt_str_parse_double, "Parse cruft as a double", "cruft");
  }

  SECTION("Test the parsing-with-units functions")
  {
    REQUIRE(xbt_parse_get_all_speeds(__FILE__, __LINE__, "1.0", "") == std::vector<double>{1e0});
    REQUIRE(xbt_parse_get_all_speeds(__FILE__, __LINE__, "1.0,2.0", "") == std::vector<double>{1e0, 2e0});
    REQUIRE(xbt_parse_get_all_speeds(__FILE__, __LINE__, "1.0,2.0,3.0", "") == std::vector<double>{1e0, 2e0, 3e0});

    REQUIRE(xbt_parse_get_all_speeds(__FILE__, __LINE__, "1", "") == std::vector<double>{1e0});
    REQUIRE(xbt_parse_get_all_speeds(__FILE__, __LINE__, "1,2", "") == std::vector<double>{1.0, 2.0});
    REQUIRE(xbt_parse_get_all_speeds(__FILE__, __LINE__, "1,2,3", "") == std::vector<double>{1.0, 2.0, 3.0});

    REQUIRE(xbt_parse_get_all_speeds(__FILE__, __LINE__, "1.0f", "") == std::vector<double>{1e0});
    REQUIRE(xbt_parse_get_all_speeds(__FILE__, __LINE__, "1.0kf,2.0Mf", "") == std::vector<double>{1e3, 2e6});
    REQUIRE(xbt_parse_get_all_speeds(__FILE__, __LINE__, "1.0Gf,2.0Tf,3.0Pf", "") ==
            std::vector<double>{1e9, 2e12, 3e15});

    REQUIRE(xbt_parse_get_all_speeds(__FILE__, __LINE__, "1f", "") == std::vector<double>{1e0});
    REQUIRE(xbt_parse_get_all_speeds(__FILE__, __LINE__, "1kf,2Gf", "") == std::vector<double>{1e3, 2e9});
    REQUIRE(xbt_parse_get_all_speeds(__FILE__, __LINE__, "1Ef,2Zf,3Yf", "") == std::vector<double>{1e18, 2e21, 3e24});
  }
}
