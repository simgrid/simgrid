/* Copyright (c) 2009-2010, 2012-2017. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SMPI_INFO_HPP
#define SMPI_INFO_HPP

#include "smpi/smpi.h"
#include "smpi_f2c.hpp"
#include <string>
#include <map>

namespace simgrid{
namespace smpi{

class Info : public F2C{
  private:
    std::map<std::string, std::string> map_;
    int refcount_ = 1;

  public:
    Info() = default;
    explicit Info(Info* orig);
    ~Info() = default;
    void ref();
    static void unref(MPI_Info info);
    void set(char *key, char *value);
    int get(char *key,int valuelen, char *value, int *flag);
    int remove(char* key);
    int get_nkeys(int *nkeys);
    int get_nthkey(int n, char *key);
    int get_valuelen(char *key, int *valuelen, int *flag);
    static Info* f2c(int id);
};

}
}

#endif
