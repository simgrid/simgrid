/* Copyright (c) 2006-2018. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef INCLUDE_SIMGRID_S4U_STORAGE_HPP_
#define INCLUDE_SIMGRID_S4U_STORAGE_HPP_

#include <simgrid/forward.h>
#include <simgrid/s4u/Io.hpp>
#include <xbt/Extendable.hpp>
#include <xbt/base.h>
#include <xbt/signal.hpp>

#include <map>
#include <string>
#include <unordered_map>

namespace simgrid {
namespace s4u {

#ifndef DOXYGEN
/** @deprecated Engine::get_all_storages() */
XBT_ATTRIB_DEPRECATED_v322("Please use Engine::get_all_storages()") XBT_PUBLIC void getStorageList(std::map<std::string, Storage*>* whereTo);
#endif

/** Storage represent the disk resources, usually associated to a given host
 *
 * By default, SimGrid does not keep track of the actual data being written but
 * only computes the time taken by the corresponding data movement.
 */

class XBT_PUBLIC Storage : public simgrid::xbt::Extendable<Storage> {
  friend simgrid::s4u::Engine;
  friend simgrid::s4u::Io;
  friend simgrid::surf::StorageImpl;

public:
  explicit Storage(std::string name, surf::StorageImpl * pimpl);

protected:
  virtual ~Storage() = default;
public:
  /** @brief Callback signal fired when a new Storage is created */
  static simgrid::xbt::signal<void(s4u::Storage&)> on_creation;
  /** @brief Callback signal fired when a Storage is destroyed */
  static simgrid::xbt::signal<void(s4u::Storage&)> on_destruction;
  /** @brief Callback signal fired when a Storage's state changes */
  static simgrid::xbt::signal<void(s4u::Storage&)> on_state_change;

  /** Retrieve a Storage by its name. It must exist in the platform file */
  static Storage* by_name(std::string name);
  static Storage* by_name_or_null(std::string name);

  /** @brief Retrieves the name of that storage as a C++ string */
  std::string const& get_name() const { return name_; }
  /** @brief Retrieves the name of that storage as a C string */
  const char* get_cname() const { return name_.c_str(); }

  const char* get_type();
  Host* get_host() { return attached_to_; };
  void set_host(Host* host) { attached_to_ = host; }

  std::unordered_map<std::string, std::string>* get_properties();
  const char* get_property(std::string key);
  void set_property(std::string, std::string value);

  void set_data(void* data) { userdata_ = data; }
  void* get_data() { return userdata_; }

  IoPtr io_init(sg_size_t size, s4u::Io::OpType type);

  IoPtr read_async(sg_size_t size);
  sg_size_t read(sg_size_t size);

  IoPtr write_async(sg_size_t size);
  sg_size_t write(sg_size_t size);
  surf::StorageImpl* get_impl() { return pimpl_; }

  // Deprecated functions
  /** @deprecated Storage::by_name() */
  XBT_ATTRIB_DEPRECATED_v323("Please use Storage::by_name()") Storage* byName(std::string name)
  {
    return by_name(name);
  }
  /** @deprecated Storage::get_name() */
  XBT_ATTRIB_DEPRECATED_v323("Please use Storage::get_name()") std::string const& getName() const { return get_name(); }
  /** @deprecated Storage::get_cname() */
  XBT_ATTRIB_DEPRECATED_v323("Please use Storage::get_cname()") const char* getCname() const { return get_cname(); }
  /** @deprecated Storage::get_type() */
  XBT_ATTRIB_DEPRECATED_v323("Please use Storage::get_type()") const char* getType() { return get_type(); }
  /** @deprecated Storage::get_host() */
  XBT_ATTRIB_DEPRECATED_v323("Please use Storage::get_host()") Host* getHost() { return get_host(); }
  /** @deprecated Storage::get_properties() */
  XBT_ATTRIB_DEPRECATED_v323("Please use Storage::get_properties()") std::map<std::string, std::string>* getProperties()
  {
    std::map<std::string, std::string>* res             = new std::map<std::string, std::string>();
    std::unordered_map<std::string, std::string>* props = get_properties();
    for (auto const& kv : *props)
      res->insert(kv);
    return res;
  }
  /** @deprecated Storage::get_property() */
  XBT_ATTRIB_DEPRECATED_v323("Please use Storage::get_property()") const char* getProperty(const char* key)
  {
    return get_property(key);
  }
  /** @deprecated Storage::set_property() */
  XBT_ATTRIB_DEPRECATED_v323("Please use Storage::set_property()") void setProperty(std::string key, std::string value)
  {
    set_property(key, value);
  }
  /** @deprecated Storage::set_data() */
  XBT_ATTRIB_DEPRECATED_v323("Please use Storage::set_data()") void setUserdata(void* data) { set_data(data); }
  /** @deprecated Storage::get_data() */
  XBT_ATTRIB_DEPRECATED_v323("Please use Storage::get_data()") void* getUserdata() { return get_data(); }

private:
  Host* attached_to_              = nullptr;
  surf::StorageImpl* const pimpl_ = nullptr;
  std::string name_;
  void* userdata_ = nullptr;
};

} /* namespace s4u */
} /* namespace simgrid */

#endif /* INCLUDE_SIMGRID_S4U_STORAGE_HPP_ */
