/* Copyright (c) 2009-2021. The SimGrid Team.
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
  std::map<std::string, std::string, std::less<>> map_;
  int refcount_ = 1;

public:
  explicit Info() {this->add_f();}
  explicit Info(const Info* orig) : map_(orig->map_) {this->add_f();}
  void ref();
  static void unref(MPI_Info info);
  void set(const char* key, const char* value) { map_[key] = value; }
  int get(const char* key, int valuelen, char* value, int* flag) const;
  std::string name() const override {return std::string("MPI_Info");}
  int remove(const char* key);
  int get_nkeys(int* nkeys) const;
  int get_nthkey(int n, char* key) const;
  int get_valuelen(const char* key, int* valuelen, int* flag) const;
  static Info* f2c(int id);
};

}
}

#endif
