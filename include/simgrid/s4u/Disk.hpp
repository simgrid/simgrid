/* Copyright (c) 2019-2020. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef INCLUDE_SIMGRID_S4U_DISK_HPP_
#define INCLUDE_SIMGRID_S4U_DISK_HPP_

#include <simgrid/forward.h>
#include <simgrid/s4u/Io.hpp>
#include <xbt/Extendable.hpp>
#include <xbt/base.h>
#include <xbt/signal.hpp>

#include <map>
#include <string>
#include <unordered_map>

namespace simgrid {
namespace s4u {

/** Disk represent the disk resources associated to a host
 *
 * By default, SimGrid does not keep track of the actual data being written but
 * only computes the time taken by the corresponding data movement.
 */

class XBT_PUBLIC Disk : public xbt::Extendable<Disk> {
  kernel::resource::DiskImpl* const pimpl_;
  std::string name_;
  friend Engine;
  friend Io;
  friend kernel::resource::DiskImpl;

protected:
  virtual ~Disk() = default;

public:
  explicit Disk(const std::string& name, kernel::resource::DiskImpl* pimpl) : pimpl_(pimpl), name_(name) {}

  /** @brief Callback signal fired when a new Disk is created */
  static xbt::signal<void(Disk&)> on_creation;
  /** @brief Callback signal fired when a Disk is destroyed */
  static xbt::signal<void(Disk const&)> on_destruction;
  /** @brief Callback signal fired when a Disk's state changes */
  static xbt::signal<void(Disk const&)> on_state_change;

  /** @brief Retrieves the name of that disk as a C++ string */
  std::string const& get_name() const { return name_; }
  /** @brief Retrieves the name of that disk as a C string */
  const char* get_cname() const { return name_.c_str(); }
  double get_read_bandwidth() const;
  double get_write_bandwidth() const;
  const std::unordered_map<std::string, std::string>* get_properties() const;
  const char* get_property(const std::string& key) const;
  void set_property(const std::string&, const std::string& value);
  Host* get_host() const;

  IoPtr io_init(sg_size_t size, s4u::Io::OpType type);

  IoPtr read_async(sg_size_t size);
  sg_size_t read(sg_size_t size);

  IoPtr write_async(sg_size_t size);
  sg_size_t write(sg_size_t size);
  kernel::resource::DiskImpl* get_impl() const { return pimpl_; }
};

} // namespace s4u
} // namespace simgrid

#endif /* INCLUDE_SIMGRID_S4U_DISK_HPP_ */
