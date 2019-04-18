/* Copyright (c) 2010-2019. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef INSTR_PAJE_VALUES_HPP
#define INSTR_PAJE_VALUES_HPP

#include "src/instr/instr_private.hpp"
#include <string>

namespace simgrid {
namespace instr {

class EntityValue {
  long long int id_;
  std::string name_;
  std::string color_;
  Type* father_;

public:
  explicit EntityValue(const std::string& name, const std::string& color, Type* father);
  const char* get_cname() { return name_.c_str(); }
  long long int get_id() { return id_; }
  void print();
};
}
}

#endif
