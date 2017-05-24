/* Copyright (c) 2006-2017. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "../surf/StorageImpl.hpp"
#include "simgrid/s4u/Storage.hpp"
#include "simgrid/simix.hpp"
#include "xbt/lib.h"
#include <unordered_map>

extern xbt_lib_t storage_lib;

namespace simgrid {
namespace s4u {

std::unordered_map<std::string, Storage*>* Storage::storages_ = new std::unordered_map<std::string, Storage*>();

Storage::Storage(std::string name, smx_storage_t inferior) :
    name_(name), pimpl_(inferior)
{
  hostname_ = surf_storage_get_host(pimpl_);
  size_     = surf_storage_get_size(pimpl_);
  storages_->insert({name, this});
}

Storage::~Storage() = default;

smx_storage_t Storage::inferior()
{
  return pimpl_;
}

Storage& Storage::byName(const char* name)
{
  s4u::Storage* res = nullptr;
  try {
    res = storages_->at(name);
  } catch (std::out_of_range& e) {
    smx_storage_t inferior = xbt_lib_get_elm_or_null(storage_lib,name);
    if (inferior == nullptr)
      xbt_die("Storage %s does not exist. Please only use the storages that are defined in your platform.", name);

    res = new Storage(name,inferior);
  }
  return *res;
}

const char* Storage::name()
{
  return name_.c_str();
}

const char* Storage::host()
{
  return hostname_.c_str();
}

sg_size_t Storage::sizeFree()
{
  return simgrid::simix::kernelImmediate([this] { return surf_storage_resource_priv(pimpl_)->getFreeSize(); });
}

sg_size_t Storage::sizeUsed()
{
  return simgrid::simix::kernelImmediate([this] { return surf_storage_resource_priv(pimpl_)->getUsedSize(); });
}

sg_size_t Storage::size() {
  return size_;
}

xbt_dict_t Storage::properties()
{
  return simcall_storage_get_properties(pimpl_);
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
  return simgrid::simix::kernelImmediate([this] { return surf_storage_resource_priv(this->pimpl_)->getContent(); });
}

std::unordered_map<std::string, Storage*>* Storage::allStorages()
{
  return storages_;
}

} /* namespace s4u */
} /* namespace simgrid */
