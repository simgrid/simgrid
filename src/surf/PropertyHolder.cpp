/* Copyright (c) 2015. The SimGrid Team. All rights reserved.               */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "xbt/sysdep.h"
#include "PropertyHolder.hpp"

namespace simgrid {
namespace surf {

PropertyHolder::PropertyHolder() = default;

PropertyHolder::~PropertyHolder() {
  xbt_dict_free(&properties_);
}

/** @brief Return the property associated to the provided key (or nullptr if not existing) */
const char *PropertyHolder::getProperty(const char*key) {
  if (properties_ == nullptr)
    return nullptr;
  return (const char*) xbt_dict_get_or_null(properties_,key);
}

/** @brief Change the value of a given key in the property set */
void PropertyHolder::setProperty(const char*key, const char*value) {
  if (!properties_)
    properties_ = xbt_dict_new_homogeneous(xbt_free_f);
  xbt_dict_set(properties_, key, xbt_strdup(value), nullptr);
}

/** @brief Return the whole set of properties. Don't mess with it, dude! */
xbt_dict_t PropertyHolder::getProperties() {
  if (!properties_)
    properties_ = xbt_dict_new_homogeneous(xbt_free_f);
  return properties_;
}

} /* namespace surf */
} /* namespace simgrid */
