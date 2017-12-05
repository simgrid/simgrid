/* Copyright (c) 2006-2015, 2017. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef INCLUDE_SIMGRID_S4U_STORAGE_HPP_
#define INCLUDE_SIMGRID_S4U_STORAGE_HPP_

#include "xbt/Extendable.hpp"
#include <map>
#include <simgrid/s4u/forward.hpp>
#include <simgrid/simix.h>
#include <string>
#include <unordered_map>
#include <xbt/base.h>

namespace simgrid {
namespace xbt {
extern template class XBT_PUBLIC() Extendable<simgrid::s4u::Storage>;
}
namespace s4u {

XBT_ATTRIB_PUBLIC void getStorageList(std::map<std::string, Storage*>* whereTo);

XBT_PUBLIC_CLASS Storage : public simgrid::xbt::Extendable<Storage>
{
  friend s4u::Engine;
  friend simgrid::surf::StorageImpl;

public:
  explicit Storage(surf::StorageImpl * pimpl) : pimpl_(pimpl) {}
  virtual ~Storage() = default;
  /** Retrieve a Storage by its name. It must exist in the platform file */
  static Storage* byName(std::string name);
  /** @brief Retrieves the name of that storage as a C++ string */
  std::string const& getName() const;
  /** @brief Retrieves the name of that storage as a C string */
  const char* getCname() const;
  const char* getType();
  Host* getHost();

  std::map<std::string, std::string>* getProperties();
  const char* getProperty(std::string key);
  void setProperty(std::string, std::string value);

  void setUserdata(void* data) { userdata_ = data; }
  void* getUserdata() { return userdata_; }

  sg_size_t read(sg_size_t size);
  sg_size_t write(sg_size_t size);
  surf::StorageImpl* getImpl() { return pimpl_; }

  /* The signals */
  /** @brief Callback signal fired when a new Link is created */
  static simgrid::xbt::signal<void(s4u::Storage&)> onCreation;

  /** @brief Callback signal fired when a Link is destroyed */
  static simgrid::xbt::signal<void(s4u::Storage&)> onDestruction;

  Host* attached_to_              = nullptr;

private:
  surf::StorageImpl* const pimpl_ = nullptr;
  std::string name_;
  void* userdata_ = nullptr;
};

} /* namespace s4u */
} /* namespace simgrid */

#endif /* INCLUDE_SIMGRID_S4U_STORAGE_HPP_ */
