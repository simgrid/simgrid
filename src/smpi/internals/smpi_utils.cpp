/* Copyright (c) 2016-2021. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "smpi_utils.hpp"

#include "src/surf/xml/platf_private.hpp"
#include "xbt/log.h"
#include "xbt/parse_units.hpp"
#include "xbt/sysdep.h"
#include "xbt/file.hpp"
#include <boost/tokenizer.hpp>
#include "smpi_config.hpp"
#include "smpi_f2c.hpp"
#include "src/simix/smx_private.hpp"

#include <algorithm>

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(smpi_utils, smpi, "Logging specific to SMPI (utils)");

extern std::string surf_parsed_filename;
extern int surf_parse_lineno;

namespace simgrid {
namespace smpi {
namespace utils {

double total_benched_time=0;
unsigned long total_malloc_size=0;
unsigned long total_shared_size=0;
unsigned int total_shared_calls=0;
struct MaxMalloc {
  size_t size          = 0;
  unsigned int numcall = 0;
  int line             = 0;
  std::string file;
};
MaxMalloc max_malloc;

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
    xbt_assert(factor_values.begin() != factor_values.end(), "Malformed radical for smpi factor: '%s'",
               smpi_coef_string.c_str());
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

void add_benched_time(double time){
  total_benched_time += time;
}

void account_malloc_size(size_t size, const char* file, int line){
  total_malloc_size += size;
  if(size > max_malloc.size){
    max_malloc.size = size;
    max_malloc.line = line;
    max_malloc.numcall = 1;
    max_malloc.file = std::string(file);
  }else if(size == max_malloc.size && max_malloc.line == line && not max_malloc.file.compare(file)){
    max_malloc.numcall++;
  }
}

void account_shared_size(size_t size){
  total_shared_size += size;
  total_shared_calls++;
}

void print_time_analysis(double global_time){
  if (simgrid::config::get_value<bool>("smpi/display-timing")) {
    XBT_INFO("Simulated time: %g seconds. \n\n"
        "The simulation took %g seconds (after parsing and platform setup)\n"
        "%g seconds were actual computation of the application",
        SIMIX_get_clock(), global_time , total_benched_time);
    if (total_benched_time/global_time>=0.75)
      XBT_INFO("More than 75%% of the time was spent inside the application code.\n"
    "You may want to use sampling functions or trace replay to reduce this.");
  }
}

void print_memory_analysis()
{
  size_t leak_count = 0;
  if (simgrid::smpi::F2C::lookup() != nullptr)
    leak_count =
        std::count_if(simgrid::smpi::F2C::lookup()->cbegin(), simgrid::smpi::F2C::lookup()->cend(),
                      [](auto const& entry) { return entry.first >= simgrid::smpi::F2C::get_num_default_handles(); });
  if (leak_count > 0) {
    XBT_INFO("Probable memory leaks in your code: SMPI detected %zu unfreed MPI handles : "
             "display types and addresses (n max) with --cfg=smpi/list-leaks:n.\n"
             "Running smpirun with -wrapper \"valgrind --leak-check=full\" can provide more information",
             leak_count);
    int n = simgrid::config::get_value<int>("smpi/list-leaks");
    for (auto const& p : *simgrid::smpi::F2C::lookup()) {
      static int printed = 0;
      if (printed >= n)
        break;
      if (p.first >= simgrid::smpi::F2C::get_num_default_handles()) {
        if (xbt_log_no_loc) {
          XBT_WARN("Leaked handle of type %s", boost::core::demangle(typeid(*(p.second)).name()).c_str());
        } else {
          XBT_WARN("Leaked handle of type %s at %p", boost::core::demangle(typeid(*(p.second)).name()).c_str(),
                   p.second);
        }
        printed++;
      }
    }
  }
  if (simgrid::config::get_value<bool>("smpi/display-allocs")) {
    if(total_malloc_size != 0)
      XBT_INFO("Memory Usage: Simulated application allocated %lu bytes during its lifetime through malloc/calloc calls.\n"
             "Largest allocation at once from a single process was %zu bytes, at %s:%d. It was called %u times during the whole simulation.\n"
             "If this is too much, consider sharing allocations for computation buffers.\n"
             "This can be done automatically by setting --cfg=smpi/auto-shared-malloc-thresh to the minimum size wanted size (this can alter execution if data content is necessary)\n",
             total_malloc_size, max_malloc.size, simgrid::xbt::Path(max_malloc.file).get_base_name().c_str(), max_malloc.line, max_malloc.numcall
      );
    else
      XBT_INFO("Allocations analysis asked, but 0 bytes were allocated through malloc/calloc calls intercepted by SMPI.\n"
               "Either code is using other ways of allocating memory, or it was built with SMPI_NO_OVERRIDE_MALLOC");
    if(total_shared_size != 0)
      XBT_INFO("%lu bytes were automatically shared between processes, in %u calls\n", total_shared_size, total_shared_calls);
  }
}
}
}
} // namespace simgrid
