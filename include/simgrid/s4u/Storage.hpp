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

XBT_ATTRIB_PUBLIC std::map<std::string, Storage*>* allStorages();

XBT_PUBLIC_CLASS Storage
{
  friend s4u::Engine;
  friend simgrid::surf::StorageImpl;

public:
  explicit Storage(surf::StorageImpl * pimpl) : pimpl_(pimpl) {}
  virtual ~Storage() = default;
  /** Retrieve a Storage by its name. It must exist in the platform file */
  static Storage* byName(const char* name);
  const char* name();
  const char* type();
  Host* host();
  sg_size_t sizeFree();
  sg_size_t sizeUsed();
  /** Retrieve the total amount of space of this storage element */
  sg_size_t size();

  xbt_dict_t properties();
  const char* property(const char* key);
  void setProperty(const char* key, char* value);
  std::map<std::string, sg_size_t>* content();

  void setUserdata(void* data) { userdata_ = data; }
  void* userdata() { return userdata_; }

  /* The signals */
  /** @brief Callback signal fired when a new Link is created */
  static simgrid::xbt::signal<void(s4u::Storage&)> onCreation;

  /** @brief Callback signal fired when a Link is destroyed */
  static simgrid::xbt::signal<void(s4u::Storage&)> onDestruction;

  Host* attached_to_              = nullptr;
  surf::StorageImpl* const pimpl_ = nullptr;

private:
  std::string name_;
  void* userdata_ = nullptr;
};

} /* namespace s4u */
} /* namespace simgrid */

#endif /* INCLUDE_SIMGRID_S4U_STORAGE_HPP_ */
