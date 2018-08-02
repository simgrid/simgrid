/* Copyright (c) 2017-2018. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_S4U_IO_HPP
#define SIMGRID_S4U_IO_HPP

#include <simgrid/forward.h>
#include <simgrid/s4u/Activity.hpp>

#include <atomic>

namespace simgrid {
namespace s4u {

class XBT_PUBLIC Io : public Activity {
  Io() : Activity() {}
public:
  friend XBT_PUBLIC void intrusive_ptr_release(simgrid::s4u::Io* i);
  friend XBT_PUBLIC void intrusive_ptr_add_ref(simgrid::s4u::Io* i);
  friend Storage; // Factory of IOs

  enum class OpType { READ, WRITE };
  ~Io() = default;

  Activity* start() override;
  Activity* wait() override;
  Activity* wait(double timeout) override;
  Activity* cancel() override;

  double get_remaining() override;
  IoPtr set_io_type(OpType type);

private:
  sg_size_t size_       = 0;
  sg_storage_t storage_ = nullptr;
  std::string name_     = "";
  OpType type_          = OpType::READ;
  std::atomic_int_fast32_t refcount_{0};
}; // class
}
}; // Namespace simgrid::s4u

#endif /* SIMGRID_S4U_IO_HPP */
