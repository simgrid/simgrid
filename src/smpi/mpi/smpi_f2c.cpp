/* Copyright (c) 2007-2021. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "smpi_f2c.hpp"
#include "private.hpp"
#include "src/smpi/include/smpi_actor.hpp"

int mpi_in_place_;
int mpi_bottom_;
int mpi_status_ignore_;
int mpi_statuses_ignore_;

namespace simgrid{
namespace smpi{

std::unique_ptr<F2C::f2c_lookup_type> F2C::f2c_lookup_    = nullptr;
int F2C::f2c_id_ = 0;
F2C::f2c_lookup_type::size_type F2C::num_default_handles_ = 0;

// Keep it non trivially-constructible, or it will break MC+smpi on FreeBSD with Clang (don't ask why)
F2C::F2C() = default;

int F2C::add_f()
{
  allocate_lookup();

  my_f2c_id_                 = f2c_id();
  (*f2c_lookup_)[my_f2c_id_] = this;
  f2c_id_increment();
  return my_f2c_id_;
}

int F2C::c2f()
{
  allocate_lookup();

  if(my_f2c_id_==-1)
    /* this function wasn't found, add it */
    return this->add_f();
  else
    return my_f2c_id_;
}

F2C* F2C::f2c(int id)
{
  allocate_lookup();

  if(id >= 0){
    auto comm = f2c_lookup_->find(id);
    return comm == f2c_lookup_->end() ? nullptr : comm->second;
  }else
    return nullptr;
}

}
}
