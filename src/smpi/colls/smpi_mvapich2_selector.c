/* selector for collective algorithms based on mvapich decision logic */

/* Copyright (c) 2009-2010, 2013-2014. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is xbt_free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "colls_private.h"

#include "smpi_mvapich2_selector_stampede.h"



int smpi_coll_tuned_alltoall_mvapich2( void *sendbuf, int sendcount, 
    MPI_Datatype sendtype,
    void* recvbuf, int recvcount,
    MPI_Datatype recvtype,
    MPI_Comm comm)
{

  if(mv2_alltoall_table_ppn_conf==NULL)
    init_mv2_alltoall_tables_stampede();

  int sendtype_size, recvtype_size, nbytes, comm_size;
  char * tmp_buf = NULL;
  int mpi_errno=MPI_SUCCESS;
  int range = 0;
  int range_threshold = 0;
  int conf_index = 0;
  comm_size =  smpi_comm_size(comm);

  sendtype_size=smpi_datatype_size(sendtype);
  recvtype_size=smpi_datatype_size(recvtype);
  nbytes = sendtype_size * sendcount;

  /* check if safe to use partial subscription mode */

  /* Search for the corresponding system size inside the tuning table */
  while ((range < (mv2_size_alltoall_tuning_table[conf_index] - 1)) &&
      (comm_size > mv2_alltoall_thresholds_table[conf_index][range].numproc)) {
      range++;
  }
  /* Search for corresponding inter-leader function */
  while ((range_threshold < (mv2_alltoall_thresholds_table[conf_index][range].size_table - 1))
      && (nbytes >
  mv2_alltoall_thresholds_table[conf_index][range].algo_table[range_threshold].max)
  && (mv2_alltoall_thresholds_table[conf_index][range].algo_table[range_threshold].max != -1)) {
      range_threshold++;
  }
  MV2_Alltoall_function = mv2_alltoall_thresholds_table[conf_index][range].algo_table[range_threshold]
                                                                                      .MV2_pt_Alltoall_function;

  if(sendbuf != MPI_IN_PLACE) {
      mpi_errno = MV2_Alltoall_function(sendbuf, sendcount, sendtype,
          recvbuf, recvcount, recvtype,
          comm);
  } else {
      range_threshold = 0;
      if(nbytes <
          mv2_alltoall_thresholds_table[conf_index][range].in_place_algo_table[range_threshold].min
          ||nbytes > mv2_alltoall_thresholds_table[conf_index][range].in_place_algo_table[range_threshold].max
      ) {
          tmp_buf = (char *)xbt_malloc( comm_size * recvcount * recvtype_size );
          mpi_errno = smpi_datatype_copy((char *)recvbuf,
              comm_size*recvcount, recvtype,
              (char *)tmp_buf,
              comm_size*recvcount, recvtype);

          mpi_errno = MV2_Alltoall_function(tmp_buf, recvcount, recvtype,
              recvbuf, recvcount, recvtype,
              comm );
          xbt_free(tmp_buf);
      } else {
          mpi_errno = MPIR_Alltoall_inplace_MV2(sendbuf, sendcount, sendtype,
              recvbuf, recvcount, recvtype,
              comm );
      }
  }


  return (mpi_errno);
}



