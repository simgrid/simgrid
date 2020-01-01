/* Copyright (c) 2015-2020. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef XBT_PROPERTYHOLDER_HPP
#define XBT_PROPERTYHOLDER_HPP

#include <memory>
#include <string>
#include <unordered_map>

namespace simgrid {
namespace xbt {

/** @brief a PropertyHolder can be given a set of textual properties
 *
 * Common PropertyHolders are elements of the platform file, such as Host, Link or Storage.
 */
class PropertyHolder { // DO NOT DERIVE THIS CLASS, or the diamond inheritance mayhem will get you
  std::unique_ptr<std::unordered_map<std::string, std::string>> properties_ = nullptr;

public:
  PropertyHolder()                      = default;
  PropertyHolder(const PropertyHolder&) = delete;
  PropertyHolder& operator=(const PropertyHolder&) = delete;

  const char* get_property(const std::string& key) const;
  void set_property(const std::string& id, const std::string& value);

  const std::unordered_map<std::string, std::string>* get_properties();
  template <class Assoc> void set_properties(const Assoc& properties);
};

} // namespace xbt
} // namespace simgrid

#endif
