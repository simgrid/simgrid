/* Copyright (c) 2006-2015, 2017. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef INCLUDE_SIMGRID_S4U_STORAGE_HPP_
#define INCLUDE_SIMGRID_S4U_STORAGE_HPP_

#include <map>
#include <simgrid/s4u/forward.hpp>
#include <simgrid/simix.h>
#include <string>
#include <unordered_map>
#include <xbt/base.h>

namespace simgrid {
namespace s4u {

XBT_PUBLIC_CLASS Storage
{
  friend s4u::Engine;

  Storage(std::string name, smx_storage_t inferior);

public:
  Storage() = default;
  virtual ~Storage();
  /** Retrieve a Storage by its name. It must exist in the platform file */
  static Storage& byName(const char* name);
  const char* name();
  const char* host();
  sg_size_t sizeFree();
  sg_size_t sizeUsed();
  /** Retrieve the total amount of space of this storage element */
  sg_size_t size();
  xbt_dict_t properties();
  const char* property(const char* key);
  void setProperty(const char* key, char* value);
  std::map<std::string, sg_size_t*>* content();
  std::unordered_map<std::string, Storage*>* allStorages();

protected:
  smx_storage_t inferior();

public:
  void setUserdata(void* data) { userdata_ = data; }
  void* userdata() { return userdata_; }

private:
  static std::unordered_map<std::string, Storage*>* storages_;

  std::string hostname_;
  std::string name_;
  sg_size_t size_      = 0;
  smx_storage_t pimpl_ = nullptr;
  void* userdata_      = nullptr;
};

} /* namespace s4u */
} /* namespace simgrid */

#endif /* INCLUDE_SIMGRID_S4U_STORAGE_HPP_ */
