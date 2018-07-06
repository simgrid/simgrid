/* Copyright (c) 2010-2018. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SMPI_GROUP_HPP_INCLUDED
#define SMPI_GROUP_HPP_INCLUDED

#include "smpi_f2c.hpp"
#include <smpi/smpi.h>
#include <map>
#include <vector>

namespace simgrid{
namespace smpi{

class Group : public F2C{
  private:
    int size_;
    /* This is actually a map from int to int. We could use
     * std::map here, but looking up a value there costs O(log(n)).
     * For a vector, this costs O(1). We hence go with the vector.
     */
    std::vector<simgrid::s4u::ActorPtr> rank_to_actor_map_;
    std::map<simgrid::s4u::ActorPtr, int> actor_to_rank_map_;
    std::vector<int> index_to_rank_map_;

    int refcount_;
  public:
    explicit Group();
    explicit Group(int size);
    explicit Group(Group* origin);

    void set_mapping(simgrid::s4u::ActorPtr actor, int rank);
    int rank(int index);
    simgrid::s4u::ActorPtr actor(int rank);
    int rank(const simgrid::s4u::ActorPtr process);
    void ref();
    static void unref(MPI_Group group);
    int size();
    int compare(MPI_Group group2);
    int incl(int n, int* ranks, MPI_Group* newgroup);
    int excl(int n, int *ranks, MPI_Group * newgroup);
    int group_union(MPI_Group group2, MPI_Group* newgroup);
    int intersection(MPI_Group group2, MPI_Group* newgroup);
    int difference(MPI_Group group2, MPI_Group* newgroup);
    int range_incl(int n, int ranges[][3], MPI_Group * newgroup);
    int range_excl(int n, int ranges[][3], MPI_Group * newgroup);

    static Group* f2c(int id);

};
}
}

#endif
