/* Copyright (c) 2017-2023. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_S4U_IO_HPP
#define SIMGRID_S4U_IO_HPP

#include <simgrid/forward.h>
#include <simgrid/s4u/Activity.hpp>

#include <string>

namespace simgrid::s4u {

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
  Io* do_start() override;

  static ssize_t deprecated_wait_any_for(const std::vector<IoPtr>& ios, double timeout); // XBT_ATTRIB_DEPRECATED_v339

public:
  enum class OpType { READ, WRITE };

   /*! \static Initiate the creation of an I/O. Setters have to be called afterwards */
  static IoPtr init();
#ifndef DOXYGEN
  XBT_ATTRIB_DEPRECATED_v339("Please use ActivitySet instead") 
  static ssize_t wait_any(const std::vector<IoPtr>& ios) { return deprecated_wait_any_for(ios, -1); }
  XBT_ATTRIB_DEPRECATED_v339("Please use ActivitySet instead") 
  static ssize_t wait_any_for(const std::vector<IoPtr>& ios, double timeout) { return deprecated_wait_any_for(ios, timeout); }
#endif

  double get_remaining() const override;
  sg_size_t get_performed_ioops() const;
  IoPtr set_disk(const_sg_disk_t disk);
  IoPtr set_priority(double priority);
  IoPtr set_size(sg_size_t size);
  IoPtr set_op_type(OpType type);

  static IoPtr streamto_init(Host* from, const Disk* from_disk, Host* to, const Disk* to_disk);
  static IoPtr streamto_async(Host* from, const Disk* from_disk, Host* to, const Disk* to_disk,
                              uint64_t simulated_size_in_bytes);
  static void streamto(Host* from, const Disk* from_disk, Host* to, const Disk* to_disk,
                       uint64_t simulated_size_in_bytes);

  IoPtr set_source(Host* from, const Disk* from_disk);
  IoPtr set_destination(Host* to, const Disk* to_disk);

  IoPtr update_priority(double priority);

  bool is_assigned() const override;
};

} // namespace simgrid::s4u

#endif /* SIMGRID_S4U_IO_HPP */
