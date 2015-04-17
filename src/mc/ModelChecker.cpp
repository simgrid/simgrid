/* Copyright (c) 2008-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <cassert>

#include "ModelChecker.hpp"
#include "PageStore.hpp"

::simgrid::mc::ModelChecker* mc_model_checker = NULL;

namespace simgrid {
namespace mc {

ModelChecker::ModelChecker(pid_t pid, int socket)
  : page_store_(500)
{
  this->hostnames_ = xbt_dict_new();
  MC_process_init(&this->process(), pid, socket);
}

ModelChecker::~ModelChecker()
{
  MC_process_clear(&this->process_);
  xbt_dict_free(&this->hostnames_);
}

const char* ModelChecker::get_host_name(const char* hostname)
{
  // Lookup the host name in the dictionary (or create it):
  xbt_dictelm_t elt = xbt_dict_get_elm_or_null(this->hostnames_, hostname);
  if (!elt) {
    xbt_mheap_t heap = mmalloc_set_current_heap(mc_heap);
    xbt_dict_set(this->hostnames_, hostname, NULL, NULL);
    elt = xbt_dict_get_elm_or_null(this->hostnames_, hostname);
    assert(elt);
    mmalloc_set_current_heap(heap);
  }
  return elt->key;
}

}
}
