/* Copyright (c) 2010-2018. The SimGrid Team. All rights reserved.          */

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
  long long int id_;
  std::string name_; /* Unique name of this container */
public:
  Container(std::string name, std::string type_name, Container* father);
  virtual ~Container();

  Type* type_; /* Type of this container */
  Container* father_;
  std::map<std::string, Container*> children_;
  kernel::routing::NetPoint* netpoint_ = nullptr;

  static Container* by_name_or_null(std::string name);
  static Container* by_name(std::string name);
  std::string get_name() { return name_; }
  const char* get_cname() { return name_.c_str(); }
  long long int get_id() { return id_; }
  void remove_from_parent();
  void log_creation();
  void log_destruction();

  StateType* get_state(std::string name);
  LinkType* get_link(std::string name);
  VariableType* get_variable(std::string name);
  void create_child(std::string name, std::string type_name);
  static Container* get_root();
};

class NetZoneContainer : public Container {
public:
  NetZoneContainer(std::string name, unsigned int level, NetZoneContainer* father);
};

class RouterContainer : public Container {
public:
  RouterContainer(std::string name, Container* father);
};

class HostContainer : public Container {
public:
  HostContainer(simgrid::s4u::Host& host, NetZoneContainer* father);
};
}
}
#endif
