/* xbt_str.cpp - various helping functions to deal with strings             */

/* Copyright (c) 2007-2020. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "xbt/str.h"

#include "simgrid/Exception.hpp"

#include "catch.hpp"
#include <string>
#include <vector>

namespace {
void test_split_quoted(const std::string& name, const char* input, const std::vector<std::string>& expected)
{
  INFO(name);
  xbt_dynar_t a = xbt_str_split_quoted(input);
  REQUIRE(xbt_dynar_length(a) == expected.size());
  unsigned i;
  char* token;
  xbt_dynar_foreach (a, i, token)
    REQUIRE(token == expected[i]);
  xbt_dynar_free(&a);
}

template <typename F> void test_parse_error(F function, const std::string& name, const char* str)
{
  INFO(name);
  REQUIRE_THROWS_AS(function(str, "Parse error"), std::invalid_argument);
}

template <typename F, typename T> void test_parse_ok(F function, const std::string& name, const char* str, T value)
{
  INFO(name);
  T variable = static_cast<T>(-9999);
  REQUIRE_NOTHROW(variable = function(str, "Parse error"));
  REQUIRE(variable == value); /* Fail to parse str */
}
}

TEST_CASE("xbt::str: String Handling", "xbt_str")
{

  SECTION("Test the function xbt_str_split_quoted")
  {
    test_split_quoted("Empty", "", {});
    test_split_quoted("Basic test", "toto tutu", {"toto", "tutu"});
    test_split_quoted("Useless backslashes", "\\t\\o\\t\\o \\t\\u\\t\\u", {"toto", "tutu"});
    test_split_quoted("Protected space", "toto\\ tutu", {"toto tutu"});
    test_split_quoted("Several spaces", "toto   tutu", {"toto", "tutu"});
    test_split_quoted("LTriming", "  toto tatu", {"toto", "tatu"});
    test_split_quoted("Triming", "  toto   tutu  ", {"toto", "tutu"});
    test_split_quoted("Single quotes", "'toto tutu' tata", {"toto tutu", "tata"});
    test_split_quoted("Double quotes", "\"toto tutu\" tata", {"toto tutu", "tata"});
    test_split_quoted("Mixed quotes", "\"toto' 'tutu\" tata", {"toto' 'tutu", "tata"});
    test_split_quoted("Backslashed quotes", "\\'toto tutu\\' tata", {"'toto", "tutu'", "tata"});
    test_split_quoted("Backslashed quotes + quotes", "'toto \\'tutu' tata", {"toto 'tutu", "tata"});
  }

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
