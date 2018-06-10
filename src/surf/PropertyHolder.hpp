/* Copyright (c) 2015-2018. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SRC_SURF_PROPERTYHOLDER_HPP_
#define SRC_SURF_PROPERTYHOLDER_HPP_
#include <string>
#include <unordered_map>

namespace simgrid {
namespace surf {

/** @brief a PropertyHolder can be given a set of textual properties
 *
 * Common PropertyHolders are elements of the platform file, such as Host, Link or Storage.
 */
class PropertyHolder { // DO NOT DERIVE THIS CLASS, or the diamond inheritance mayhem will get you

public:
  PropertyHolder() = default;
  ~PropertyHolder();

  const char* get_property(std::string key);
  void set_property(std::string id, std::string value);

  /* FIXME: This should not be exposed, as users may do bad things with the map they got (it's not a copy).
   * But some user API expose this call so removing it is not so easy.
   */
  std::unordered_map<std::string, std::string>* get_properties();

private:
  std::unordered_map<std::string, std::string>* properties_ = nullptr;
};

} /* namespace surf */
} /* namespace simgrid */

#endif /* SRC_SURF_PROPERTYHOLDER_HPP_ */
