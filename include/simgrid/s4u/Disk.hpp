/* Copyright (c) 2019-2022. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef INCLUDE_SIMGRID_S4U_DISK_HPP_
#define INCLUDE_SIMGRID_S4U_DISK_HPP_

#include <simgrid/disk.h>
#include <simgrid/forward.h>
#include <simgrid/s4u/Io.hpp>
#include <xbt/Extendable.hpp>
#include <xbt/base.h>
#include <xbt/signal.hpp>

#include <map>
#include <string>
#include <unordered_map>

namespace simgrid {

extern template class XBT_PUBLIC xbt::Extendable<s4u::Disk>;

namespace s4u {

/** Disk represent the disk resources associated to a host
 *
 * By default, SimGrid does not keep track of the actual data being written but
 * only computes the time taken by the corresponding data movement.
 */

class XBT_PUBLIC Disk : public xbt::Extendable<Disk> {
#ifndef DOXYGEN
  friend Engine;
  friend Io;
  friend kernel::resource::DiskImpl;
#endif

  explicit Disk(kernel::resource::DiskImpl* pimpl) : pimpl_(pimpl) {}
  virtual ~Disk() = default;

  // The private implementation, that never changes
  kernel::resource::DiskImpl* const pimpl_;

public:
#ifndef DOXYGEN
  kernel::resource::DiskImpl* get_impl() const { return pimpl_; }
#endif

  std::string const& get_name() const;
  /** @brief Retrieves the name of that disk as a C string */
  const char* get_cname() const;

  Disk* set_read_bandwidth(double read_bw);
  double get_read_bandwidth() const;

  Disk* set_write_bandwidth(double write_bw);
  double get_write_bandwidth() const;

  /**
   * @brief Set limit for read/write operations.
   *
   * This determines the limit for read and write operation in the same disk.
   * Usually, it's configured to max(read_bw, write_bw).
   * You can change this behavior using this method
   *
   * @param bw New bandwidth for the disk
   */
  Disk* set_readwrite_bandwidth(double bw);

  const std::unordered_map<std::string, std::string>* get_properties() const;
  const char* get_property(const std::string& key) const;
  Disk* set_property(const std::string&, const std::string& value);
  Disk* set_properties(const std::unordered_map<std::string, std::string>& properties);

  Disk* set_host(Host* host);
  Host* get_host() const;

  Disk* set_state_profile(kernel::profile::Profile* profile);
  Disk* set_read_bandwidth_profile(kernel::profile::Profile* profile);
  Disk* set_write_bandwidth_profile(kernel::profile::Profile* profile);

  IoPtr io_init(sg_size_t size, s4u::Io::OpType type) const;

  IoPtr read_async(sg_size_t size) const;
  sg_size_t read(sg_size_t size) const;
  sg_size_t read(sg_size_t size, double priority) const;

  IoPtr write_async(sg_size_t size) const;
  sg_size_t write(sg_size_t size) const;
  sg_size_t write(sg_size_t size, double priority) const;

  /** @brief Policy for sharing the disk among activities */
  enum class SharingPolicy { NONLINEAR = 1, LINEAR = 0 };
  enum class Operation { READ = 2, WRITE = 1, READWRITE = 0 };

  /**
   * @brief Describes how the disk is shared between activities for each operation
   *
   * Disks have different bandwidths for read and write operations, that can have different policies:
   * - Read: resource sharing for read operation
   * - Write: resource sharing for write
   * - ReadWrite: global sharing for read and write operations
   *
   * Note that the NONLINEAR callback is in the critical path of the solver, so it should be fast.
   *
   * @param op Operation type
   * @param policy Sharing policy
   * @param cb Callback for NONLINEAR policies
   */
  Disk* set_sharing_policy(Operation op, SharingPolicy policy, const s4u::NonLinearResourceCb& cb = {});
  SharingPolicy get_sharing_policy(Operation op) const;
  /**
   * @brief Callback to set IO factors
   *
   * This callback offers a flexible way to create variability in I/O operations
   *
   * @param size I/O operation size in bytes
   * @param op I/O operation type: read or write
   * @return Multiply factor
   */
  using IoFactorCb = double(sg_size_t size, Io::OpType op);
  /** @brief Configure the factor callback */
  Disk* set_factor_cb(const std::function<IoFactorCb>& cb);

  Disk* seal();

  /* The signals */
  /** @brief Add a callback fired when a new Disk is created */
  static void on_creation_cb(const std::function<void(Disk&)>& cb) { on_creation.connect(cb); }
  /** @brief Add a callback fired when a Disk is destroyed */
  static void on_destruction_cb(const std::function<void(Disk const&)>& cb) { on_destruction.connect(cb); }
  /** @brief Add a callback fired when a Disk's state changes */
  static void on_state_change_cb(const std::function<void(Disk const&)>& cb) { on_state_change.connect(cb); }

private:
  static xbt::signal<void(Disk&)> on_creation;
  static xbt::signal<void(Disk const&)> on_destruction;
  static xbt::signal<void(Disk const&)> on_state_change;
};

} // namespace s4u
} // namespace simgrid

#endif /* INCLUDE_SIMGRID_S4U_DISK_HPP_ */
