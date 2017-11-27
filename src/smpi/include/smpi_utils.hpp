/* Copyright (c) 2016-2017. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SMPI_UTILS_HPP
#define SMPI_UTILS_HPP
#include "xbt/base.h"
#include <cstddef>
#include <string>
#include <vector>

extern "C" {

// Methods used to parse and store the values for timing injections in smpi
struct s_smpi_factor_t {
  size_t factor = 0;
  std::vector<double> values;
};
typedef s_smpi_factor_t* smpi_os_factor_t;
}

XBT_PUBLIC(std::vector<s_smpi_factor_t>) parse_factor(std::string smpi_coef_string);

#endif
