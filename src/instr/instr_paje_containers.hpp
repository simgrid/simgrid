/* Copyright (c) 2010-2021. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef INSTR_PAJE_CONTAINERS_HPP
#define INSTR_PAJE_CONTAINERS_HPP

#include "src/instr/instr_private.hpp"
#include <string>

namespace simgrid {
namespace instr {
class Type;
class LinkType;
class StateType;
class VariableType;

class Container {
  friend class NetZoneContainer;
  static Container* root_container_;
  static std::map<std::string, Container*, std::less<>> all_containers_;

  long long int id_;
  std::string name_; /* Unique name of this container */
  Type* type_;       /* Type of this container */
  Container* parent_;
  std::map<std::string, Container*, std::less<>> children_;

protected:
  static void set_root(Container* root) { root_container_ = root; }

public:
  static xbt::signal<void(Container const&)> on_creation;
  static xbt::signal<void(Container const&)> on_destruction;

  explicit Container(const std::string& name, const std::string& type_name, Container* parent);
  Container(const Container&) = delete;
  Container& operator=(const Container&) = delete;
  virtual ~Container();


  static Container* by_name_or_null(const std::string& name);
  static Container* by_name(const std::string& name);
  const std::string& get_name() const { return name_; }
  const char* get_cname() const { return name_.c_str(); }
  long long int get_id() const { return id_; }
  void remove_from_parent();

  Container* get_parent() const { return parent_; }
  Type* get_type() const { return type_; }
  StateType* get_state(const std::string& name);
  LinkType* get_link(const std::string& name);
  VariableType* get_variable(const std::string& name);
  void create_child(const std::string& name, const std::string& type_name);
  Container* get_child_by_name(const std::string& name) const { return children_.at(name); }
  static Container* get_root() { return root_container_; }
};

class NetZoneContainer : public Container {
public:
  NetZoneContainer(const std::string& name, unsigned int level, NetZoneContainer* parent);
};

class RouterContainer : public Container {
public:
  RouterContainer(const std::string& name, Container* parent);
};

class HostContainer : public Container {
public:
  HostContainer(s4u::Host const& host, NetZoneContainer* parent);
};
} // namespace instr
} // namespace simgrid
#endif