int smpi_coll_tuned_allgather_mvapich2(void *sendbuf, int sendcount, MPI_Datatype sendtype,
    void *recvbuf, int recvcount, MPI_Datatype recvtype,
    MPI_Comm comm)
{

  int mpi_errno = MPI_SUCCESS;
  int nbytes = 0, comm_size, recvtype_size;
  int range = 0;
  //int partial_sub_ok = 0;
  int conf_index = 0;
  int range_threshold = 0;
  int is_two_level = 0;
  //int local_size = -1;
  //MPI_Comm shmem_comm;
  //MPI_Comm *shmem_commptr=NULL;
  /* Get the size of the communicator */
  comm_size = smpi_comm_size(comm);
  recvtype_size=smpi_datatype_size(recvtype);
  nbytes = recvtype_size * recvcount;

  if(mv2_allgather_table_ppn_conf==NULL)
    init_mv2_allgather_tables_stampede();

  //int i;
  /* check if safe to use partial subscription mode */
  /*  if (comm->ch.shmem_coll_ok == 1 && comm->ch.is_uniform) {

        shmem_comm = comm->ch.shmem_comm;
        MPID_Comm_get_ptr(shmem_comm, shmem_commptr);
        local_size = shmem_commptr->local_size;
        i = 0;
        if (mv2_allgather_table_ppn_conf[0] == -1) {
            // Indicating user defined tuning
            conf_index = 0;
            goto conf_check_end;
        }
        do {
            if (local_size == mv2_allgather_table_ppn_conf[i]) {
                conf_index = i;
                partial_sub_ok = 1;
                break;
            }
            i++;
        } while(i < mv2_allgather_num_ppn_conf);
    }

  conf_check_end:
    if (partial_sub_ok != 1) {
        conf_index = 0;
    }*/
  /* Search for the corresponding system size inside the tuning table */
  while ((range < (mv2_size_allgather_tuning_table[conf_index] - 1)) &&
      (comm_size >
  mv2_allgather_thresholds_table[conf_index][range].numproc)) {
      range++;
  }
  /* Search for corresponding inter-leader function */
  while ((range_threshold <
      (mv2_allgather_thresholds_table[conf_index][range].size_inter_table - 1))
      && (nbytes > mv2_allgather_thresholds_table[conf_index][range].inter_leader[range_threshold].max)
      && (mv2_allgather_thresholds_table[conf_index][range].inter_leader[range_threshold].max !=
          -1)) {
      range_threshold++;
  }

  /* Set inter-leader pt */
  MV2_Allgather_function =
      mv2_allgather_thresholds_table[conf_index][range].inter_leader[range_threshold].
      MV2_pt_Allgather_function;

  is_two_level =  mv2_allgather_thresholds_table[conf_index][range].two_level[range_threshold];

  /* intracommunicator */
  if(is_two_level ==1){

      /*       if(comm->ch.shmem_coll_ok == 1){
            MPIR_T_PVAR_COUNTER_INC(MV2, mv2_num_shmem_coll_calls, 1);
	   if (1 == comm->ch.is_blocked) {
                mpi_errno = MPIR_2lvl_Allgather_MV2(sendbuf, sendcount, sendtype,
						    recvbuf, recvcount, recvtype,
						    comm, errflag);
	   }
	   else {
	       mpi_errno = MPIR_Allgather_intra(sendbuf, sendcount, sendtype,
						recvbuf, recvcount, recvtype,
						comm, errflag);
	   }
        } else {*/
      mpi_errno = MPIR_Allgather_RD_MV2(sendbuf, sendcount, sendtype,
          recvbuf, recvcount, recvtype,
          comm);
      //     }
  } else if(MV2_Allgather_function == &MPIR_Allgather_Bruck_MV2
      || MV2_Allgather_function == &MPIR_Allgather_RD_MV2
      || MV2_Allgather_function == &MPIR_Allgather_Ring_MV2) {
      mpi_errno = MV2_Allgather_function(sendbuf, sendcount, sendtype,
          recvbuf, recvcount, recvtype,
          comm);
  }else{
      return MPI_ERR_OTHER;
  }

  return mpi_errno;
}


