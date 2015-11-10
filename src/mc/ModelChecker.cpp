/* Copyright (c) 2008-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <cassert>

#include "simgrid/sg_config.h" // sg_cfg_get_boolean

#include "ModelChecker.hpp"
#include "PageStore.hpp"

::simgrid::mc::ModelChecker* mc_model_checker = NULL;

namespace simgrid {
namespace mc {

ModelChecker::ModelChecker(pid_t pid, int socket) :
  hostnames_(xbt_dict_new()),
  page_store_(500),
  process_(pid, socket),
  parent_snapshot_(nullptr)
{
  // TODO, avoid direct dependency on sg_cfg
  process_.privatized(sg_cfg_get_boolean("smpi/privatize_global_variables"));
}

ModelChecker::~ModelChecker()
{
  xbt_dict_free(&this->hostnames_);
}

const char* ModelChecker::get_host_name(const char* hostname)
{
  // Lookup the host name in the dictionary (or create it):
  xbt_dictelm_t elt = xbt_dict_get_elm_or_null(this->hostnames_, hostname);
  if (!elt) {
    xbt_dict_set(this->hostnames_, hostname, NULL, NULL);
    elt = xbt_dict_get_elm_or_null(this->hostnames_, hostname);
    assert(elt);
  }
  return elt->key;
}

}
}
