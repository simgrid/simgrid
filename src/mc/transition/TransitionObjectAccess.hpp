/* Copyright (c) 2015-2023. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_MC_TRANSITION_OBJECT_ACCESS_HPP
#define SIMGRID_MC_TRANSITION_OBJECT_ACCESS_HPP

#include "src/mc/transition/Transition.hpp"

namespace simgrid::mc {
XBT_DECLARE_ENUM_CLASS(ObjectAccessType, ENTER, EXIT, BOTH);

class ObjectAccessTransition : public Transition {
  ObjectAccessType access_type_;
  void* objaddr_;
  std::string objname_;
  std::string file_;
  int line_;

public:
  ObjectAccessTransition(aid_t issuer, int times_considered, std::stringstream& stream);
  std::string to_string(bool verbose) const override;
  bool depends(const Transition* other) const override;
  bool reversible_race(const Transition* other) const override;
};

} // namespace simgrid::mc

#endif