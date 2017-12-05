/* Copyright (c) 2006-2017. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/s4u/Host.hpp"
#include "simgrid/s4u/Storage.hpp"
#include "simgrid/simix.hpp"
#include "src/plugins/file_system/FileSystem.hpp"
#include "src/surf/StorageImpl.hpp"
#include <unordered_map>

namespace simgrid {
namespace xbt {
template class Extendable<simgrid::s4u::Storage>;
}

namespace s4u {

void getStorageList(std::map<std::string, Storage*>* whereTo)
{
  for (auto const& s : *surf::StorageImpl::storagesMap())
    whereTo->insert({s.first, &(s.second->piface_)}); // Convert each entry into its interface
}

Storage* Storage::byName(std::string name)
{
  surf::StorageImpl* res = surf::StorageImpl::byName(name);
  if (res == nullptr)
    return nullptr;
  return &res->piface_;
}

const std::string& Storage::getName() const
{
  return pimpl_->getName();
}

const char* Storage::getCname() const
{
  return pimpl_->getCname();
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
