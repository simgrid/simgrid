/* Copyright (c) 2016. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_MC_CHECKER_HPP
#define SIMGRID_MC_CHECKER_HPP

#include <functional>
#include <memory>

#include "src/mc/mc_forward.hpp"

namespace simgrid {
namespace mc {

/** A model-checking algorithm
 *
 *  The goal is to move the data/state/configuration of a model-checking
 *  algorihms in subclasses. Implementing this interface will probably
 *  not be really mandatory, you might be able to write your
 *  model-checking algorithm as plain imperative code instead.
 *
 *  It works by manipulating a model-checking Session.
 */
// abstract
class Checker {
  Session* session_;
public:
  Checker(Session& session) : session_(&session) {}

  // No copy:
  Checker(Checker const&) = delete;
  Checker& operator=(Checker const&) = delete;

  virtual ~Checker();
  virtual int run() = 0;

protected:
  Session& getSession() { return *session_; }
};

}
}

#endif
