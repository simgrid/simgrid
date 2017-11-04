/* Copyright (c) 2015-2017. The SimGrid Team. All rights reserved.               */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "PropertyHolder.hpp"

namespace simgrid {
namespace surf {

PropertyHolder::~PropertyHolder() {
  delete properties_;
}

/** @brief Return the property associated to the provided key (or nullptr if not existing) */
const char* PropertyHolder::getProperty(std::string key)
{
  if (properties_ == nullptr)
    return nullptr;
  auto prop = properties_->find(key);
  return prop == properties_->end() ? nullptr : prop->second.c_str();
}

/** @brief Change the value of a given key in the property set */
void PropertyHolder::setProperty(std::string key, std::string value)
{
  if (not properties_)
    properties_       = new std::map<std::string, std::string>;
  (*properties_)[key] = value;
}

/** @brief Return the whole set of properties. Don't mess with it, dude! */
std::map<std::string, std::string>* PropertyHolder::getProperties()
{
  if (not properties_)
    properties_ = new std::map<std::string, std::string>;
  return properties_;
}

} /* namespace surf */
} /* namespace simgrid */
