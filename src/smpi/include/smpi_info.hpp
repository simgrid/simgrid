/* Copyright (c) 2009-2020. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SMPI_INFO_HPP
#define SMPI_INFO_HPP

#include "smpi/smpi.h"
#include "smpi_f2c.hpp"
#include <string>
#include <map>

namespace simgrid{
namespace smpi{

class Info : public F2C{
  std::map<std::string, std::string> map_;
  int refcount_ = 1;

public:
  Info() = default;
  explicit Info(const Info* orig) : map_(orig->map_) {}
  ~Info() = default;
  void ref();
  static void unref(MPI_Info info);
  void set(const char* key, const char* value) { map_[key] = value; }
  int get(const char* key, int valuelen, char* value, int* flag);
  int remove(const char* key);
  int get_nkeys(int* nkeys);
  int get_nthkey(int n, char* key);
  int get_valuelen(const char* key, int* valuelen, int* flag);
  static Info* f2c(int id);
};

}
}

#endif
