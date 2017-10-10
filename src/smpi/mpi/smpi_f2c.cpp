/* Copyright (c) 2007-2017. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "smpi_f2c.hpp"
#include "private.hpp"
#include "smpi_process.hpp"

#include <cstdio>

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

char* F2C::get_key(char* key, int id) {
  std::snprintf(key, KEY_SIZE, "%x", static_cast<unsigned>(id));
  return key;
}

char* F2C::get_key_id(char* key, int id) {
  std::snprintf(key, KEY_SIZE, "%x_%d", static_cast<unsigned>(id), smpi_process()->index());
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
  f2c_lookup_->erase(get_key(key, id));
}

int F2C::add_f()
{
  if (f2c_lookup_ == nullptr)
    f2c_lookup_ = new std::unordered_map<std::string, F2C*>;

  char key[KEY_SIZE];
  (*f2c_lookup_)[get_key(key, f2c_id_)] = this;
  f2c_id_increment();
  return f2c_id_-1;
}

int F2C::c2f()
{
  if (f2c_lookup_ == nullptr) {
    f2c_lookup_ = new std::unordered_map<std::string, F2C*>;
  }

  for (auto const& elm : *f2c_lookup_)
    if (elm.second == this)
      return std::stoi(elm.first);

  /* this function wasn't found, add it */
  return this->add_f();
}

F2C* F2C::f2c(int id)
{
  if (f2c_lookup_ == nullptr)
    f2c_lookup_ = new std::unordered_map<std::string, F2C*>;

  if(id >= 0){
    char key[KEY_SIZE];
    auto comm = f2c_lookup_->find(get_key(key, id));
    return comm == f2c_lookup_->end() ? nullptr : comm->second;
  }else
    return nullptr;
}

}
}