int smpi_coll_tuned_gather_mvapich2(void *sendbuf,
    int sendcnt,
    MPI_Datatype sendtype,
    void *recvbuf,
    int recvcnt,
    MPI_Datatype recvtype,
    int root, MPI_Comm  comm)
{
  if(mv2_gather_thresholds_table==NULL)
    init_mv2_gather_tables_stampede();

  int mpi_errno = MPI_SUCCESS;
  int range = 0;
  int range_threshold = 0;
  int range_intra_threshold = 0;
  int nbytes = 0;
  int comm_size = 0;
  int recvtype_size, sendtype_size;
  int rank = -1;
  comm_size = smpi_comm_size(comm);
  rank = smpi_comm_rank(comm);

  if (rank == root) {
      recvtype_size=smpi_datatype_size(recvtype);
      nbytes = recvcnt * recvtype_size;
  } else {
      sendtype_size=smpi_datatype_size(sendtype);
      nbytes = sendcnt * sendtype_size;
  }

  /* Search for the corresponding system size inside the tuning table */
  while ((range < (mv2_size_gather_tuning_table - 1)) &&
      (comm_size > mv2_gather_thresholds_table[range].numproc)) {
      range++;
  }
  /* Search for corresponding inter-leader function */
  while ((range_threshold < (mv2_gather_thresholds_table[range].size_inter_table - 1))
      && (nbytes >
  mv2_gather_thresholds_table[range].inter_leader[range_threshold].max)
  && (mv2_gather_thresholds_table[range].inter_leader[range_threshold].max !=
      -1)) {
      range_threshold++;
  }

  /* Search for corresponding intra node function */
  while ((range_intra_threshold < (mv2_gather_thresholds_table[range].size_intra_table - 1))
      && (nbytes >
  mv2_gather_thresholds_table[range].intra_node[range_intra_threshold].max)
  && (mv2_gather_thresholds_table[range].intra_node[range_intra_threshold].max !=
      -1)) {
      range_intra_threshold++;
  }
  /*
    if (comm->ch.is_global_block == 1 && mv2_use_direct_gather == 1 &&
            mv2_use_two_level_gather == 1 && comm->ch.shmem_coll_ok == 1) {
        // Set intra-node function pt for gather_two_level 
        MV2_Gather_intra_node_function = 
                              mv2_gather_thresholds_table[range].intra_node[range_intra_threshold].
                              MV2_pt_Gather_function;
        //Set inter-leader pt 
        MV2_Gather_inter_leader_function =
                              mv2_gather_thresholds_table[range].inter_leader[range_threshold].
                              MV2_pt_Gather_function;
        // We call Gather function 
        mpi_errno =
            MV2_Gather_inter_leader_function(sendbuf, sendcnt, sendtype, recvbuf, recvcnt,
                                             recvtype, root, comm);

    } else {*/
  // Indded, direct (non SMP-aware)gather is MPICH one
  mpi_errno = smpi_coll_tuned_gather_mpich(sendbuf, sendcnt, sendtype,
      recvbuf, recvcnt, recvtype,
      root, comm);
  //}

  return mpi_errno;
}


int smpi_coll_tuned_allgatherv_mvapich2(void *sendbuf, int sendcount, MPI_Datatype sendtype,
    void *recvbuf, int *recvcounts, int *displs,
    MPI_Datatype recvtype, MPI_Comm  comm )
{
  int mpi_errno = MPI_SUCCESS;
  int range = 0, comm_size, total_count, recvtype_size, i;
  int range_threshold = 0;
  int nbytes = 0;

  if(mv2_allgatherv_thresholds_table==NULL)
    init_mv2_allgatherv_tables_stampede();

  comm_size = smpi_comm_size(comm);
  total_count = 0;
  for (i = 0; i < comm_size; i++)
    total_count += recvcounts[i];

  recvtype_size=smpi_datatype_size(recvtype);
  nbytes = total_count * recvtype_size;

  /* Search for the corresponding system size inside the tuning table */
  while ((range < (mv2_size_allgatherv_tuning_table - 1)) &&
      (comm_size > mv2_allgatherv_thresholds_table[range].numproc)) {
      range++;
  }
  /* Search for corresponding inter-leader function */
  while ((range_threshold < (mv2_allgatherv_thresholds_table[range].size_inter_table - 1))
      && (nbytes >
  comm_size * mv2_allgatherv_thresholds_table[range].inter_leader[range_threshold].max)
  && (mv2_allgatherv_thresholds_table[range].inter_leader[range_threshold].max !=
      -1)) {
      range_threshold++;
  }
  /* Set inter-leader pt */
  MV2_Allgatherv_function =
      mv2_allgatherv_thresholds_table[range].inter_leader[range_threshold].
      MV2_pt_Allgatherv_function;

  if (MV2_Allgatherv_function == &MPIR_Allgatherv_Rec_Doubling_MV2)
    {
      if(!(comm_size & (comm_size - 1)))
        {
          mpi_errno =
              MPIR_Allgatherv_Rec_Doubling_MV2(sendbuf, sendcount,
                  sendtype, recvbuf,
                  recvcounts, displs,
                  recvtype, comm);
        } else {
            mpi_errno =
                MPIR_Allgatherv_Bruck_MV2(sendbuf, sendcount,
                    sendtype, recvbuf,
                    recvcounts, displs,
                    recvtype, comm);
        }
    } else {
        mpi_errno =
            MV2_Allgatherv_function(sendbuf, sendcount, sendtype,
                recvbuf, recvcounts, displs,
                recvtype, comm);
    }

  return mpi_errno;
}



