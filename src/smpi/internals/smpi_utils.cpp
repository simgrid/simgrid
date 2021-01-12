/* Copyright (c) 2016-2021. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "smpi_utils.hpp"

#include "src/surf/xml/platf_private.hpp"
#include "xbt/log.h"
#include "xbt/parse_units.hpp"
#include "xbt/sysdep.h"
#include <boost/tokenizer.hpp>

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(smpi_utils, smpi, "Logging specific to SMPI (utils)");

extern std::string surf_parsed_filename;
extern int surf_parse_lineno;

std::vector<s_smpi_factor_t> parse_factor(const std::string& smpi_coef_string)
{
  std::vector<s_smpi_factor_t> smpi_factor;

  /** Setup the tokenizer that parses the string **/
  using Tokenizer = boost::tokenizer<boost::char_separator<char>>;
  boost::char_separator<char> sep(";");
  boost::char_separator<char> factor_separator(":");
  Tokenizer tokens(smpi_coef_string, sep);

  /**
   * Iterate over patterns like A:B:C:D;E:F;G:H
   * These will be broken down into:
   * A --> B, C, D
   * E --> F
   * G --> H
   */
  for (Tokenizer::iterator token_iter = tokens.begin(); token_iter != tokens.end(); ++token_iter) {
    XBT_DEBUG("token : %s", token_iter->c_str());
    Tokenizer factor_values(*token_iter, factor_separator);
    s_smpi_factor_t fact;
    if (factor_values.begin() == factor_values.end()) {
      xbt_die("Malformed radical for smpi factor: '%s'", smpi_coef_string.c_str());
    }
    unsigned int iteration = 0;
    for (Tokenizer::iterator factor_iter = factor_values.begin(); factor_iter != factor_values.end(); ++factor_iter) {
      iteration++;

      if (factor_iter == factor_values.begin()) { /* first element */
        try {
          fact.factor = std::stoi(*factor_iter);
        } catch (const std::invalid_argument&) {
          throw std::invalid_argument(std::string("Invalid factor in chunk ") + std::to_string(smpi_factor.size() + 1) +
                                      ": " + *factor_iter);
        }
      } else {
        try {
          fact.values.push_back(
              xbt_parse_get_time(surf_parsed_filename, surf_parse_lineno, (*factor_iter).c_str(), "smpi factor", ""));
        } catch (const std::invalid_argument&) {
          throw std::invalid_argument(std::string("Invalid factor value ") + std::to_string(iteration) + " in chunk " +
                                      std::to_string(smpi_factor.size() + 1) + ": " + *factor_iter);
        }
      }
    }

    smpi_factor.push_back(fact);
    XBT_DEBUG("smpi_factor:\t%zu : %zu values, first: %f", fact.factor, smpi_factor.size(), fact.values[0]);
  }
  std::sort(smpi_factor.begin(), smpi_factor.end(), [](const s_smpi_factor_t &pa, const s_smpi_factor_t &pb) {
    return (pa.factor < pb.factor);
  });
  for (auto const& fact : smpi_factor) {
    XBT_DEBUG("smpi_factor:\t%zu : %zu values, first: %f", fact.factor, smpi_factor.size() ,fact.values[0]);
  }
  smpi_factor.shrink_to_fit();

  return smpi_factor;
}
