/* xbt_str.cpp - various helping functions to deal with strings             */

/* Copyright (c) 2007-2021. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/Exception.hpp"
#include "xbt/misc.h"
#include "xbt/str.h" /* headers of these functions */
#include "xbt/string.hpp"

/** @brief Parse an integer out of a string, or raise an error
 *
 * The @a str is passed as argument to your @a error_msg, as follows:
 * @verbatim throw std::invalid_argument(simgrid::xbt::string_printf(error_msg, str)); @endverbatim
 */
long int xbt_str_parse_int(const char* str, const char* error_msg)
{
  char* endptr;
  if (str == nullptr || str[0] == '\0')
    throw std::invalid_argument(simgrid::xbt::string_printf(error_msg, str));

  long int res = strtol(str, &endptr, 10);
  if (endptr[0] != '\0')
    throw std::invalid_argument(simgrid::xbt::string_printf(error_msg, str));

  return res;
}

/** @brief Parse a double out of a string, or raise an error
 *
 * The @a str is passed as argument to your @a error_msg, as follows:
 * @verbatim throw std::invalid_argument(simgrid::xbt::string_printf(error_msg, str)); @endverbatim
 */
double xbt_str_parse_double(const char* str, const char* error_msg)
{
  char *endptr;
  if (str == nullptr || str[0] == '\0')
    throw std::invalid_argument(simgrid::xbt::string_printf(error_msg, str));

  double res = strtod(str, &endptr);
  if (endptr[0] != '\0')
    throw std::invalid_argument(simgrid::xbt::string_printf(error_msg, str));

  return res;
}