int smpi_coll_tuned_allreduce_mvapich2(void *sendbuf,
    void *recvbuf,
    int count,
    MPI_Datatype datatype,
    MPI_Op op, MPI_Comm comm)
{

  int mpi_errno = MPI_SUCCESS;
  //int rank = 0,
  int comm_size = 0;

  comm_size = smpi_comm_size(comm);
  //rank = smpi_comm_rank(comm);

  if (count == 0) {
      return MPI_SUCCESS;
  }

  if (mv2_allreduce_thresholds_table == NULL)
    init_mv2_allreduce_tables_stampede();

  /* check if multiple threads are calling this collective function */

  MPI_Aint sendtype_size = 0;
  int nbytes = 0;
  int range = 0, range_threshold = 0, range_threshold_intra = 0;
  int is_two_level = 0;
  //int is_commutative = 0;
  MPI_Aint true_lb, true_extent;

  sendtype_size=smpi_datatype_size(datatype);
  nbytes = count * sendtype_size;

  smpi_datatype_extent(datatype, &true_lb, &true_extent);
  //MPI_Op *op_ptr;
  //is_commutative = smpi_op_is_commute(op);

  {
    /* Search for the corresponding system size inside the tuning table */
    while ((range < (mv2_size_allreduce_tuning_table - 1)) &&
        (comm_size > mv2_allreduce_thresholds_table[range].numproc)) {
        range++;
    }
    /* Search for corresponding inter-leader function */
    /* skip mcast poiters if mcast is not available */
    if(mv2_allreduce_thresholds_table[range].mcast_enabled != 1){
        while ((range_threshold < (mv2_allreduce_thresholds_table[range].size_inter_table - 1))
            && ((mv2_allreduce_thresholds_table[range].
                inter_leader[range_threshold].MV2_pt_Allreduce_function
                == &MPIR_Allreduce_mcst_reduce_redscat_gather_MV2) ||
                (mv2_allreduce_thresholds_table[range].
                    inter_leader[range_threshold].MV2_pt_Allreduce_function
                    == &MPIR_Allreduce_mcst_reduce_two_level_helper_MV2)
            )) {
            range_threshold++;
        }
    }
    while ((range_threshold < (mv2_allreduce_thresholds_table[range].size_inter_table - 1))
        && (nbytes >
    mv2_allreduce_thresholds_table[range].inter_leader[range_threshold].max)
    && (mv2_allreduce_thresholds_table[range].inter_leader[range_threshold].max != -1)) {
        range_threshold++;
    }
    if(mv2_allreduce_thresholds_table[range].is_two_level_allreduce[range_threshold] == 1){
        is_two_level = 1;
    }
    /* Search for corresponding intra-node function */
    while ((range_threshold_intra <
        (mv2_allreduce_thresholds_table[range].size_intra_table - 1))
        && (nbytes >
    mv2_allreduce_thresholds_table[range].intra_node[range_threshold_intra].max)
    && (mv2_allreduce_thresholds_table[range].intra_node[range_threshold_intra].max !=
        -1)) {
        range_threshold_intra++;
    }

    MV2_Allreduce_function = mv2_allreduce_thresholds_table[range].inter_leader[range_threshold]
                                                                                .MV2_pt_Allreduce_function;

    MV2_Allreduce_intra_function = mv2_allreduce_thresholds_table[range].intra_node[range_threshold_intra]
                                                                                    .MV2_pt_Allreduce_function;

    /* check if mcast is ready, otherwise replace mcast with other algorithm */
    if((MV2_Allreduce_function == &MPIR_Allreduce_mcst_reduce_redscat_gather_MV2)||
        (MV2_Allreduce_function == &MPIR_Allreduce_mcst_reduce_two_level_helper_MV2)){
        {
          MV2_Allreduce_function = &MPIR_Allreduce_pt2pt_rd_MV2;
        }
        if(is_two_level != 1) {
            MV2_Allreduce_function = &MPIR_Allreduce_pt2pt_rd_MV2;
        }
    }

    if(is_two_level == 1){
        // check if shm is ready, if not use other algorithm first
        /*if ((comm->ch.shmem_coll_ok == 1)
                    && (mv2_enable_shmem_allreduce)
                    && (is_commutative)
                    && (mv2_enable_shmem_collectives)) {
                    mpi_errno = MPIR_Allreduce_two_level_MV2(sendbuf, recvbuf, count,
                                                     datatype, op, comm);
                } else {*/
        mpi_errno = MPIR_Allreduce_pt2pt_rd_MV2(sendbuf, recvbuf, count,
            datatype, op, comm);
        // }
    } else {
        mpi_errno = MV2_Allreduce_function(sendbuf, recvbuf, count,
            datatype, op, comm);
    }
  }

  //comm->ch.intra_node_done=0;

  return (mpi_errno);


}


