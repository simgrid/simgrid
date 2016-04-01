/* Copyright (c) 2015. The SimGrid Team. All rights reserved.               */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "xbt/sysdep.h"
#include "PropertyHolder.hpp"

namespace simgrid {
namespace surf {

PropertyHolder::PropertyHolder(xbt_dict_t props)
: properties_(props)
{
}

PropertyHolder::~PropertyHolder() {
  xbt_dict_free(&properties_);
}

/** @brief Return the property associated to the provided key (or NULL if not existing) */
const char *PropertyHolder::getProperty(const char*key) {
  if (properties_ == NULL)
    return NULL;
  return (const char*) xbt_dict_get_or_null(properties_,key);
}

/** @brief Change the value of a given key in the property set */
void PropertyHolder::setProperty(const char*key, const char*value) {
  if (!properties_)
    properties_ = xbt_dict_new();
  xbt_dict_set(properties_, key, xbt_strdup(value), &xbt_free_f);
}

/** @brief Return the whole set of properties. Don't mess with it, dude! */
xbt_dict_t PropertyHolder::getProperties() {
  if (!properties_)
    properties_ = xbt_dict_new();
  return properties_;
}

} /* namespace surf */
} /* namespace simgrid */
