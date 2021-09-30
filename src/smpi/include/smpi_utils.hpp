/* Copyright (c) 2016-2021. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SMPI_UTILS_HPP
#define SMPI_UTILS_HPP
#include <xbt/base.h>

#include "smpi_f2c.hpp"

#include <cstddef>
#include <string>
#include <vector>

// Methods used to parse and store the values for timing injections in smpi
struct s_smpi_factor_t {
  size_t factor = 0;
  std::vector<double> values;
};

namespace simgrid {
namespace smpi {
namespace utils {

XBT_PUBLIC std::vector<s_smpi_factor_t> parse_factor(const std::string& smpi_coef_string);
XBT_PUBLIC void add_benched_time(double time);
XBT_PUBLIC void account_malloc_size(size_t size, const std::string& file, int line, void* ptr);
XBT_PUBLIC void account_shared_size(size_t size);
XBT_PUBLIC void print_time_analysis(double time);
XBT_PUBLIC void print_buffer_info();
XBT_PUBLIC void print_memory_analysis();
XBT_PUBLIC void print_current_handle();
XBT_PUBLIC void set_current_handle(F2C* handle);
XBT_PUBLIC void set_current_buffer(int i, const char* name, const void* handle);
XBT_PUBLIC size_t get_buffer_size(const void* ptr);
XBT_PUBLIC void account_free(const void* ptr);

} // namespace utils
} // namespace smpi
} // namespace simgrid
#endif