int smpi_coll_tuned_alltoallv_mvapich2(void *sbuf, int *scounts, int *sdisps,
    MPI_Datatype sdtype,
    void *rbuf, int *rcounts, int *rdisps,
    MPI_Datatype rdtype,
    MPI_Comm  comm
)
{

  if (sbuf == MPI_IN_PLACE) {
      return smpi_coll_tuned_alltoallv_ompi_basic_linear(sbuf, scounts, sdisps, sdtype,
          rbuf, rcounts, rdisps,rdtype,
          comm);
  } else     /* For starters, just keep the original algorithm. */
  return smpi_coll_tuned_alltoallv_ring(sbuf, scounts, sdisps, sdtype,
      rbuf, rcounts, rdisps,rdtype,
      comm);
}


int smpi_coll_tuned_barrier_mvapich2(MPI_Comm  comm)
{   
  return smpi_coll_tuned_barrier_mvapich2_pair(comm);
}




int smpi_coll_tuned_bcast_mvapich2(void *buffer,
    int count,
    MPI_Datatype datatype,
    int root, MPI_Comm comm)
{

  //TODO : Bcast really needs intra/inter phases in mvapich. Default to mpich if not available
  return smpi_coll_tuned_bcast_mpich(buffer, count, datatype, root, comm);

}



