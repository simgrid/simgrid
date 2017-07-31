/* Copyright (c) 2015. The SimGrid Team. All rights reserved.               */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "PropertyHolder.hpp"

namespace simgrid {
namespace surf {

PropertyHolder::PropertyHolder() = default;

PropertyHolder::~PropertyHolder() {
  delete properties_;
}

/** @brief Return the property associated to the provided key (or nullptr if not existing) */
const char *PropertyHolder::getProperty(const char*key) {
  if (properties_ == nullptr)
    return nullptr;
  try {
    return properties_->at(key).c_str();
  } catch (std::out_of_range& unfound) {
    return nullptr;
  }
}

/** @brief Change the value of a given key in the property set */
void PropertyHolder::setProperty(const char*key, const char*value) {
  if (not properties_)
    properties_       = new std::unordered_map<std::string, std::string>;
  (*properties_)[key] = value;
}

/** @brief Return the whole set of properties. Don't mess with it, dude! */
std::unordered_map<std::string, std::string>* PropertyHolder::getProperties()
{
  if (not properties_)
    properties_ = new std::unordered_map<std::string, std::string>;
  return properties_;
}

} /* namespace surf */
} /* namespace simgrid */
