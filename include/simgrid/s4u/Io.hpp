/* Copyright (c) 2017-2020. The SimGrid Team. All rights reserved.          */

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
public:
  enum class OpType { READ, WRITE };

private:
  Storage* storage_ = nullptr;
  Disk* disk_       = nullptr;
  sg_size_t size_   = 0;
  OpType type_      = OpType::READ;

  explicit Io(sg_storage_t storage, sg_size_t size, OpType type);
  explicit Io(sg_disk_t disk, sg_size_t size, OpType type);

public:
#ifndef DOXYGEN
  friend Disk;    // Factory of IOs
  friend Storage; // Factory of IOs

  ~Io() override = default;
#endif

  static xbt::signal<void(Io const&, Actor const&)> on_start;
  static xbt::signal<void(Io const&, Actor const&)> on_completion;

  Io* start() override;
  Io* wait() override;
  Io* wait_for(double timeout) override;
  Io* cancel() override;

  double get_remaining() const override;
  sg_size_t get_performed_ioops() const;
};

} // namespace s4u
} // namespace simgrid

#endif /* SIMGRID_S4U_IO_HPP */
