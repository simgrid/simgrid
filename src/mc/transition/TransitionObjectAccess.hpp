/* Copyright (c) 2015-2025. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_MC_TRANSITION_OBJECT_ACCESS_HPP
#define SIMGRID_MC_TRANSITION_OBJECT_ACCESS_HPP

#include "src/mc/transition/Transition.hpp"
#include <memory>

namespace simgrid::mc {
XBT_DECLARE_ENUM_CLASS(ObjectAccessType, ENTER, EXIT, BOTH);

class ObjectAccessTransition : public Transition {
  struct data {
    ObjectAccessType access_type_;
    void* objaddr_;
    std::string objname_;
    std::string file_;
    int line_;
  };
  std::unique_ptr<data> data_;

public:
  ObjectAccessTransition(aid_t issuer, int times_considered, mc::Channel& channel);
  std::string to_string(bool verbose) const override;
  bool depends(const Transition* other) const override
  {
    if (other->type_ == Type::OBJECT_ACCESS) // dependent only if it's an access to the same object
      return data_->objaddr_ == static_cast<const ObjectAccessTransition*>(other)->data_->objaddr_;
    return false;
  }

  bool reversible_race(const Transition* other, const odpor::Execution* exec, EventHandle this_handle,
                       EventHandle other_handle) const override;
};

} // namespace simgrid::mc

#endif
