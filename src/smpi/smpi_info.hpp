/* Copyright (c) 2009-2010, 2012-2017. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SMPI_INFO_HPP
#define SMPI_INFO_HPP

#include "src/smpi/smpi_f2c.hpp"
#include "smpi/smpi.h"
#include "xbt/dict.h"

namespace simgrid{
namespace smpi{

class Info : public F2C{
  private:
    xbt_dict_t dict_;
    int refcount_;
  public:
    explicit Info();
    explicit Info(Info* orig);
    ~Info();
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
