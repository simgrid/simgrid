#ifndef SMPI_COLLS_H
#define SMPI_COLLS_H

#include "smpi/mpi.h"
#include "xbt.h"

int smpi_coll_tuned_alltoall_2dmesh(
  void * send_buff, int send_count, MPI_Datatype send_type,
  void * recv_buff, int recv_count, MPI_Datatype recv_type,
  MPI_Comm comm);
int smpi_coll_tuned_alltoall_3dmesh(
  void * send_buff, int send_count, MPI_Datatype send_type,
  void * recv_buff, int recv_count, MPI_Datatype recv_type,
  MPI_Comm comm);
/*int smpi_coll_tuned_alltoall_bruck(
  void * send_buff, int send_count, MPI_Datatype send_type,
  void * recv_buff, int recv_count, MPI_Datatype recv_type,
  MPI_Comm comm);*/
int smpi_coll_tuned_alltoall_pair(
  void * send_buff, int send_count, MPI_Datatype send_type,
  void * recv_buff, int recv_count, MPI_Datatype recv_type,
  MPI_Comm comm);
int smpi_coll_tuned_alltoall_pair_light_barrier(
  void * send_buff, int send_count, MPI_Datatype send_type,
  void * recv_buff, int recv_count, MPI_Datatype recv_type,
  MPI_Comm comm);
int smpi_coll_tuned_alltoall_pair_mpi_barrier(
  void * send_buff, int send_count, MPI_Datatype send_type,
  void * recv_buff, int recv_count, MPI_Datatype recv_type,
  MPI_Comm comm);
int smpi_coll_tuned_alltoall_pair_one_barrier(
  void * send_buff, int send_count, MPI_Datatype send_type,
  void * recv_buff, int recv_count, MPI_Datatype recv_type,
  MPI_Comm comm);
int smpi_coll_tuned_alltoall_rdb(
  void * send_buff, int send_count, MPI_Datatype send_type,
  void * recv_buff, int recv_count, MPI_Datatype recv_type,
  MPI_Comm comm);
int smpi_coll_tuned_alltoall_ring(
  void * send_buff, int send_count, MPI_Datatype send_type,
  void * recv_buff, int recv_count, MPI_Datatype recv_type,
  MPI_Comm comm);
int smpi_coll_tuned_alltoall_ring_light_barrier(
  void * send_buff, int send_count, MPI_Datatype send_type,
  void * recv_buff, int recv_count, MPI_Datatype recv_type,
  MPI_Comm comm);
int smpi_coll_tuned_alltoall_ring_mpi_barrier(
  void * send_buff, int send_count, MPI_Datatype send_type,
  void * recv_buff, int recv_count, MPI_Datatype recv_type,
  MPI_Comm comm);
int smpi_coll_tuned_alltoall_ring_one_barrier(
  void * send_buff, int send_count, MPI_Datatype send_type,
  void * recv_buff, int recv_count, MPI_Datatype recv_type,
  MPI_Comm comm);
int smpi_coll_tuned_alltoall_simple(
  void * send_buff, int send_count, MPI_Datatype send_type,
  void * recv_buff, int recv_count, MPI_Datatype recv_type,
  MPI_Comm comm);


int smpi_coll_tuned_allgather_2dmesh(
  void * send_buff, int send_count, MPI_Datatype send_type,
  void * recv_buff, int recv_count, MPI_Datatype recv_type,
  MPI_Comm comm);

#endif
