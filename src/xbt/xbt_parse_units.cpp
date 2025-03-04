/* Copyright (c) 2007-2025. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/Exception.hpp"
#include "xbt/ex.h"
#include "xbt/log.h"

#include "xbt/parse_units.hpp"

#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>
#include <string>
#include <unordered_map>
#include <vector>

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(parse, xbt, "Parsing functions");

class unit_scale : public std::unordered_map<std::string, double> {
public:
  using std::unordered_map<std::string, double>::unordered_map;
  // tuples are : <unit, value for unit, base (2 or 10), true if abbreviated>
  explicit unit_scale(std::initializer_list<std::tuple<const std::string, double, int, bool>> generators);
};

unit_scale::unit_scale(std::initializer_list<std::tuple<const std::string, double, int, bool>> generators)
{
  for (auto [unit, value, base, abbrev] : generators) {
    double mult;
    std::vector<std::string> prefixes;
    switch (base) {
      case 2:
        mult     = 1024.0;
        prefixes = abbrev ? std::vector<std::string>{"Ki", "Mi", "Gi", "Ti", "Pi", "Ei", "Zi", "Yi"}
                          : std::vector<std::string>{"kibi", "mebi", "gibi", "tebi", "pebi", "exbi", "zebi", "yobi"};
        break;
      case 10:
        mult     = 1000.0;
        prefixes = abbrev ? std::vector<std::string>{"k", "M", "G", "T", "P", "E", "Z", "Y"}
                          : std::vector<std::string>{"kilo", "mega", "giga", "tera", "peta", "exa", "zeta", "yotta"};
        break;
      default:
        THROW_IMPOSSIBLE;
    }
    emplace(unit, value);
    for (const auto& prefix : prefixes) {
      value *= mult;
      emplace(prefix + unit, value);
    }
  }
}

/* Note: no warning is issued for unit-less values when `entity_kind' is empty. */
static double xbt_parse_get_value_with_unit(const std::string& filename, int lineno, const std::string& string,
                                            const unit_scale& units, const std::string& entity_kind,
                                            const char* error_msg, const char* default_unit)
{
  char* endptr;
  errno           = 0;
  double res      = strtod(string.c_str(), &endptr);
  const char* ptr = endptr; // for const-correctness
  if (errno == ERANGE)
    throw simgrid::ParseError(filename, lineno, "value out of range: " + string);
  if (ptr == string)
    throw simgrid::ParseError(filename, lineno, "cannot parse number:" + string);
  if (ptr[0] == '\0') {
    // Ok, 0 can be unit-less
    if (res != 0 && not entity_kind.empty())
      XBT_WARN("Deprecated unit-less value '%s' for %s. %s", string.c_str(), entity_kind.c_str(), error_msg);
    ptr = default_unit;
  }
  auto u = units.find(ptr);
  if (u == units.end())
    throw simgrid::ParseError(filename, lineno, std::string("unknown unit: ") + ptr);
  return res * u->second;
}

double xbt_parse_get_time(const std::string& filename, int lineno, const std::string& string,
                          const std::string& entity_kind)
{
  static const unit_scale units{std::make_pair("w", 7 * 24 * 60 * 60),
                                std::make_pair("d", 24 * 60 * 60),
                                std::make_pair("h", 60 * 60),
                                std::make_pair("m", 60),
                                std::make_pair("s", 1.0),
                                std::make_pair("ms", 1e-3),
                                std::make_pair("us", 1e-6),
                                std::make_pair("ns", 1e-9),
                                std::make_pair("ps", 1e-12)};
  return xbt_parse_get_value_with_unit(filename, lineno, string, units, entity_kind,
                                       "Append 's' to your time to get seconds", "s");
}

double xbt_parse_get_size(const std::string& filename, int lineno, const std::string& string,
                          const std::string& entity_kind)
{
  static const unit_scale units{std::make_tuple("b", 0.125, 2, true), std::make_tuple("b", 0.125, 10, true),
                                std::make_tuple("B", 1.0, 2, true), std::make_tuple("B", 1.0, 10, true)};
  return xbt_parse_get_value_with_unit(filename, lineno, string, units, entity_kind,
                                       "Append 'B' to get bytes (or 'b' for bits but 1B = 8b).", "B");
}

double xbt_parse_get_bandwidth(const std::string& filename, int lineno, const std::string& string,
                               const std::string& entity_kind)
{
  static const unit_scale units{std::make_tuple("bps", 0.125, 2, true), std::make_tuple("bps", 0.125, 10, true),
                                std::make_tuple("Bps", 1.0, 2, true), std::make_tuple("Bps", 1.0, 10, true)};
  return xbt_parse_get_value_with_unit(filename, lineno, string, units, entity_kind,
                                       "Append 'Bps' to get bytes per second (or 'bps' for bits but 1Bps = 8bps)",
                                       "Bps");
}

std::vector<double> xbt_parse_get_bandwidths(const std::string& filename, int lineno, const std::string& string,
                                             const std::string& entity_kind)
{
  static const unit_scale units{std::make_tuple("bps", 0.125, 2, true), std::make_tuple("bps", 0.125, 10, true),
                                std::make_tuple("Bps", 1.0, 2, true), std::make_tuple("Bps", 1.0, 10, true)};

  std::vector<double> bandwidths;
  std::vector<std::string> tokens;
  boost::split(tokens, string, boost::is_any_of(";,"));
  for (auto const& token : tokens) {
    bandwidths.push_back(xbt_parse_get_value_with_unit(
        filename, lineno, token.c_str(), units, entity_kind,
        "Append 'Bps' to get bytes per second (or 'bps' for bits but 1Bps = 8bps)", "Bps"));
  }

  return bandwidths;
}

double xbt_parse_get_speed(const std::string& filename, int lineno, const std::string& string,
                           const std::string& entity_kind)
{
  static const unit_scale units{std::make_tuple("f", 1.0, 10, true), std::make_tuple("flops", 1.0, 10, false)};
  return xbt_parse_get_value_with_unit(filename, lineno, string, units, entity_kind,
                                       "Append 'f' or 'flops' to your speed to get flop per second", "f");
}

std::vector<double> xbt_parse_get_all_speeds(const std::string& filename, int lineno, const std::string& speeds,
                                             const std::string& entity_kind)
{
  std::vector<double> speed_per_pstate;
  std::vector<std::string> pstate_list;

  boost::split(pstate_list, speeds, boost::is_any_of(","));
  for (auto speed_str : pstate_list) {
    boost::trim(speed_str);
    double speed = xbt_parse_get_speed(filename, lineno, speed_str, entity_kind);
    speed_per_pstate.push_back(speed);
    XBT_DEBUG("Speed value: %f", speed);
  }

  return speed_per_pstate;
}
