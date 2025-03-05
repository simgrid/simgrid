/* Copyright (c) 2023-2025. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <simgrid/plugins/jbod.hpp>
#include <simgrid/s4u/Comm.hpp>
#include <simgrid/s4u/Disk.hpp>
#include <simgrid/s4u/Exec.hpp>
#include <simgrid/s4u/NetZone.hpp>


XBT_LOG_NEW_DEFAULT_SUBCATEGORY(s4u_jbod, s4u, "Logging specific to the JBOD implmentation");

namespace simgrid::plugin {

JbodPtr Jbod::create_jbod(s4u::NetZone* zone, const std::string& name, double speed, unsigned int num_disks,
                          RAID raid_level, double read_bandwidth, double write_bandwidth)
{
  xbt_assert(not ((raid_level == RAID::RAID4 || raid_level == RAID::RAID5) && num_disks < 3),
             "RAID%d requires at least 3 disks", (int) raid_level);
  xbt_assert(not (raid_level == RAID::RAID6 && num_disks < 4), "RAID6 requires at least 4 disks");

  auto* jbod = new Jbod();
  jbod->set_controller(zone->add_host(name, speed));
  jbod->set_num_disks(num_disks);
  jbod->set_parity_disk_idx(num_disks -1 );
  jbod->set_read_disk_idx(-1);
  jbod->set_raid_level(raid_level);
  for (unsigned int i = 0; i < num_disks; i++)
    jbod->get_controller()->add_disk(name + "_disk_" + std::to_string(i), read_bandwidth, write_bandwidth);

  auto res = JbodPtr(jbod, false);
  all_jbods_.insert({name,res});
  return res;
}

JbodPtr Jbod::by_name(const std::string& name)
{
  auto jbod = Jbod::by_name_or_null(name);
  if (not jbod)
    throw std::invalid_argument("Host not found: '" + name + "'");
  return jbod;
}

JbodPtr Jbod::by_name_or_null(const std::string& name)
{
  if (auto jbod_it = all_jbods_.find(name); jbod_it != all_jbods_.end())
    return jbod_it->second;
  return nullptr;
}

JbodIoPtr Jbod::read_init(sg_size_t size)
{
  auto comm = s4u::Comm::sendto_init()->set_source(this->controller_)->set_payload_size(size);
  std::vector<s4u::IoPtr> pending_ios;
  sg_size_t read_size = 0;
  std::vector<s4u::Disk*> targets;
  switch(raid_level_) {
    case RAID::RAID0:
      read_size = size / num_disks_;
      targets = controller_->get_disks();
      break;
    case RAID::RAID1:
      read_size = size;
      targets.push_back(controller_->get_disks().at(get_next_read_disk_idx()));
      break;
    case RAID::RAID4:
      read_size = size / (num_disks_ - 1);
      targets = controller_->get_disks();
      targets.pop_back();
      break;
    case RAID::RAID5:
      read_size = size / (num_disks_ - 1);
      targets = controller_->get_disks();
      targets.erase(targets.begin() + ((get_parity_disk_idx() + 1) % num_disks_));
      break;
    case RAID::RAID6:
      read_size = size / (num_disks_ - 2);
      targets = controller_->get_disks();
      if ( ((get_parity_disk_idx() + 2) % num_disks_) == 0 ) {
        targets.pop_back();
        targets.erase(targets.begin());
      } else if (get_parity_disk_idx() + 1 == static_cast<int>(num_disks_)) {
        targets.pop_back();
        targets.pop_back();
      } else {
        targets.erase(targets.begin() + (get_parity_disk_idx() + 1) % num_disks_,
                      targets.begin() + get_parity_disk_idx() + 3);
      }
      break;
    default:
      xbt_die("Unsupported RAID level. Supported level are: 0, 1, 4, 5, and 6");
  }
  for (const auto* disk : targets) {
    auto io = s4u::IoPtr(disk->io_init(read_size, s4u::Io::OpType::READ));
    io->set_name(disk->get_name());
    pending_ios.push_back(io);
  }

  return JbodIoPtr(new JbodIo(this, comm, nullptr, pending_ios, s4u::Io::OpType::READ));
}

JbodIoPtr Jbod::read_async(sg_size_t size)
{
  JbodIoPtr io = read_init(size);
  io->start();
  return io;
}

sg_size_t Jbod::read(sg_size_t size)
{
  read_async(size)->wait();
  return size;
}

JbodIoPtr Jbod::write_init(sg_size_t size)
{
  auto comm = s4u::Comm::sendto_init(s4u::Host::current(), this->get_controller());
  std::vector<s4u::IoPtr> pending_ios;
  sg_size_t write_size = 0;
  switch(raid_level_) {
    case RAID::RAID0:
      write_size = size / num_disks_;
      break;
    case RAID::RAID1:
      write_size = size;
      break;
    case RAID::RAID4:
      write_size = size / (num_disks_ - 1);
      break;
    case RAID::RAID5:
      update_parity_disk_idx();
      write_size = size / (num_disks_ - 1);
      break;
    case RAID::RAID6:
      update_parity_disk_idx();
      update_parity_disk_idx();
      write_size = size / (num_disks_ - 2);
      break;
    default:
      xbt_die("Unsupported RAID level. Supported level are: 0, 1, 4, 5, and 6");
  }
  for (const auto* disk : get_controller()->get_disks()) {
    auto io = s4u::IoPtr(disk->io_init(write_size, s4u::Io::OpType::WRITE));
    io->set_name(disk->get_name());
    pending_ios.push_back(io);
  }

  s4u::ExecPtr parity_block_comp = nullptr;
  if (raid_level_ == RAID::RAID4 || raid_level_ == RAID::RAID5 || raid_level_ == RAID::RAID6) {
    // Assume 1 flop per byte to write per parity block and two for RAID6.
    // Do not assign the Exec yet, will be done after the completion of the CommPtr
    if (raid_level_ == RAID::RAID6)
      parity_block_comp = s4u::Exec::init()->set_flops_amount(2 * write_size);
    else
      parity_block_comp = s4u::Exec::init()->set_flops_amount(write_size);
  }
  comm->set_payload_size(size);
  return JbodIoPtr(new JbodIo(this, comm, parity_block_comp, pending_ios, s4u::Io::OpType::WRITE));
}


JbodIoPtr Jbod::write_async(sg_size_t size)
{
  JbodIoPtr io = write_init(size);
  io->start();
  return io;
}

sg_size_t Jbod::write(sg_size_t size)
{
  write_async(size)->wait();
  return size;
}

JbodIo* JbodIo::start()
{
  if (type_ == s4u::Io::OpType::READ) {
    for (auto& io : pending_ios_)
      io->start();
  } else if (type_ == s4u::Io::OpType::WRITE) {
    transfer_->start();
  } else
    xbt_die("Unknown JBOD I/O type");
  return this;
}

void JbodIo::wait()
{
  if ((type_ == s4u::Io::OpType::WRITE && transfer_->get_state() == s4u::Activity::State::INITED) ||
      (type_ == s4u::Io::OpType::READ && pending_ios_.front()->get_state() == s4u::Activity::State::INITED))
    start();

  if (type_ == s4u::Io::OpType::WRITE) {
    transfer_->wait();
    XBT_DEBUG("Data received on JBOD");
    if (parity_block_comp_) {
      parity_block_comp_->set_host(jbod_->get_controller())->wait();
      XBT_DEBUG("Parity block computed");
    }
    XBT_DEBUG("Start writing");
    for (const auto& io : pending_ios_)
      io->start();
  }

  for (const auto& io : pending_ios_) {
    XBT_DEBUG("Wait for I/O on %s", io->get_cname());
    io->wait();
  }

  if (type_ == s4u::Io::OpType::READ) {
    XBT_DEBUG("Data read on JBOD, send it to %s", s4u::Host::current()->get_cname());
    transfer_->set_destination(s4u::Host::current())->wait();
  }
}
} // namespace simgrid::plugin
