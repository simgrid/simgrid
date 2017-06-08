/* Copyright (c) 2006-2017. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "../surf/StorageImpl.hpp"
#include "simgrid/s4u/Storage.hpp"
#include "simgrid/simix.hpp"
#include "xbt/lib.h"
#include <unordered_map>

namespace simgrid {
namespace s4u {
std::unordered_map<std::string, Storage*>* allStorages()
{
  std::unordered_map<std::string, surf::StorageImpl*>* map = surf::StorageImpl::storagesMap();
  std::unordered_map<std::string, Storage*>* res           = new std::unordered_map<std::string, Storage*>;
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

const char* Storage::name()
{
  return pimpl_->cname();
}

const char* Storage::host()
{
  return pimpl_->attach_;
}

sg_size_t Storage::sizeFree()
{
  return simgrid::simix::kernelImmediate([this] { return pimpl_->getFreeSize(); });
}

sg_size_t Storage::sizeUsed()
{
  return simgrid::simix::kernelImmediate([this] { return pimpl_->getUsedSize(); });
}

sg_size_t Storage::size() {
  return pimpl_->size_;
}

xbt_dict_t Storage::properties()
{
  return simgrid::simix::kernelImmediate([this] { return pimpl_->getProperties(); });
}

const char* Storage::property(const char* key)
{
  return static_cast<const char*>(xbt_dict_get_or_null(this->properties(), key));
}

void Storage::setProperty(const char* key, char* value)
{
  xbt_dict_set(this->properties(), key, value, nullptr);
}

std::map<std::string, sg_size_t*>* Storage::content()
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
