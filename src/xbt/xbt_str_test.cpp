/* xbt_str.cpp - various helping functions to deal with strings             */

/* Copyright (c) 2007-2021. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

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
}
