/* Copyright (c) 2006-2017. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "../surf/StorageImpl.hpp"
#include "simgrid/s4u/Host.hpp"
#include "simgrid/s4u/Storage.hpp"
#include "simgrid/simix.hpp"
#include <unordered_map>

namespace simgrid {
namespace s4u {

std::map<std::string, Storage*>* allStorages()
{
  std::unordered_map<std::string, surf::StorageImpl*>* map = surf::StorageImpl::storagesMap();
  std::map<std::string, Storage*>* res                     = new std::map<std::string, Storage*>;
  for (auto s : *map)
    res->insert({s.first, &(s.second->piface_)}); // Convert each entry into its interface

  return res;
}

Storage* Storage::byName(const char* name)
{
  surf::StorageImpl* res = surf::StorageImpl::byName(name);
  if (res == nullptr)
    return nullptr;
  return &res->piface_;
}

const char* Storage::getName()
{
  return pimpl_->cname();
}

const char* Storage::getType()
{
  return pimpl_->typeId_.c_str();
}

Host* Storage::getHost()
{
  return attached_to_;
}

sg_size_t Storage::getSizeFree()
{
  return simgrid::simix::kernelImmediate([this] { return pimpl_->getFreeSize(); });
}

sg_size_t Storage::getSizeUsed()
{
  return simgrid::simix::kernelImmediate([this] { return pimpl_->getUsedSize(); });
}

sg_size_t Storage::getSize()
{
  return pimpl_->getSize();
}

xbt_dict_t Storage::getProperties()
{
  return simgrid::simix::kernelImmediate([this] { return pimpl_->getProperties(); });
}

const char* Storage::getProperty(const char* key)
{
  return this->pimpl_->getProperty(key);
}

void Storage::setProperty(const char* key, const char* value)
{
  simgrid::simix::kernelImmediate([this, key, value] { this->pimpl_->setProperty(key, value); });
}

std::map<std::string, sg_size_t>* Storage::getContent()
{
  return simgrid::simix::kernelImmediate([this] { return pimpl_->getContent(); });
}

/*************
 * Callbacks *
 *************/
simgrid::xbt::signal<void(s4u::Storage&)> Storage::onCreation;
simgrid::xbt::signal<void(s4u::Storage&)> Storage::onDestruction;

} /* namespace s4u */
} /* namespace simgrid */
