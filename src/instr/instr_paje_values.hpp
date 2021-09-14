/* Copyright (c) 2010-2021. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef INSTR_PAJE_VALUES_HPP
#define INSTR_PAJE_VALUES_HPP

#include "src/instr/instr_private.hpp"
#include <string>

namespace simgrid {
namespace instr {

class EntityValue {
  long long int id_ = new_paje_id();
  std::string name_;
  std::string color_;
  Type* parent_;

public:
  static xbt::signal<void(const EntityValue&)> on_creation;
  explicit EntityValue(const std::string& name, const std::string& color, Type* parent);

  long long int get_id() const { return id_; }
  std::string get_name() const { return name_; }
  const char* get_cname() const { return name_.c_str(); }
  std::string get_color() const { return color_; }
  Type* get_parent() const { return parent_; }
};
} // namespace instr
} // namespace simgrid

#endif
