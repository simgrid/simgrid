/* Copyright (c) 2007-2014. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <stddef.h>
#include <set>

typedef std::set<const void*>*  mc_address_set_t;

extern "C" {

mc_address_set_t mc_address_set_new();
void mc_address_set_free(mc_address_set_t* p);
void mc_address_add(mc_address_set_t p, const void* value);
bool mc_address_test(mc_address_set_t p, const void* value);

mc_address_set_t mc_address_set_new() {
  return new std::set<const void*>();
}

void mc_address_set_free(mc_address_set_t* p) {
  delete *p;
  *p = NULL;
}

void mc_address_add(mc_address_set_t p, const void* value) {
  p->insert(value);
}

bool mc_address_test(mc_address_set_t p, const void* value) {
  return p->find(value) != p->end();
}

};
