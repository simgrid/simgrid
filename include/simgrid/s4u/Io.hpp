/* Copyright (c) 2017-2022. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_S4U_IO_HPP
#define SIMGRID_S4U_IO_HPP

#include <simgrid/forward.h>
#include <simgrid/s4u/Activity.hpp>

#include <string>

namespace simgrid {
namespace s4u {

/** I/O Activity, representing the asynchronous disk access.
 *
 * They are generated from Disk::io_init(), Disk::read() Disk::read_async(), Disk::write() and Disk::write_async().
 */

class XBT_PUBLIC Io : public Activity_T<Io> {
#ifndef DOXYGEN
  friend kernel::activity::IoImpl;
  friend kernel::EngineImpl;
#endif

protected:
  explicit Io(kernel::activity::IoImplPtr pimpl);

public:
  enum class OpType { READ, WRITE };

  static xbt::signal<void(Io const&)> on_start;

  static IoPtr init();
  Io* start() override;
  /*! take a vector of s4u::IoPtr and return when one of them is finished.
   * The return value is the rank of the first finished IoPtr. */
  static ssize_t wait_any(const std::vector<IoPtr>& ios) { return wait_any_for(ios, -1); }
  /*! Same as wait_any, but with a timeout. If the timeout occurs, parameter last is returned.*/
  static ssize_t wait_any_for(const std::vector<IoPtr>& ios, double timeout);

  double get_remaining() const override;
  sg_size_t get_performed_ioops() const;
  IoPtr set_disk(const_sg_disk_t disk);
  IoPtr set_priority(double priority);
  IoPtr set_size(sg_size_t size);
  IoPtr set_op_type(OpType type);

  IoPtr update_priority(double priority);

  bool is_assigned() const override;
};

} // namespace s4u
} // namespace simgrid

#endif /* SIMGRID_S4U_IO_HPP */