int smpi_coll_tuned_reduce_mvapich2( void *sendbuf,
    void *recvbuf,
    int count,
    MPI_Datatype datatype,
    MPI_Op op, int root, MPI_Comm comm)
{
  if(mv2_reduce_thresholds_table == NULL)
    init_mv2_reduce_tables_stampede();

  int mpi_errno = MPI_SUCCESS;
  int range = 0;
  int range_threshold = 0;
  int range_intra_threshold = 0;
  int is_commutative, pof2;
  int comm_size = 0;
  int nbytes = 0;
  int sendtype_size;
  int is_two_level = 0;

  comm_size = smpi_comm_size(comm);
  sendtype_size=smpi_datatype_size(datatype);
  nbytes = count * sendtype_size;

  if (count == 0)
    return MPI_SUCCESS;

  is_commutative = smpi_op_is_commute(op);

  /* find nearest power-of-two less than or equal to comm_size */
  for( pof2 = 1; pof2 <= comm_size; pof2 <<= 1 );
  pof2 >>=1;


  /* Search for the corresponding system size inside the tuning table */
  while ((range < (mv2_size_reduce_tuning_table - 1)) &&
      (comm_size > mv2_reduce_thresholds_table[range].numproc)) {
      range++;
  }
  /* Search for corresponding inter-leader function */
  while ((range_threshold < (mv2_reduce_thresholds_table[range].size_inter_table - 1))
      && (nbytes >
  mv2_reduce_thresholds_table[range].inter_leader[range_threshold].max)
  && (mv2_reduce_thresholds_table[range].inter_leader[range_threshold].max !=
      -1)) {
      range_threshold++;
  }

  /* Search for corresponding intra node function */
  while ((range_intra_threshold < (mv2_reduce_thresholds_table[range].size_intra_table - 1))
      && (nbytes >
  mv2_reduce_thresholds_table[range].intra_node[range_intra_threshold].max)
  && (mv2_reduce_thresholds_table[range].intra_node[range_intra_threshold].max !=
      -1)) {
      range_intra_threshold++;
  }

  /* Set intra-node function pt for reduce_two_level */
  MV2_Reduce_intra_function =
      mv2_reduce_thresholds_table[range].intra_node[range_intra_threshold].
      MV2_pt_Reduce_function;
  /* Set inter-leader pt */
  MV2_Reduce_function =
      mv2_reduce_thresholds_table[range].inter_leader[range_threshold].
      MV2_pt_Reduce_function;

  if(mv2_reduce_intra_knomial_factor<0)
    {
      mv2_reduce_intra_knomial_factor = mv2_reduce_thresholds_table[range].intra_k_degree;
    }
  if(mv2_reduce_inter_knomial_factor<0)
    {
      mv2_reduce_inter_knomial_factor = mv2_reduce_thresholds_table[range].inter_k_degree;
    }
  if(mv2_reduce_thresholds_table[range].is_two_level_reduce[range_threshold] == 1){
      is_two_level = 1;
  }
  /* We call Reduce function */
  if(is_two_level == 1)
    {
      /* if (comm->ch.shmem_coll_ok == 1
            && is_commutative == 1) {
            mpi_errno = MPIR_Reduce_two_level_helper_MV2(sendbuf, recvbuf, count, 
                                           datatype, op, root, comm, errflag);
        } else {*/
      mpi_errno = MPIR_Reduce_binomial_MV2(sendbuf, recvbuf, count,
          datatype, op, root, comm);
      //}
    } else if(MV2_Reduce_function == &MPIR_Reduce_inter_knomial_wrapper_MV2 ){
        if(is_commutative ==1)
          {
            mpi_errno = MV2_Reduce_function(sendbuf, recvbuf, count, 
                datatype, op, root, comm);
          } else {
              mpi_errno = MPIR_Reduce_binomial_MV2(sendbuf, recvbuf, count,
                  datatype, op, root, comm);
          }
    } else if(MV2_Reduce_function == &MPIR_Reduce_redscat_gather_MV2){
        if (/*(HANDLE_GET_KIND(op) == HANDLE_KIND_BUILTIN) &&*/ (count >= pof2))
          {
            mpi_errno = MV2_Reduce_function(sendbuf, recvbuf, count, 
                datatype, op, root, comm);
          } else {
              mpi_errno = MPIR_Reduce_binomial_MV2(sendbuf, recvbuf, count,
                  datatype, op, root, comm);
          }
    } else {
        mpi_errno = MV2_Reduce_function(sendbuf, recvbuf, count, 
            datatype, op, root, comm);
    }


  return mpi_errno;

}


