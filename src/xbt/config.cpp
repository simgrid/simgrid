/* Copyright (c) 2016. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <functional>
#include <string>

#include <xbt/config.h>
#include <xbt/config.hpp>

namespace simgrid {
namespace config {

static void callCallback(const char* name, void* data)
{
  (*(std::function<void(const char*)>*) data)(name);
}

static void freeCallback(void* data)
{
  delete (std::function<void(const char*)>*) data;
}

void registerConfig(const char* name, const char* description,
  e_xbt_cfgelm_type_t type,
  std::function<void(const char*)> callback)
{
  std::function<void(const char*)>* code
    = new std::function<void(const char*)>(std::move(callback));
  xbt_cfg_register_ext(name, description, type,
    callCallback, code, freeCallback);
}

}
}