/* Copyright (c) 2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_HOST_HPP
#define SIMGRID_HOST_HPP

#include <cstddef>
#include <memory>
#include <string>
#include <vector>

#include <xbt/base.h>
#include <xbt/string.hpp>
#include <xbt/Extendable.hpp>

namespace simgrid {

XBT_PUBLIC_CLASS Host :
	public simgrid::xbt::Extendable<Host> {

	public:
	surf::Cpu *p_cpu;

private:
  simgrid::xbt::string name_;
public:
  Host(std::string const& name);
  ~Host();
  simgrid::xbt::string const& getName() const { return name_; }
  static Host* by_name_or_null(const char* name);
  static Host* by_name_or_create(const char* name);
};

}

#endif
