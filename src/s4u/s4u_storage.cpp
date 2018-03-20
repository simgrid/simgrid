/* Copyright (c) 2006-2018. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/kernel/resource/Resource.hpp"
#include "simgrid/plugins/file_system.h"
#include "simgrid/s4u/Engine.hpp"
#include "simgrid/s4u/Host.hpp"
#include "simgrid/s4u/Storage.hpp"
#include "simgrid/simix.hpp"
#include "simgrid/storage.h"
#include "src/surf/StorageImpl.hpp"

#include <string>
#include <unordered_map>

namespace simgrid {
namespace xbt {
template class Extendable<simgrid::s4u::Storage>;
}

namespace s4u {

void XBT_ATTRIB_DEPRECATED_v322(
    "simgrid::s4u::getStorageList() is deprecated in favor of Engine::getAllStorages(). Please switch before v3.22")
    getStorageList(std::map<std::string, Storage*>* whereTo)
{
  for (auto const& s : simgrid::s4u::Engine::getInstance()->getAllStorages())
    whereTo->insert({s->getName(), s});
}

Storage::Storage(std::string name, surf::StorageImpl* pimpl) : pimpl_(pimpl), name_(name)
{
  simgrid::s4u::Engine::getInstance()->addStorage(name, this);
}

Storage* Storage::byName(std::string name)
{
  return Engine::getInstance()->storageByNameOrNull(name);
}

const std::string& Storage::getName() const
{
  return name_;
}

const char* Storage::getCname() const
{
  return name_.c_str();
}

const char* Storage::getType()
{
  return pimpl_->typeId_.c_str();
}

Host* Storage::getHost()
{
  return attached_to_;
}

std::map<std::string, std::string>* Storage::getProperties()
{
  return simgrid::simix::kernelImmediate([this] { return pimpl_->getProperties(); });
}

const char* Storage::getProperty(std::string key)
{
  return this->pimpl_->getProperty(key);
}

void Storage::setProperty(std::string key, std::string value)
{
  simgrid::simix::kernelImmediate([this, key, value] { this->pimpl_->setProperty(key, value); });
}

sg_size_t Storage::read(sg_size_t size)
{
  return simcall_storage_read(pimpl_, size);
}

sg_size_t Storage::write(sg_size_t size)
{
  return simcall_storage_write(pimpl_, size);
}

/*************
 * Callbacks *
 *************/
simgrid::xbt::signal<void(s4u::Storage&)> Storage::onCreation;
simgrid::xbt::signal<void(s4u::Storage&)> Storage::onDestruction;

} /* namespace s4u */
} /* namespace simgrid */

/* **************************** Public C interface *************************** */
SG_BEGIN_DECL()
/** @addtogroup sg_storage_management
 * (#sg_storage_t) and the functions for managing it.
 */

/** \ingroup sg_storage_management
 *
 * \brief Returns the name of the #sg_storage_t.
 *
 * This functions checks whether a storage is a valid pointer or not and return its name.
 */
const char* sg_storage_get_name(sg_storage_t storage)
{
  xbt_assert((storage != nullptr), "Invalid parameters");
  return storage->getCname();
}

const char* sg_storage_get_host(sg_storage_t storage)
{
  xbt_assert((storage != nullptr), "Invalid parameters");
  return storage->getHost()->getCname();
}

/** \ingroup sg_storage_management
 * \brief Returns a xbt_dict_t consisting of the list of properties assigned to this storage
 * \param storage a storage
 * \return a dict containing the properties
 */
xbt_dict_t sg_storage_get_properties(sg_storage_t storage)
{
  xbt_assert((storage != nullptr), "Invalid parameters (storage is nullptr)");
  xbt_dict_t as_dict = xbt_dict_new_homogeneous(xbt_free_f);
  std::map<std::string, std::string>* props = storage->getProperties();
  if (props == nullptr)
    return nullptr;
  for (auto const& elm : *props) {
    xbt_dict_set(as_dict, elm.first.c_str(), xbt_strdup(elm.second.c_str()), nullptr);
  }
  return as_dict;
}

/** \ingroup sg_storage_management
 * \brief Change the value of a given storage property
 *
 * \param storage a storage
 * \param name a property name
 * \param value what to change the property to
 */
void sg_storage_set_property_value(sg_storage_t storage, const char* name, char* value)
{
  storage->setProperty(name, value);
}

/** \ingroup sg_storage_management
 * \brief Returns the value of a given storage property
 *
 * \param storage a storage
 * \param name a property name
 * \return value of a property (or nullptr if property not set)
 */
const char* sg_storage_get_property_value(sg_storage_t storage, const char* name)
{
  return storage->getProperty(name);
}

/** \ingroup sg_storage_management
 * \brief Finds a sg_storage_t using its name.
 * \param name the name of a storage
 * \return the corresponding storage
 */
sg_storage_t sg_storage_get_by_name(const char* name)
{
  return simgrid::s4u::Storage::byName(name);
}

/** \ingroup sg_storage_management
 * \brief Returns a dynar containing all the storage elements declared at a given point of time
 */
xbt_dynar_t sg_storages_as_dynar()
{
  std::vector<simgrid::s4u::Storage*> storage_list = simgrid::s4u::Engine::getInstance()->getAllStorages();
  xbt_dynar_t res = xbt_dynar_new(sizeof(sg_storage_t), nullptr);
  for (auto const& s : storage_list)
    xbt_dynar_push(res, &s);
  return res;
}

void* sg_storage_get_data(sg_storage_t storage)
{
  xbt_assert((storage != nullptr), "Invalid parameters");
  return storage->getUserdata();
}

void sg_storage_set_data(sg_storage_t storage, void* data)
{
  storage->setUserdata(data);
}

sg_size_t sg_storage_read(sg_storage_t storage, sg_size_t size)
{
  return storage->read(size);
}

sg_size_t sg_storage_write(sg_storage_t storage, sg_size_t size)
{
  return storage->write(size);
}
SG_END_DECL()
