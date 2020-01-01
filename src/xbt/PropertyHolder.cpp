/* Copyright (c) 2015-2020. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <xbt/PropertyHolder.hpp>

#include <map>

namespace simgrid {
namespace xbt {

/** @brief Return the property associated to the provided key (or nullptr if not existing) */
const char* PropertyHolder::get_property(const std::string& key) const
{
  if (not properties_)
    return nullptr;
  auto prop = properties_->find(key);
  return prop == properties_->end() ? nullptr : prop->second.c_str();
}

/** @brief Change the value of a given key in the property set */
void PropertyHolder::set_property(const std::string& key, const std::string& value)
{
  if (not properties_)
    properties_.reset(new std::unordered_map<std::string, std::string>);
  (*properties_)[key] = value;
}

/** @brief Return the whole set of properties. Don't mess with it, dude! */
const std::unordered_map<std::string, std::string>* PropertyHolder::get_properties()
{
  if (not properties_)
    properties_.reset(new std::unordered_map<std::string, std::string>);
  return properties_.get();
}

/** @brief Change the value of the given keys in the property set */
template <class Assoc> void PropertyHolder::set_properties(const Assoc& properties)
{
  if (not properties_)
    properties_.reset(new std::unordered_map<std::string, std::string>);
  std::unordered_map<std::string, std::string> props(properties.cbegin(), properties.cend());
#if __cplusplus >= 201703L
  props.merge(properties_);
#else
  props.insert(properties_->cbegin(), properties_->cend());
#endif
  properties_->swap(props);
}

template void PropertyHolder::set_properties(const std::map<std::string, std::string>& properties);
template void PropertyHolder::set_properties(const std::unordered_map<std::string, std::string>& properties);

} // namespace xbt
} // namespace simgrid
