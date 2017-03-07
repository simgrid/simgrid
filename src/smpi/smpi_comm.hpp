/* Copyright (c) 2010-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SMPI_COMM_HPP_INCLUDED
#define SMPI_COMM_HPP_INCLUDED

#include "private.h"

namespace simgrid{
namespace smpi{

class Comm {

  private:
    MPI_Group group_;
    MPIR_Topo_type topoType_; 
    MPI_Topology topo_; // to be replaced by an union
    int refcount_;
    MPI_Comm leaders_comm_;//inter-node communicator
    MPI_Comm intra_comm_;//intra-node communicator . For MPI_COMM_WORLD this can't be used, as var is global.
    //use an intracomm stored in the process data instead
    int* leaders_map_; //who is the leader of each process
    int is_uniform_;
    int* non_uniform_map_; //set if smp nodes have a different number of processes allocated
    int is_blocked_;// are ranks allocated on the same smp node contiguous ?
    xbt_dict_t attributes_;

  public:
    Comm();
    Comm(MPI_Group group, MPI_Topology topo);

    void destroy();
    int dup(MPI_Comm* newcomm);
    MPI_Group group();
    MPI_Topology topo();
    int size();
    int rank();
    void get_name (char* name, int* len);
    void set_leaders_comm(MPI_Comm leaders);
    void set_intra_comm(MPI_Comm leaders);
    int* get_non_uniform_map();
    int* get_leaders_map();
    MPI_Comm get_leaders_comm();
    MPI_Comm get_intra_comm();
    int is_uniform();
    int is_blocked();
    MPI_Comm split(int color, int key);
    void use();
    void cleanup_attributes();
    void cleanup_smp();
    void unuse();
    void init_smp();
    int attr_delete(int keyval);
    int attr_get(int keyval, void* attr_value, int* flag);
    int attr_put(int keyval, void* attr_value);

};

}
}


#endif
