/* Copyright (c) 2017-2018. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_S4U_IO_HPP
#define SIMGRID_S4U_IO_HPP

#include <simgrid/forward.h>
#include <simgrid/s4u/Activity.hpp>

#include <atomic>
#include <string>

namespace simgrid {
namespace s4u {

/** I/O Activity, representing the asynchronous disk access.
 *
 * They are generated from simgrid::s4u::Storage::read() and simgrid::s4u::Storage::write().
 */

class XBT_PUBLIC Io : public Activity {
public:
  enum class OpType { READ, WRITE };

private:
  explicit Io(sg_size_t size, OpType type) : Activity(), size_(size), type_(type) {}
public:
  friend XBT_PUBLIC void intrusive_ptr_release(simgrid::s4u::Io* i);
  friend XBT_PUBLIC void intrusive_ptr_add_ref(simgrid::s4u::Io* i);
  friend simgrid::s4u::Storage; // Factory of IOs

  ~Io() = default;

  Io* start() override;
  Io* wait() override;
  Io* wait_for(double timeout) override;
  Io* cancel() override;
  bool test() override;

  double get_remaining() override;
  sg_size_t get_performed_ioops();

#ifndef DOXYGEN
  XBT_ATTRIB_DEPRECATED_v324("Please use Io::wait_for()") void wait(double t) override { wait_for(t); }
#endif

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
