/* Copyright (c) 2007-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "private.h"
#include <vector>


namespace simgrid{
namespace smpi{

xbt_dict_t F2C::f2c_lookup_=nullptr;
int F2C::f2c_id_=0;

xbt_dict_t F2C::f2c_lookup(){
  return f2c_lookup_;
}

void F2C::set_f2c_lookup(xbt_dict_t dict){
  f2c_lookup_=dict;
}

void F2C::f2c_id_increment(){
  f2c_id_++;
};

int F2C::f2c_id(){
  return f2c_id_;
};

char* F2C::get_key(char* key, int id) {
  snprintf(key, KEY_SIZE, "%x",id);
  return key;
}

char* F2C::get_key_id(char* key, int id) {
  snprintf(key, KEY_SIZE, "%x_%d",id, smpi_process()->index());
  return key;
}

void F2C::delete_lookup(){
  xbt_dict_free(&f2c_lookup_);
}

xbt_dict_t F2C::lookup(){
  return f2c_lookup_;
}

void F2C::free_f(int id){
  char key[KEY_SIZE];
  xbt_dict_remove(f2c_lookup_, get_key(key, id));
}

int F2C::add_f(){
  if(f2c_lookup_==nullptr){
    f2c_lookup_=xbt_dict_new_homogeneous(nullptr);
  }
  char key[KEY_SIZE];
  xbt_dict_set(f2c_lookup_, get_key(key, f2c_id_), this, nullptr);
  f2c_id_increment();
  return f2c_id_-1;
}

int F2C::c2f(){
  if(f2c_lookup_==nullptr){
    f2c_lookup_=xbt_dict_new_homogeneous(nullptr);
  }

  char* existing_key = xbt_dict_get_key(f2c_lookup_, this);
  if(existing_key!=nullptr){
    return atoi(existing_key);}
  else{
    return this->add_f();}
}

F2C* F2C::f2c(int id){
  if(f2c_lookup_==nullptr){
    f2c_lookup_=xbt_dict_new_homogeneous(nullptr);
  }
  char key[KEY_SIZE];
  if(id >= 0){
    return static_cast<F2C*>(xbt_dict_get_or_null(f2c_lookup_, get_key(key, id)));
  }else
    return NULL;
}

}
}
