/* Copyright (c) 2010-2017. The SimGrid Team. All rights reserved.          */

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
  explicit EntityValue(std::string name, std::string color, Type* father);
  ~EntityValue() = default;
  const char* getCname() { return name_.c_str(); }
  long long int getId() { return id_; }
  void print();
};
}
}

#endif
