/* Copyright (c) 2006-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/s4u/storage.hpp"

#include "xbt/lib.h"
extern xbt_lib_t storage_lib;

namespace simgrid {
namespace s4u {

boost::unordered_map <std::string, Storage *> *Storage::storages_ = new boost::unordered_map<std::string, Storage*> ();
Storage::Storage(std::string name, smx_storage_t inferior) {
  name_ = name;
  inferior_ = inferior;

  storages_->insert({name, this});
}

Storage::~Storage() {
  // TODO Auto-generated destructor stub
}

smx_storage_t Storage::inferior() {
  return inferior_;
}
Storage &Storage::byName(const char*name) {
  s4u::Storage *res = NULL;
  try {
    res = storages_->at(name);
  } catch (std::out_of_range& e) {
    smx_storage_t inferior = xbt_lib_get_elm_or_null(storage_lib,name);
    if (inferior == NULL)
      xbt_die("Storage %s does not exist. Please only use the storages that are defined in your platform.", name);

    res = new Storage(name,inferior);
  }
  return *res;
}

const char*Storage::name() {
  return name_.c_str();
}

sg_size_t Storage::sizeFree() {
  return simcall_storage_get_free_size(inferior_);
}
sg_size_t Storage::sizeUsed() {
  return simcall_storage_get_used_size(inferior_);
}
sg_size_t Storage::size() {
  return SIMIX_storage_get_size(inferior_);
}

} /* namespace s4u */
} /* namespace simgrid */
