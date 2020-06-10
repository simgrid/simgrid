/* Copyright (c) 2007-2020. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_XBT_PARSE_UNITS_HPP
#define SIMGRID_XBT_PARSE_UNITS_HPP

double xbt_parse_get_time(const std::string& filename, int lineno, const char* string, const char* entity_kind,
                          const std::string& name);
double surf_parse_get_size(const std::string& filename, int lineno, const char* string, const char* entity_kind,
                           const std::string& name);
double xbt_parse_get_bandwidth(const std::string& filename, int lineno, const char* string, const char* entity_kind,
                               const std::string& name);
std::vector<double> xbt_parse_get_bandwidths(const std::string& filename, int lineno, const char* string,
                                             const char* entity_kind, const std::string& name);
double xbt_parse_get_speed(const std::string& filename, int lineno, const char* string, const char* entity_kind,
                           const std::string& name);
std::vector<double> xbt_parse_get_all_speeds(const std::string& filename, int lineno, char* speeds,
                                             const char* entity_kind, const std::string& id);

#endif
