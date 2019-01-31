/* xbt_str.cpp - various helping functions to deal with strings             */

/* Copyright (c) 2007-2019. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "xbt/str.h"

#include "simgrid/Exception.hpp"

#include "catch.hpp"

#define mytest(name, input, expected)                                                                                  \
  INFO(name);                                                                                                          \
  a = static_cast<char**>(xbt_dynar_to_array(xbt_str_split_quoted(input)));                                            \
  s = xbt_str_join_array(a, "XXX");                                                                                    \
  REQUIRE(not strcmp(s, expected));                                                                                    \
  xbt_free(s);                                                                                                         \
  for (int i = 0; a[i] != nullptr; i++)                                                                                \
    xbt_free(a[i]);                                                                                                    \
  xbt_free(a);

#define test_parse_error(function, name, variable, str)                                                                \
  do {                                                                                                                 \
    INFO(name);                                                                                                        \
    REQUIRE_THROWS_MATCHES(variable = function(str, "Parse error"), xbt_ex,                                            \
                           Catch::Matchers::Predicate<xbt_ex>([](xbt_ex const& e) { return e.category == arg_error; }, \
                                                              "category arg_error"));                                  \
  } while (0)

#define test_parse_ok(function, name, variable, str, value)                                                            \
  do {                                                                                                                 \
    INFO(name);                                                                                                        \
    REQUIRE_NOTHROW(variable = function(str, "Parse error"));                                                          \
    REQUIRE(variable == value); /* Fail to parse str */                                                                \
  } while (0)

TEST_CASE("xbt::str: String Handling", "xbt_str")
{

  SECTION("Test the function xbt_str_split_quoted")
  {
    char** a;
    char* s;

    mytest("Empty", "", "");
    mytest("Basic test", "toto tutu", "totoXXXtutu");
    mytest("Useless backslashes", "\\t\\o\\t\\o \\t\\u\\t\\u", "totoXXXtutu");
    mytest("Protected space", "toto\\ tutu", "toto tutu");
    mytest("Several spaces", "toto   tutu", "totoXXXtutu");
    mytest("LTriming", "  toto tatu", "totoXXXtatu");
    mytest("Triming", "  toto   tutu  ", "totoXXXtutu");
    mytest("Single quotes", "'toto tutu' tata", "toto tutuXXXtata");
    mytest("Double quotes", "\"toto tutu\" tata", "toto tutuXXXtata");
    mytest("Mixed quotes", "\"toto' 'tutu\" tata", "toto' 'tutuXXXtata");
    mytest("Backslashed quotes", "\\'toto tutu\\' tata", "'totoXXXtutu'XXXtata");
    mytest("Backslashed quotes + quotes", "'toto \\'tutu' tata", "toto 'tutuXXXtata");
  }

  SECTION("Test the parsing functions")
  {
    int rint = -9999;
    test_parse_ok(xbt_str_parse_int, "Parse int", rint, "42", 42);
    test_parse_ok(xbt_str_parse_int, "Parse 0 as an int", rint, "0", 0);
    test_parse_ok(xbt_str_parse_int, "Parse -1 as an int", rint, "-1", -1);

    test_parse_error(xbt_str_parse_int, "Parse int + noise", rint, "342 cruft");
    test_parse_error(xbt_str_parse_int, "Parse nullptr as an int", rint, nullptr);
    test_parse_error(xbt_str_parse_int, "Parse '' as an int", rint, "");
    test_parse_error(xbt_str_parse_int, "Parse cruft as an int", rint, "cruft");

    double rdouble = -9999;
    test_parse_ok(xbt_str_parse_double, "Parse 42 as a double", rdouble, "42", 42);
    test_parse_ok(xbt_str_parse_double, "Parse 42.5 as a double", rdouble, "42.5", 42.5);
    test_parse_ok(xbt_str_parse_double, "Parse 0 as a double", rdouble, "0", 0);
    test_parse_ok(xbt_str_parse_double, "Parse -1 as a double", rdouble, "-1", -1);

    test_parse_error(xbt_str_parse_double, "Parse double + noise", rdouble, "342 cruft");
    test_parse_error(xbt_str_parse_double, "Parse nullptr as a double", rdouble, nullptr);
    test_parse_error(xbt_str_parse_double, "Parse '' as a double", rdouble, "");
    test_parse_error(xbt_str_parse_double, "Parse cruft as a double", rdouble, "cruft");
  }
}
