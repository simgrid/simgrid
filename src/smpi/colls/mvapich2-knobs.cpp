/* Copyright (c) 2009-2026. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/* Knobs the ported MVAPICH kernels read at global scope. They used to live in the
 * Stampede tuning tables, which are gone now that the mvapich2 selector is generated
 * from MVAPICH's csel JSON: what remains are the parameters of the two-level and smp
 * kernels (knomial factors, segment size) and the function pointers they fall back to.
 * Defaults are MVAPICH's, and the generated selectors never read them. */

#include "colls_private.hpp"

typedef int (*MV2_Gather_function_ptr)(const void* sendbuf, int sendcnt, MPI_Datatype sendtype, void* recvbuf,
                                       int recvcnt, MPI_Datatype recvtype, int root, MPI_Comm comm);

int (*MV2_Allreduce_intra_function)(const void* sendbuf, void* recvbuf, int count, MPI_Datatype datatype, MPI_Op op,
                                    MPI_Comm comm)                                                        = nullptr;
int (*MV2_Allreducection)(const void* sendbuf, void* recvbuf, int count, MPI_Datatype datatype, MPI_Op op,
                          MPI_Comm comm)                                                              = nullptr;
int (*MV2_Bcast_function)(void* buffer, int count, MPI_Datatype datatype, int root, MPI_Comm comm_ptr)     = nullptr;
int (*MV2_Bcast_intra_node_function)(void* buffer, int count, MPI_Datatype datatype, int root,
                                     MPI_Comm comm_ptr)                                                   = nullptr;
int (*MV2_Reduce_function)(const void* sendbuf, void* recvbuf, int count, MPI_Datatype datatype, MPI_Op op,
                           int root, MPI_Comm comm_ptr)                                                   = nullptr;
int (*MV2_Reduce_intra_function)(const void* sendbuf, void* recvbuf, int count, MPI_Datatype datatype, MPI_Op op,
                                 int root, MPI_Comm comm_ptr)                                             = nullptr;
int (*MV2_Scatter_intra_function)(const void* sendbuf, int sendcount, MPI_Datatype sendtype, void* recvbuf,
                                 int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm)           = nullptr;

MV2_Gather_function_ptr MV2_Gather_inter_leader_function = nullptr;
MV2_Gather_function_ptr MV2_Gather_intra_node_function   = nullptr;

int zcpy_knomial_factor               = 2;
int mv2_pipelined_zcpy_knomial_factor = -1;
int bcast_segment_size                = 8192;
int mv2_inter_node_knomial_factor     = 4;
int mv2_intra_node_knomial_factor     = 4;
int mv2_reduce_intra_knomial_factor   = -1;
int mv2_reduce_inter_knomial_factor   = -1;
