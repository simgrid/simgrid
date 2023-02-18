/* Copyright (c) 2016-2023. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/kernel/resource/FactorSet.hpp"
#include "xbt/ex.h"
#include "xbt/file.hpp"
#include "xbt/log.h"
#include "xbt/parse_units.hpp"
#include "xbt/str.h"
#include "xbt/sysdep.h"

#include <algorithm>
#include <boost/tokenizer.hpp>

XBT_LOG_EXTERNAL_DEFAULT_CATEGORY(ker_resource);

extern std::string simgrid_parsed_filename;
extern int simgrid_parse_lineno;

namespace simgrid::kernel::resource {

void FactorSet::parse(const std::string& values)
{
  initialized_    = true;

  if (values.find_first_of(":;") == std::string::npos) { // Single value
    default_value_ = xbt_str_parse_double(values.c_str(), name_.c_str());
    return;
  }

  /** Setup the tokenizer that parses the string **/
  using Tokenizer = boost::tokenizer<boost::char_separator<char>>;
  boost::char_separator<char> sep(";");
  boost::char_separator<char> factor_separator(":");
  Tokenizer tokens(values, sep);

  /**
   * Iterate over patterns like A:B:C:D;E:F;G:H
   * These will be broken down into:
   * A --> B, C, D
   * E --> F
   * G --> H
   */
  for (auto token_iter = tokens.begin(); token_iter != tokens.end(); ++token_iter) {
    XBT_DEBUG("token: %s", token_iter->c_str());
    Tokenizer factor_values(*token_iter, factor_separator);
    s_smpi_factor_t fact;
    xbt_assert(factor_values.begin() != factor_values.end(), "Malformed radical for %s: '%s'", name_.c_str(),
               values.c_str());
    unsigned int iteration = 0;
    for (auto factor_iter = factor_values.begin(); factor_iter != factor_values.end(); ++factor_iter) {
      iteration++;

      if (factor_iter == factor_values.begin()) { /* first element */
        try {
          fact.factor = std::stoi(*factor_iter);
        } catch (const std::invalid_argument&) {
          throw std::invalid_argument("Invalid factor in chunk " + std::to_string(factors_.size() + 1) + ": " +
                                      *factor_iter + " for " + name_);
        }
      } else {
        try {
          fact.values.push_back(xbt_parse_get_time(simgrid_parsed_filename, simgrid_parse_lineno, *factor_iter, ""));
        } catch (const std::invalid_argument&) {
          throw std::invalid_argument("Invalid factor value " + std::to_string(iteration) + " in chunk " +
                                      std::to_string(factors_.size() + 1) + ": " + *factor_iter + " for " + name_);
        }
      }
    }

    factors_.push_back(fact);
    XBT_DEBUG("smpi_factor:\t%zu: %zu values, first: %f", fact.factor, factors_.size(), fact.values[0]);
  }
  std::sort(factors_.begin(), factors_.end(),
            [](const s_smpi_factor_t& pa, const s_smpi_factor_t& pb) { return (pa.factor < pb.factor); });
  for (auto const& fact : factors_) {
    XBT_DEBUG("smpi_factor:\t%zu: %zu values, first: %f", fact.factor, factors_.size(), fact.values[0]);
  }
  factors_.shrink_to_fit();
}

FactorSet::FactorSet(const std::string& name, double default_value,
                     std::function<double(std::vector<double> const&, double)> const& lambda)
    : name_(name), default_value_(default_value), lambda_(lambda)
{
}

double FactorSet::operator()() const
{
  return default_value_;
}

double FactorSet::operator()(double size) const
{
  if (factors_.empty())
    return default_value_;

  for (long unsigned i = 0; i < factors_.size(); i++) {
    auto const& fact = factors_[i];

    if (size <= fact.factor) { // Too large already, use the previous value
      if (i == 0) { // Before the first boundary: use the default value
        XBT_DEBUG("%s: %f <= %zu return default %f", name_.c_str(), size, fact.factor, default_value_);
        return default_value_;
      }
      double val = lambda_(factors_[i - 1].values, size);
      XBT_DEBUG("%s: %f <= %zu return %f", name_.c_str(), size, fact.factor, val);
      return val;
    }
  }
  double val = lambda_(factors_.back().values, size);

  XBT_DEBUG("%s: %f > %zu return %f", name_.c_str(), size, factors_.back().factor, val);
  return val;
}
} // namespace simgrid::kernel::resource
