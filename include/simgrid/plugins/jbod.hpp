/* Copyright (c) 2023. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_PLUGIN_JBOD_HPP
#define SIMGRID_PLUGIN_JBOD_HPP
#include <simgrid/s4u/Host.hpp>
#include <simgrid/s4u/Io.hpp>

namespace simgrid::plugin {

class Jbod;
using JbodPtr = boost::intrusive_ptr<Jbod>;
class JbodIo;
using JbodIoPtr = boost::intrusive_ptr<JbodIo>;

class Jbod {
public:
  enum class RAID {RAID0 = 0, RAID1 = 1, RAID4 = 4 , RAID5 = 5, RAID6 = 6};
  s4u::Host* get_controller() const { return controller_; }
  int get_parity_disk_idx() { return parity_disk_idx_; }
  void update_parity_disk_idx() { parity_disk_idx_ = (parity_disk_idx_- 1) % num_disks_; }

  int get_next_read_disk_idx() { return (++read_disk_idx_) % num_disks_; }

  JbodIoPtr read_async(sg_size_t size);
  sg_size_t read(sg_size_t size);

  JbodIoPtr write_async(sg_size_t size);
  sg_size_t write(sg_size_t size);

  static JbodPtr create_jbod(s4u::NetZone* zone, const std::string& name, double speed, unsigned int num_disks,
                             RAID raid_level, double read_bandwidth, double write_bandwidth);

protected:
  void set_controller(s4u::Host* host) { controller_ = host; }
  void set_num_disks(unsigned int num_disks) { num_disks_ = num_disks; }
  void set_parity_disk_idx(unsigned int index) { parity_disk_idx_ = index; }
  void set_read_disk_idx(int index) { read_disk_idx_ = index; }
  void set_raid_level(RAID raid_level) { raid_level_ = raid_level; }

private:
  s4u::Host* controller_;
  unsigned int num_disks_;
  RAID raid_level_;
  unsigned int parity_disk_idx_;
  int read_disk_idx_;
  std::atomic_int_fast32_t refcount_{1};
#ifndef DOXYGEN
  friend void intrusive_ptr_release(Jbod* jbod)
  {
    if (jbod->refcount_.fetch_sub(1, std::memory_order_release) == 1) {
      std::atomic_thread_fence(std::memory_order_acquire);
      delete jbod;
    }
  }
  friend void intrusive_ptr_add_ref(Jbod* jbod) { jbod->refcount_.fetch_add(1, std::memory_order_relaxed); }
#endif
};

class JbodIo {
  const Jbod* jbod_;
  s4u::CommPtr transfer_;
  s4u::ExecPtr parity_block_comp_;
  std::vector<s4u::IoPtr> pending_ios_;
  s4u::Io::OpType type_;
  std::atomic_int_fast32_t refcount_{0};
public:

  explicit JbodIo(const Jbod* jbod, const s4u::CommPtr transfer, const s4u::ExecPtr parity_block_comp,
                  const std::vector<s4u::IoPtr>& pending_ios, s4u::Io::OpType type)
    : jbod_(jbod), transfer_(transfer), parity_block_comp_(parity_block_comp), pending_ios_(pending_ios), type_(type)
    {}

  void wait();

#ifndef DOXYGEN
  friend void intrusive_ptr_release(JbodIo* io)
  {
    if (io->refcount_.fetch_sub(1, std::memory_order_release) == 1) {
      std::atomic_thread_fence(std::memory_order_acquire);
      delete io;
    }
  }
  friend void intrusive_ptr_add_ref(JbodIo* io) { io->refcount_.fetch_add(1, std::memory_order_relaxed); }
#endif
};

/* Refcounting functions */
XBT_PUBLIC void intrusive_ptr_release(const Jbod* io);
XBT_PUBLIC void intrusive_ptr_add_ref(const Jbod* io);
XBT_PUBLIC void intrusive_ptr_release(const JbodIo* io);
XBT_PUBLIC void intrusive_ptr_add_ref(const JbodIo* io);

} // namespace simgrid::plugin
#endif
