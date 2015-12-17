/* Copyright (c) 2015. The SimGrid Team. All rights reserved.               */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "xbt/sysdep.h"
#include "PropertyHolder.hpp"

namespace simgrid {
namespace surf {

PropertyHolder::PropertyHolder(xbt_dict_t props)
: p_properties(props)
{
}

PropertyHolder::~PropertyHolder() {
	xbt_dict_free(&p_properties);
}

/** @brief Return the property associated to the provided key (or NULL if not existing) */
const char *PropertyHolder::getProperty(const char*key) {
	if (p_properties == NULL)
		return NULL;
	return (const char*) xbt_dict_get_or_null(p_properties,key);
}

/** @brief Change the value of a given key in the property set */
void PropertyHolder::setProperty(const char*key, const char*value) {
	if (!p_properties)
		p_properties = xbt_dict_new();
	xbt_dict_set(p_properties, key, xbt_strdup(value), &xbt_free_f);
}

/** @brief Return the whole set of properties. Don't mess with it, dude! */
xbt_dict_t PropertyHolder::getProperties() {
	if (!p_properties)
		p_properties = xbt_dict_new();
	return p_properties;
}

} /* namespace surf */
} /* namespace simgrid */
