/* Copyright (c) 2004-2023. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SURF_MODEL_H_
#define SURF_MODEL_H_

#include <set>
#include <string>

/** set of hosts for which one want to be notified if they ever restart */
inline auto& watched_hosts() // avoid static initialization order fiasco
{
  static std::set<std::string, std::less<>> value;
  return value;
}
#endif /* SURF_MODEL_H_ */
