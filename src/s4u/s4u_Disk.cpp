/* Copyright (c) 2019-2020. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/s4u/Disk.hpp"
#include "simgrid/s4u/Engine.hpp"
#include "simgrid/s4u/Host.hpp"
#include "simgrid/s4u/Io.hpp"
#include "src/kernel/resource/DiskImpl.hpp"

namespace simgrid {

template class xbt::Extendable<s4u::Disk>;

namespace s4u {

xbt::signal<void(Disk&)> Disk::on_creation;
xbt::signal<void(Disk const&)> Disk::on_destruction;
xbt::signal<void(Disk const&)> Disk::on_state_change;

double Disk::get_read_bandwidth() const
{
  return this->pimpl_->get_read_bandwidth();
}

double Disk::get_write_bandwidth() const
{
  return pimpl_->get_write_bandwidth();
}

Host* Disk::get_host() const
{
  return pimpl_->get_host();
}

const std::unordered_map<std::string, std::string>* Disk::get_properties() const
{
  return pimpl_->get_properties();
}

const char* Disk::get_property(const std::string& key) const
{
  return this->pimpl_->get_property(key);
}

void Disk::set_property(const std::string& key, const std::string& value)
{
  kernel::actor::simcall([this, &key, &value] { this->pimpl_->set_property(key, value); });
}

IoPtr Disk::io_init(sg_size_t size, Io::OpType type)
{
  return IoPtr(new Io(this, size, type));
}

IoPtr Disk::read_async(sg_size_t size)
{
  return IoPtr(io_init(size, Io::OpType::READ))->start();
}

sg_size_t Disk::read(sg_size_t size)
{
  return IoPtr(io_init(size, Io::OpType::READ))->start()->wait()->get_performed_ioops();
}

IoPtr Disk::write_async(sg_size_t size)
{
  return IoPtr(io_init(size, Io::OpType::WRITE)->start());
}

sg_size_t Disk::write(sg_size_t size)
{
  return IoPtr(io_init(size, Io::OpType::WRITE))->start()->wait()->get_performed_ioops();
}

} // namespace s4u
} // namespace simgrid

/* **************************** Public C interface *************************** */

const char* sg_disk_name(const_sg_disk_t disk)
{
  return disk->get_cname();
}

double sg_disk_read_bandwidth(const_sg_disk_t disk)
{
  return disk->get_read_bandwidth();
}

double sg_disk_write_bandwidth(const_sg_disk_t disk)
{
  return disk->get_write_bandwidth();
}

sg_size_t sg_disk_read(sg_disk_t disk, sg_size_t size)
{
  return disk->read(size);
}
sg_size_t sg_disk_write(sg_disk_t disk, sg_size_t size)
{
  return disk->write(size);
}
void* sg_disk_data(const_sg_disk_t disk)
{
  return disk->get_data();
}
void sg_disk_data_set(sg_disk_t disk, void* data)
{
  disk->set_data(data);
}
