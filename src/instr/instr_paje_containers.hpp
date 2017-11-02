/* Copyright (c) 2010-2017. The SimGrid Team. All rights reserved.          */

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
  sg_netpoint_t netpoint_ = nullptr;

  static Container* byNameOrNull(std::string name);
  static Container* byName(std::string name);
  std::string getName() { return name_; }
  const char* getCname() { return name_.c_str(); }
  long long int getId() { return id_; }
  void removeFromParent();
  void logCreation();
  void logDestruction();

  StateType* getState(std::string name);
  LinkType* getLink(std::string name);
  VariableType* getVariable(std::string name);

  static Container* getRoot();
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