int smpi_coll_tuned_reduce_scatter_mvapich2(void *sendbuf, void *recvbuf, int *recvcnts,
    MPI_Datatype datatype, MPI_Op op,
    MPI_Comm comm)
{
  int mpi_errno = MPI_SUCCESS;
  int i = 0, comm_size = smpi_comm_size(comm), total_count = 0, type_size =
      0, nbytes = 0;
  int range = 0;
  int range_threshold = 0;
  int is_commutative = 0;
  int *disps = xbt_malloc(comm_size * sizeof (int));

  if(mv2_red_scat_thresholds_table==NULL)
    init_mv2_reduce_scatter_tables_stampede();

  is_commutative=smpi_op_is_commute(op);
  for (i = 0; i < comm_size; i++) {
      disps[i] = total_count;
      total_count += recvcnts[i];
  }

  type_size=smpi_datatype_size(datatype);
  nbytes = total_count * type_size;

  if (is_commutative) {

      /* Search for the corresponding system size inside the tuning table */
      while ((range < (mv2_size_red_scat_tuning_table - 1)) &&
          (comm_size > mv2_red_scat_thresholds_table[range].numproc)) {
          range++;
      }
      /* Search for corresponding inter-leader function */
      while ((range_threshold < (mv2_red_scat_thresholds_table[range].size_inter_table - 1))
          && (nbytes >
      mv2_red_scat_thresholds_table[range].inter_leader[range_threshold].max)
      && (mv2_red_scat_thresholds_table[range].inter_leader[range_threshold].max !=
          -1)) {
          range_threshold++;
      }

      /* Set inter-leader pt */
      MV2_Red_scat_function =
          mv2_red_scat_thresholds_table[range].inter_leader[range_threshold].
          MV2_pt_Red_scat_function;

      mpi_errno = MV2_Red_scat_function(sendbuf, recvbuf,
          recvcnts, datatype,
          op, comm);
  } else {
      int is_block_regular = 1;
      for (i = 0; i < (comm_size - 1); ++i) {
          if (recvcnts[i] != recvcnts[i+1]) {
              is_block_regular = 0;
              break;
          }
      }
      int pof2 = 1;
      while (pof2 < comm_size) pof2 <<= 1;
      if (pof2 == comm_size && is_block_regular) {
          /* noncommutative, pof2 size, and block regular */
          mpi_errno = MPIR_Reduce_scatter_non_comm_MV2(sendbuf, recvbuf,
              recvcnts, datatype,
              op, comm);
      }
      mpi_errno =  smpi_coll_tuned_reduce_scatter_mpich_rdb(sendbuf, recvbuf,
          recvcnts, datatype,
          op, comm);
  }

  return mpi_errno;

}



