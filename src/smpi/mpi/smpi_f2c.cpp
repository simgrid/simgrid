/* Copyright (c) 2007-2019. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "smpi_f2c.hpp"
#include "private.hpp"
#include "src/smpi/include/smpi_actor.hpp"

#include <cstdio>

int mpi_in_place_;
int mpi_bottom_;
int mpi_status_ignore_;
int mpi_statuses_ignore_;

namespace simgrid{
namespace smpi{

std::unordered_map<std::string, F2C*>* F2C::f2c_lookup_ = nullptr;
int F2C::f2c_id_ = 0;

std::unordered_map<std::string, F2C*>* F2C::f2c_lookup()
{
  return f2c_lookup_;
}

void F2C::set_f2c_lookup(std::unordered_map<std::string, F2C*>* map)
{
  f2c_lookup_ = map;
}

void F2C::f2c_id_increment(){
  f2c_id_++;
};

int F2C::f2c_id(){
  return f2c_id_;
};

char* F2C::get_my_key(char* key) {
  std::snprintf(key, KEY_SIZE, "%d", my_f2c_id_);
  return key;
}

char* F2C::get_key(char* key, int id) {
  std::snprintf(key, KEY_SIZE, "%d", id);
  return key;
}

void F2C::delete_lookup(){
  delete f2c_lookup_;
}

std::unordered_map<std::string, F2C*>* F2C::lookup()
{
  return f2c_lookup_;
}

void F2C::free_f(int id)
{
  char key[KEY_SIZE];
  f2c_lookup_->erase(get_key(key,id));
}

int F2C::add_f()
{
  if (f2c_lookup_ == nullptr)
    f2c_lookup_ = new std::unordered_map<std::string, F2C*>;

  char key[KEY_SIZE];
  my_f2c_id_=f2c_id_;
  (*f2c_lookup_)[get_my_key(key)] = this;
  f2c_id_increment();
  return my_f2c_id_;
}

int F2C::c2f()
{
  if (f2c_lookup_ == nullptr) {
    f2c_lookup_ = new std::unordered_map<std::string, F2C*>;
  }

  if(my_f2c_id_==-1)
    /* this function wasn't found, add it */
    return this->add_f();
  else
    return my_f2c_id_;
}

F2C* F2C::f2c(int id)
{
  if (f2c_lookup_ == nullptr)
    f2c_lookup_ = new std::unordered_map<std::string, F2C*>;

  if(id >= 0){
    char key[KEY_SIZE];
    auto comm = f2c_lookup_->find(get_key(key,id));
    return comm == f2c_lookup_->end() ? nullptr : comm->second;
  }else
    return nullptr;
}

}
}
