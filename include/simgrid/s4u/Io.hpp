/* Copyright (c) 2017-2025. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_S4U_IO_HPP
#define SIMGRID_S4U_IO_HPP

#include <simgrid/forward.h>
#include <simgrid/s4u/Activity.hpp>

#include <string>

namespace simgrid::s4u {

/** I/O Activity, representing an asynchronous disk access.
 *
 * @beginrst
 * They are generated from :cpp:func:`simgrid::s4u::Disk::io_init`, :cpp:func:`simgrid::s4u::Disk::read`,
 * :cpp:func:`simgrid::s4u::Disk::read_async`, :cpp:func:`simgrid::s4u::Disk::write` and
 * :cpp:func:`simgrid::s4u::Disk::write_async`. You can also create direct host-to-host I/O streams (bypassing the
 * mailbox and actor mechanisms) with :cpp:func:`streamto`, :cpp:func:`streamto_init` and
 * :cpp:func:`streamto_async`.
 * @endrst
 */

class XBT_PUBLIC Io : public Activity_T<Io> {
#ifndef DOXYGEN
  friend kernel::activity::IoImpl;
  friend kernel::EngineImpl;
#endif

protected:
  explicit Io(kernel::activity::IoImplPtr pimpl);
  Io* do_start() override;

  static ssize_t deprecated_wait_any_for(const std::vector<IoPtr>& ios, double timeout); // XBT_ATTRIB_DEPRECATED_v401

public:
  /** The kind of I/O operation: reading from, or writing to, a disk. */
  enum class OpType { READ, WRITE };

   /*! \static Initiate the creation of an I/O. Setters have to be called afterwards */
  static IoPtr init();

  /** Get the remaining amount of bytes to transfer for this I/O. When it's 0, it's done. */
  double get_remaining() const override;
  /** Retrieve the amount of I/O operations (e.g. individual disk blocks) already performed by this activity. */
  sg_size_t get_performed_ioops() const;
  /** Specify the disk on which this I/O operation must take place. Not to be used for direct host-to-host
   *  streams: use set_source()/set_destination() instead. */
  IoPtr set_disk(const s4u::Disk* disk);
  /** @brief Change the I/O priority, don't you think?
   *
   * An I/O with twice the priority will get twice the amount of bytes transferred when the resource is shared.
   * The default priority is 1. Currently, this cannot be changed once the I/O started. See also
   * update_priority(). */
  IoPtr set_priority(double priority);
  /** Specify the amount of bytes to read or write during this I/O. */
  IoPtr set_size(sg_size_t size);
  /** Specify whether this I/O is a read or a write operation. */
  IoPtr set_op_type(OpType type);

  /** Do a blocking I/O stream between two arbitrary hosts and their disks, bypassing the mailbox and actor
   *  mechanisms. This is equivalent to `streamto_async(...)->wait()`. */
  static IoPtr streamto_init(s4u::Host* from, const Disk* from_disk, s4u::Host* to, const Disk* to_disk);
  /** Creates and starts a direct, asynchronous I/O stream between the disk @a from_disk of host @a from and the
   *  disk @a to_disk of host @a to, bypassing the mailbox and actor mechanisms. */
  static IoPtr streamto_async(s4u::Host* from, const Disk* from_disk, s4u::Host* to, const Disk* to_disk,
                              uint64_t simulated_size_in_bytes);
  /** Do a blocking I/O stream between two arbitrary hosts and their disks, bypassing the mailbox and actor
   *  mechanisms. */
  static void streamto(s4u::Host* from, const Disk* from_disk, s4u::Host* to, const Disk* to_disk,
                       uint64_t simulated_size_in_bytes);

  /** Specify the source host and disk of a direct host-to-host I/O stream (see streamto()). Must be set together
   *  with set_destination(), before the I/O starts. */
  IoPtr set_source(s4u::Host* from, const Disk* from_disk);
  /** Specify the destination host and disk of a direct host-to-host I/O stream (see streamto()). Must be set
   *  together with set_source(), before the I/O starts. */
  IoPtr set_destination(s4u::Host* to, const Disk* to_disk);

  /** Change the I/O priority while it is already running. See also set_priority(). */
  IoPtr update_priority(double priority);

  /** Returns whether this I/O is assigned to the disk(s) that it needs to start (or to a source and destination
   *  host, for direct I/O streams). An unassigned I/O cannot start. */
  bool is_assigned() const override;
};

} // namespace simgrid::s4u

#endif /* SIMGRID_S4U_IO_HPP */