int smpi_coll_tuned_scatter_mvapich2(void *sendbuf,
    int sendcnt,
    MPI_Datatype sendtype,
    void *recvbuf,
    int recvcnt,
    MPI_Datatype recvtype,
    int root, MPI_Comm comm_ptr)
{
  int range = 0, range_threshold = 0, range_threshold_intra = 0;
  int mpi_errno = MPI_SUCCESS;
  //   int mpi_errno_ret = MPI_SUCCESS;
  int rank, nbytes, comm_size;
  int recvtype_size, sendtype_size;
  int partial_sub_ok = 0;
  int conf_index = 0;
  //  int local_size = -1;
  //  int i;
  //   MPI_Comm shmem_comm;
  //    MPID_Comm *shmem_commptr=NULL;
  if(mv2_scatter_thresholds_table==NULL)
    init_mv2_scatter_tables_stampede();

  comm_size = smpi_comm_size(comm_ptr);

  rank = smpi_comm_rank(comm_ptr);

  if (rank == root) {
      sendtype_size=smpi_datatype_size(sendtype);
      nbytes = sendcnt * sendtype_size;
  } else {
      recvtype_size=smpi_datatype_size(recvtype);
      nbytes = recvcnt * recvtype_size;
  }
  /*
    // check if safe to use partial subscription mode 
    if (comm_ptr->ch.shmem_coll_ok == 1 && comm_ptr->ch.is_uniform) {

        shmem_comm = comm_ptr->ch.shmem_comm;
        MPID_Comm_get_ptr(shmem_comm, shmem_commptr);
        local_size = shmem_commptr->local_size;
        i = 0;
        if (mv2_scatter_table_ppn_conf[0] == -1) {
            // Indicating user defined tuning 
            conf_index = 0;
            goto conf_check_end;
        }
        do {
            if (local_size == mv2_scatter_table_ppn_conf[i]) {
                conf_index = i;
                partial_sub_ok = 1;
                break;
            }
            i++;
        } while(i < mv2_scatter_num_ppn_conf);
    }
   */
  if (partial_sub_ok != 1) {
      conf_index = 0;
  }

  /* Search for the corresponding system size inside the tuning table */
  while ((range < (mv2_size_scatter_tuning_table[conf_index] - 1)) &&
      (comm_size > mv2_scatter_thresholds_table[conf_index][range].numproc)) {
      range++;
  }
  /* Search for corresponding inter-leader function */
  while ((range_threshold < (mv2_scatter_thresholds_table[conf_index][range].size_inter_table - 1))
      && (nbytes >
  mv2_scatter_thresholds_table[conf_index][range].inter_leader[range_threshold].max)
  && (mv2_scatter_thresholds_table[conf_index][range].inter_leader[range_threshold].max != -1)) {
      range_threshold++;
  }

  /* Search for corresponding intra-node function */
  while ((range_threshold_intra <
      (mv2_scatter_thresholds_table[conf_index][range].size_intra_table - 1))
      && (nbytes >
  mv2_scatter_thresholds_table[conf_index][range].intra_node[range_threshold_intra].max)
  && (mv2_scatter_thresholds_table[conf_index][range].intra_node[range_threshold_intra].max !=
      -1)) {
      range_threshold_intra++;
  }

  MV2_Scatter_function = mv2_scatter_thresholds_table[conf_index][range].inter_leader[range_threshold]
                                                                                      .MV2_pt_Scatter_function;

  if(MV2_Scatter_function == &MPIR_Scatter_mcst_wrap_MV2) {
#if defined(_MCST_SUPPORT_)
      if(comm_ptr->ch.is_mcast_ok == 1
          && mv2_use_mcast_scatter == 1
          && comm_ptr->ch.shmem_coll_ok == 1) {
          MV2_Scatter_function = &MPIR_Scatter_mcst_MV2;
      } else
#endif /*#if defined(_MCST_SUPPORT_) */
        {
          if(mv2_scatter_thresholds_table[conf_index][range].inter_leader[range_threshold + 1].
              MV2_pt_Scatter_function != NULL) {
              MV2_Scatter_function = mv2_scatter_thresholds_table[conf_index][range].inter_leader[range_threshold + 1]
                                                                                                  .MV2_pt_Scatter_function;
          } else {
              /* Fallback! */
              MV2_Scatter_function = &MPIR_Scatter_MV2_Binomial;
          }
        } 
  }

  if( (MV2_Scatter_function == &MPIR_Scatter_MV2_two_level_Direct) ||
      (MV2_Scatter_function == &MPIR_Scatter_MV2_two_level_Binomial)) {
      /* if( comm_ptr->ch.shmem_coll_ok == 1 &&
             comm_ptr->ch.is_global_block == 1 ) {
             MV2_Scatter_intra_function = mv2_scatter_thresholds_table[conf_index][range].intra_node[range_threshold_intra]
                                .MV2_pt_Scatter_function;

             mpi_errno =
                   MV2_Scatter_function(sendbuf, sendcnt, sendtype,
                                        recvbuf, recvcnt, recvtype, root,
                                        comm_ptr);
         } else {*/
      mpi_errno = MPIR_Scatter_MV2_Binomial(sendbuf, sendcnt, sendtype,
          recvbuf, recvcnt, recvtype, root,
          comm_ptr);

      //}
  } else {
      mpi_errno = MV2_Scatter_function(sendbuf, sendcnt, sendtype,
          recvbuf, recvcnt, recvtype, root,
          comm_ptr);
  }
  return (mpi_errno);
}

