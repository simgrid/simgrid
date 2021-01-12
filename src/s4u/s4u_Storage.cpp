/* Copyright (c) 2006-2021. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/s4u/Engine.hpp"
#include "simgrid/s4u/Host.hpp"
#include "simgrid/s4u/Io.hpp"
#include "simgrid/s4u/Storage.hpp"
#include "simgrid/simix.hpp"
#include "simgrid/storage.h"
#include "src/surf/StorageImpl.hpp"

namespace simgrid {

template class xbt::Extendable<s4u::Storage>;

namespace s4u {

xbt::signal<void(Storage&)> Storage::on_creation;
xbt::signal<void(Storage const&)> Storage::on_destruction;
xbt::signal<void(Storage const&)> Storage::on_state_change;

Storage::Storage(const std::string& name, kernel::resource::StorageImpl* pimpl) : pimpl_(pimpl), name_(name)
{
  Engine::get_instance()->storage_register(name_, this);
}

Storage* Storage::by_name(const std::string& name)
{
  return Engine::get_instance()->storage_by_name(name);
}

Storage* Storage::by_name_or_null(const std::string& name)
{
  return Engine::get_instance()->storage_by_name_or_null(name);
}

const char* Storage::get_type() const
{
  return pimpl_->get_type();
}

const std::unordered_map<std::string, std::string>* Storage::get_properties() const
{
  return pimpl_->get_properties();
}

const char* Storage::get_property(const std::string& key) const
{
  return this->pimpl_->get_property(key);
}

void Storage::set_property(const std::string& key, const std::string& value)
{
  kernel::actor::simcall([this, &key, &value] { this->pimpl_->set_property(key, value); });
}

IoPtr Storage::io_init(sg_size_t size, Io::OpType type)
{
  return IoPtr(new Io(this, size, type));
}

IoPtr Storage::read_async(sg_size_t size)
{
  return IoPtr(io_init(size, Io::OpType::READ))->start();
}

sg_size_t Storage::read(sg_size_t size)
{
  return IoPtr(io_init(size, Io::OpType::READ))->start()->wait()->get_performed_ioops();
}

IoPtr Storage::write_async(sg_size_t size)
{
  return IoPtr(io_init(size, Io::OpType::WRITE)->start());
}

sg_size_t Storage::write(sg_size_t size)
{
  return IoPtr(io_init(size, Io::OpType::WRITE))->start()->wait()->get_performed_ioops();
}

} // namespace s4u
} // namespace simgrid

/* **************************** Public C interface *************************** */

/** @addtogroup sg_storage_management
 * (#sg_storage_t) and the functions for managing it.
 */

/** @ingroup sg_storage_management
 *
 * @brief Returns the name of the #sg_storage_t.
 *
 * This functions checks whether a storage is a valid pointer or not and return its name.
 */
const char* sg_storage_get_name(const_sg_storage_t storage)
{
  xbt_assert((storage != nullptr), "Invalid parameters");
  return storage->get_cname();
}

const char* sg_storage_get_host(const_sg_storage_t storage)
{
  xbt_assert((storage != nullptr), "Invalid parameters");
  return storage->get_host()->get_cname();
}

/** @ingroup sg_storage_management
 * @brief Returns a xbt_dict_t consisting of the list of properties assigned to this storage
 * @param storage a storage
 * @return a dict containing the properties
 */
xbt_dict_t sg_storage_get_properties(const_sg_storage_t storage)
{
  xbt_assert((storage != nullptr), "Invalid parameters (storage is nullptr)");
  xbt_dict_t as_dict                        = xbt_dict_new_homogeneous(xbt_free_f);
  const std::unordered_map<std::string, std::string>* props = storage->get_properties();
  if (props == nullptr)
    return nullptr;
  for (auto const& elm : *props) {
    xbt_dict_set(as_dict, elm.first.c_str(), xbt_strdup(elm.second.c_str()));
  }
  return as_dict;
}

/** @ingroup sg_storage_management
 * @brief Change the value of a given storage property
 *
 * @param storage a storage
 * @param name a property name
 * @param value what to change the property to
 */
void sg_storage_set_property_value(sg_storage_t storage, const char* name, const char* value)
{
  storage->set_property(name, value);
}

/** @ingroup sg_storage_management
 * @brief Returns the value of a given storage property
 *
 * @param storage a storage
 * @param name a property name
 * @return value of a property (or nullptr if property not set)
 */
const char* sg_storage_get_property_value(const_sg_storage_t storage, const char* name)
{
  return storage->get_property(name);
}

/** @ingroup sg_storage_management
 * @brief Finds a sg_storage_t using its name.
 * @param name the name of a storage
 * @return the corresponding storage
 */
sg_storage_t sg_storage_get_by_name(const char* name)
{
  return simgrid::s4u::Storage::by_name(name);
}

/** @ingroup sg_storage_management
 * @brief Returns a dynar containing all the storage elements declared at a given point of time
 */
xbt_dynar_t sg_storages_as_dynar()
{
  std::vector<simgrid::s4u::Storage*> storage_list = simgrid::s4u::Engine::get_instance()->get_all_storages();
  xbt_dynar_t res                                  = xbt_dynar_new(sizeof(sg_storage_t), nullptr);
  for (auto const& s : storage_list)
    xbt_dynar_push(res, &s);
  return res;
}

void* sg_storage_get_data(const_sg_storage_t storage)
{
  xbt_assert((storage != nullptr), "Invalid parameters");
  return storage->get_data();
}

void sg_storage_set_data(sg_storage_t storage, void* data)
{
  storage->set_data(data);
}

sg_size_t sg_storage_read(sg_storage_t storage, sg_size_t size)
{
  return storage->read(size);
}

sg_size_t sg_storage_write(sg_storage_t storage, sg_size_t size)
{
  return storage->write(size);
}
