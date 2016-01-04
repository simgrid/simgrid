/* Copyright (c) 2015. The SimGrid Team. All rights reserved.               */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SRC_SURF_PROPERTYHOLDER_HPP_
#define SRC_SURF_PROPERTYHOLDER_HPP_
#include <xbt/dict.h>

namespace simgrid {
namespace surf {

/** @brief a PropertyHolder can be given a set of textual properties
 *
 * Common PropertyHolders are elements of the platform file, such as Host, Link or Storage.
 */
class PropertyHolder { // DO NOT DERIVE THIS CLASS, or the diamond inheritance mayhem will get you

public:
	PropertyHolder(xbt_dict_t props);
	~PropertyHolder();

	const char *getProperty(const char*id);
	void setProperty(const char*id, const char*value);

	/* FIXME: This should not be exposed, as users may do bad things with the dict they got (it's not a copy).
	 * But some user API expose this call so removing it is not so easy.
	 */
	xbt_dict_t getProperties();
private:
	xbt_dict_t p_properties = NULL;
};

} /* namespace surf */
} /* namespace simgrid */

#endif /* SRC_SURF_PROPERTYHOLDER_HPP_ */
