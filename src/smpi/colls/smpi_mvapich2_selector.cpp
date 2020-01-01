/* selector for collective algorithms based on mvapich decision logic */

/* Copyright (c) 2009-2020. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "colls_private.hpp"

#include "smpi_mvapich2_selector_stampede.hpp"

namespace simgrid {
namespace smpi {


int alltoall__mvapich2( const void *sendbuf, int sendcount,
                        MPI_Datatype sendtype,
                        void* recvbuf, int recvcount,
                        MPI_Datatype recvtype,
                        MPI_Comm comm)
{

  if(mv2_alltoall_table_ppn_conf==NULL)
    init_mv2_alltoall_tables_stampede();

  int sendtype_size, recvtype_size, comm_size;
  int mpi_errno=MPI_SUCCESS;
  int range = 0;
  int range_threshold = 0;
  int conf_index = 0;
  comm_size =  comm->size();

  sendtype_size=sendtype->size();
  recvtype_size=recvtype->size();
  long nbytes = sendtype_size * sendcount;

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
        unsigned char* tmp_buf = smpi_get_tmp_sendbuffer(comm_size * recvcount * recvtype_size);
        Datatype::copy(recvbuf, comm_size * recvcount, recvtype, tmp_buf, comm_size * recvcount, recvtype);

        mpi_errno = MV2_Alltoall_function(tmp_buf, recvcount, recvtype, recvbuf, recvcount, recvtype, comm);
        smpi_free_tmp_buffer(tmp_buf);
      } else {
          mpi_errno = MPIR_Alltoall_inplace_MV2(sendbuf, sendcount, sendtype,
              recvbuf, recvcount, recvtype,
              comm );
      }
  }


  return (mpi_errno);
}

int allgather__mvapich2(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
    void *recvbuf, int recvcount, MPI_Datatype recvtype,
    MPI_Comm comm)
{

  int mpi_errno = MPI_SUCCESS;
  long nbytes = 0, comm_size, recvtype_size;
  int range = 0;
  bool partial_sub_ok = false;
  int conf_index = 0;
  int range_threshold = 0;
  MPI_Comm shmem_comm;
  //MPI_Comm *shmem_commptr=NULL;
  /* Get the size of the communicator */
  comm_size = comm->size();
  recvtype_size=recvtype->size();
  nbytes = recvtype_size * recvcount;

  if(mv2_allgather_table_ppn_conf==NULL)
    init_mv2_allgather_tables_stampede();

  if(comm->get_leaders_comm()==MPI_COMM_NULL){
    comm->init_smp();
  }

  if (comm->is_uniform()){
    shmem_comm = comm->get_intra_comm();
    int local_size = shmem_comm->size();
    int i          = 0;
    if (mv2_allgather_table_ppn_conf[0] == -1) {
      // Indicating user defined tuning
      conf_index = 0;
      goto conf_check_end;
    }
    do {
      if (local_size == mv2_allgather_table_ppn_conf[i]) {
        conf_index = i;
        partial_sub_ok = true;
        break;
      }
      i++;
    } while(i < mv2_allgather_num_ppn_conf);
  }
  conf_check_end:
  if (not partial_sub_ok) {
    conf_index = 0;
  }

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
  MV2_Allgatherction =
      mv2_allgather_thresholds_table[conf_index][range].inter_leader[range_threshold].
      MV2_pt_Allgatherction;

  bool is_two_level = mv2_allgather_thresholds_table[conf_index][range].two_level[range_threshold];

  /* intracommunicator */
  if (is_two_level) {
    if (partial_sub_ok) {
      if (comm->is_blocked()){
      mpi_errno = MPIR_2lvl_Allgather_MV2(sendbuf, sendcount, sendtype,
                            recvbuf, recvcount, recvtype,
                            comm);
      }else{
      mpi_errno = allgather__mpich(sendbuf, sendcount, sendtype,
                            recvbuf, recvcount, recvtype,
                            comm);
      }
    } else {
      mpi_errno = MPIR_Allgather_RD_MV2(sendbuf, sendcount, sendtype,
          recvbuf, recvcount, recvtype,
          comm);
    }
  } else if(MV2_Allgatherction == &MPIR_Allgather_Bruck_MV2
      || MV2_Allgatherction == &MPIR_Allgather_RD_MV2
      || MV2_Allgatherction == &MPIR_Allgather_Ring_MV2) {
      mpi_errno = MV2_Allgatherction(sendbuf, sendcount, sendtype,
          recvbuf, recvcount, recvtype,
          comm);
  }else{
      return MPI_ERR_OTHER;
  }

  return mpi_errno;
}

int gather__mvapich2(const void *sendbuf,
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
  long nbytes = 0;
  int comm_size = comm->size();
  int rank      = comm->rank();

  if (rank == root) {
    int recvtype_size = recvtype->size();
    nbytes            = recvcnt * recvtype_size;
  } else {
    int sendtype_size = sendtype->size();
    nbytes            = sendcnt * sendtype_size;
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

    if (comm->is_blocked() ) {
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

    } else {
  // Indeed, direct (non SMP-aware)gather is MPICH one
  mpi_errno = gather__mpich(sendbuf, sendcnt, sendtype,
      recvbuf, recvcnt, recvtype,
      root, comm);
  }

  return mpi_errno;
}

int allgatherv__mvapich2(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
    void *recvbuf, const int *recvcounts, const int *displs,
    MPI_Datatype recvtype, MPI_Comm  comm )
{
  int mpi_errno = MPI_SUCCESS;
  int range = 0, comm_size, total_count, recvtype_size, i;
  int range_threshold = 0;
  long nbytes = 0;

  if(mv2_allgatherv_thresholds_table==NULL)
    init_mv2_allgatherv_tables_stampede();

  comm_size = comm->size();
  total_count = 0;
  for (i = 0; i < comm_size; i++)
    total_count += recvcounts[i];

  recvtype_size=recvtype->size();
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
    if (not(comm_size & (comm_size - 1))) {
      mpi_errno =
          MPIR_Allgatherv_Rec_Doubling_MV2(sendbuf, sendcount, sendtype, recvbuf, recvcounts, displs, recvtype, comm);
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



int allreduce__mvapich2(const void *sendbuf,
    void *recvbuf,
    int count,
    MPI_Datatype datatype,
    MPI_Op op, MPI_Comm comm)
{

  int mpi_errno = MPI_SUCCESS;
  //int rank = 0,
  int comm_size = 0;

  comm_size = comm->size();
  //rank = comm->rank();

  if (count == 0) {
      return MPI_SUCCESS;
  }

  if (mv2_allreduce_thresholds_table == NULL)
    init_mv2_allreduce_tables_stampede();

  /* check if multiple threads are calling this collective function */

  MPI_Aint sendtype_size = 0;
  long nbytes = 0;
  MPI_Aint true_lb, true_extent;

  sendtype_size=datatype->size();
  nbytes = count * sendtype_size;

  datatype->extent(&true_lb, &true_extent);
  bool is_commutative = op->is_commutative();

  {
    int range = 0, range_threshold = 0, range_threshold_intra = 0;
    bool is_two_level = false;

    /* Search for the corresponding system size inside the tuning table */
    while ((range < (mv2_size_allreduce_tuning_table - 1)) &&
        (comm_size > mv2_allreduce_thresholds_table[range].numproc)) {
        range++;
    }
    /* Search for corresponding inter-leader function */
    /* skip mcast pointers if mcast is not available */
    if (not mv2_allreduce_thresholds_table[range].mcast_enabled) {
        while ((range_threshold < (mv2_allreduce_thresholds_table[range].size_inter_table - 1))
            && ((mv2_allreduce_thresholds_table[range].
                inter_leader[range_threshold].MV2_pt_Allreducection
                == &MPIR_Allreduce_mcst_reduce_redscat_gather_MV2) ||
                (mv2_allreduce_thresholds_table[range].
                    inter_leader[range_threshold].MV2_pt_Allreducection
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
    if (mv2_allreduce_thresholds_table[range].is_two_level_allreduce[range_threshold]) {
      is_two_level = true;
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

    MV2_Allreducection = mv2_allreduce_thresholds_table[range].inter_leader[range_threshold]
                                                                                .MV2_pt_Allreducection;

    MV2_Allreduce_intra_function = mv2_allreduce_thresholds_table[range].intra_node[range_threshold_intra]
                                                                                    .MV2_pt_Allreducection;

    /* check if mcast is ready, otherwise replace mcast with other algorithm */
    if((MV2_Allreducection == &MPIR_Allreduce_mcst_reduce_redscat_gather_MV2)||
        (MV2_Allreducection == &MPIR_Allreduce_mcst_reduce_two_level_helper_MV2)){
        {
          MV2_Allreducection = &MPIR_Allreduce_pt2pt_rd_MV2;
        }
        if (not is_two_level) {
            MV2_Allreducection = &MPIR_Allreduce_pt2pt_rd_MV2;
        }
    }

    if (is_two_level) {
      // check if shm is ready, if not use other algorithm first
      if (is_commutative) {
          if(comm->get_leaders_comm()==MPI_COMM_NULL){
            comm->init_smp();
          }
          mpi_errno = MPIR_Allreduce_two_level_MV2(sendbuf, recvbuf, count,
                                                     datatype, op, comm);
      } else {
        mpi_errno = MPIR_Allreduce_pt2pt_rd_MV2(sendbuf, recvbuf, count,
            datatype, op, comm);
      }
    } else {
        mpi_errno = MV2_Allreducection(sendbuf, recvbuf, count,
            datatype, op, comm);
    }
  }

  //comm->ch.intra_node_done=0;

  return (mpi_errno);


}


int alltoallv__mvapich2(const void *sbuf, const int *scounts, const int *sdisps,
    MPI_Datatype sdtype,
    void *rbuf, const int *rcounts, const int *rdisps,
    MPI_Datatype rdtype,
    MPI_Comm  comm
)
{

  if (sbuf == MPI_IN_PLACE) {
      return alltoallv__ompi_basic_linear(sbuf, scounts, sdisps, sdtype,
                                          rbuf, rcounts, rdisps, rdtype,
                                          comm);
  } else     /* For starters, just keep the original algorithm. */
  return alltoallv__ring(sbuf, scounts, sdisps, sdtype,
                         rbuf, rcounts, rdisps, rdtype,
                         comm);
}


int barrier__mvapich2(MPI_Comm  comm)
{
  return barrier__mvapich2_pair(comm);
}




int bcast__mvapich2(void *buffer,
                    int count,
                    MPI_Datatype datatype,
                    int root, MPI_Comm comm)
{
    int mpi_errno = MPI_SUCCESS;
    int comm_size/*, rank*/;
    bool two_level_bcast      = true;
    long nbytes = 0;
    int range = 0;
    int range_threshold = 0;
    int range_threshold_intra = 0;
    MPI_Aint type_size;
    //, position;
    // unsigned char *tmp_buf = NULL;
    MPI_Comm shmem_comm;
    //MPID_Datatype *dtp;

    if (count == 0)
        return MPI_SUCCESS;
    if(comm->get_leaders_comm()==MPI_COMM_NULL){
      comm->init_smp();
    }
    if (not mv2_bcast_thresholds_table)
      init_mv2_bcast_tables_stampede();
    comm_size = comm->size();
    //rank = comm->rank();

    // bool is_contig = true;
/*    if (HANDLE_GET_KIND(datatype) == HANDLE_KIND_BUILTIN)*/
/*        is_contig = true;*/
/*    else {*/
/*        MPID_Datatype_get_ptr(datatype, dtp);*/
/*        is_contig = dtp->is_contig;*/
/*    }*/

    // bool is_homogeneous = true;

    /* MPI_Type_size() might not give the accurate size of the packed
     * datatype for heterogeneous systems (because of padding, encoding,
     * etc). On the other hand, MPI_Pack_size() can become very
     * expensive, depending on the implementation, especially for
     * heterogeneous systems. We want to use MPI_Type_size() wherever
     * possible, and MPI_Pack_size() in other places.
     */
    //if (is_homogeneous) {
        type_size=datatype->size();

   /* } else {
        MPIR_Pack_size_impl(1, datatype, &type_size);
    }*/
    nbytes =  (count) * (type_size);

    /* Search for the corresponding system size inside the tuning table */
    while ((range < (mv2_size_bcast_tuning_table - 1)) &&
           (comm_size > mv2_bcast_thresholds_table[range].numproc)) {
        range++;
    }
    /* Search for corresponding inter-leader function */
    while ((range_threshold < (mv2_bcast_thresholds_table[range].size_inter_table - 1))
           && (nbytes >
               mv2_bcast_thresholds_table[range].inter_leader[range_threshold].max)
           && (mv2_bcast_thresholds_table[range].inter_leader[range_threshold].max != -1)) {
        range_threshold++;
    }

    /* Search for corresponding intra-node function */
    while ((range_threshold_intra <
            (mv2_bcast_thresholds_table[range].size_intra_table - 1))
           && (nbytes >
               mv2_bcast_thresholds_table[range].intra_node[range_threshold_intra].max)
           && (mv2_bcast_thresholds_table[range].intra_node[range_threshold_intra].max !=
               -1)) {
        range_threshold_intra++;
    }

    MV2_Bcast_function =
        mv2_bcast_thresholds_table[range].inter_leader[range_threshold].
        MV2_pt_Bcast_function;

    MV2_Bcast_intra_node_function =
        mv2_bcast_thresholds_table[range].
        intra_node[range_threshold_intra].MV2_pt_Bcast_function;

/*    if (mv2_user_bcast_intra == NULL && */
/*            MV2_Bcast_intra_node_function == &MPIR_Knomial_Bcast_intra_node_MV2) {*/
/*            MV2_Bcast_intra_node_function = &MPIR_Shmem_Bcast_MV2;*/
/*    }*/

    if (mv2_bcast_thresholds_table[range].inter_leader[range_threshold].
        zcpy_pipelined_knomial_factor != -1) {
        zcpy_knomial_factor =
            mv2_bcast_thresholds_table[range].inter_leader[range_threshold].
            zcpy_pipelined_knomial_factor;
    }

    if (mv2_pipelined_zcpy_knomial_factor != -1) {
        zcpy_knomial_factor = mv2_pipelined_zcpy_knomial_factor;
    }

    if(MV2_Bcast_intra_node_function == NULL) {
        /* if tuning table do not have any intra selection, set func pointer to
        ** default one for mcast intra node */
        MV2_Bcast_intra_node_function = &MPIR_Shmem_Bcast_MV2;
    }

    /* Set value of pipeline segment size */
    bcast_segment_size = mv2_bcast_thresholds_table[range].bcast_segment_size;

    /* Set value of inter node knomial factor */
    mv2_inter_node_knomial_factor = mv2_bcast_thresholds_table[range].inter_node_knomial_factor;

    /* Set value of intra node knomial factor */
    mv2_intra_node_knomial_factor = mv2_bcast_thresholds_table[range].intra_node_knomial_factor;

    /* Check if we will use a two level algorithm or not */
    two_level_bcast =
#if defined(_MCST_SUPPORT_)
        mv2_bcast_thresholds_table[range].is_two_level_bcast[range_threshold]
        || comm->ch.is_mcast_ok;
#else
        mv2_bcast_thresholds_table[range].is_two_level_bcast[range_threshold];
#endif
    if (two_level_bcast) {
       // if (not is_contig || not is_homogeneous) {
//   tmp_buf = smpi_get_tmp_sendbuffer(nbytes);

/*            position = 0;*/
/*            if (rank == root) {*/
/*                mpi_errno =*/
/*                    MPIR_Pack_impl(buffer, count, datatype, tmp_buf, nbytes, &position);*/
/*                if (mpi_errno)*/
/*                    MPIU_ERR_POP(mpi_errno);*/
/*            }*/
// }
#ifdef CHANNEL_MRAIL_GEN2
        if ((mv2_enable_zcpy_bcast == 1) &&
              (&MPIR_Pipelined_Bcast_Zcpy_MV2 == MV2_Bcast_function)) {
          // if (not is_contig || not is_homogeneous) {
          //   mpi_errno = MPIR_Pipelined_Bcast_Zcpy_MV2(tmp_buf, nbytes, MPI_BYTE, root, comm);
          // } else {
                mpi_errno = MPIR_Pipelined_Bcast_Zcpy_MV2(buffer, count, datatype,
                                                 root, comm);
          // }
        } else
#endif /* defined(CHANNEL_MRAIL_GEN2) */
        {
            shmem_comm = comm->get_intra_comm();
            // if (not is_contig || not is_homogeneous) {
            //   MPIR_Bcast_tune_inter_node_helper_MV2(tmp_buf, nbytes, MPI_BYTE, root, comm);
            // } else {
              MPIR_Bcast_tune_inter_node_helper_MV2(buffer, count, datatype, root, comm);
            // }

            /* We are now done with the inter-node phase */


                    root = INTRA_NODE_ROOT;

                    // if (not is_contig || not is_homogeneous) {
                    //       mpi_errno = MV2_Bcast_intra_node_function(tmp_buf, nbytes, MPI_BYTE, root, shmem_comm);
                    // } else {
                    mpi_errno = MV2_Bcast_intra_node_function(buffer, count,
                                                              datatype, root, shmem_comm);

                    // }
        }
        /*        if (not is_contig || not is_homogeneous) {*/
        /*            if (rank != root) {*/
        /*                position = 0;*/
        /*                mpi_errno = MPIR_Unpack_impl(tmp_buf, nbytes, &position, buffer,*/
        /*                                             count, datatype);*/
        /*            }*/
        /*        }*/
    } else {
        /* We use Knomial for intra node */
        MV2_Bcast_intra_node_function = &MPIR_Knomial_Bcast_intra_node_MV2;
/*        if (mv2_enable_shmem_bcast == 0) {*/
            /* Fall back to non-tuned version */
/*            MPIR_Bcast_intra_MV2(buffer, count, datatype, root, comm);*/
/*        } else {*/
            mpi_errno = MV2_Bcast_function(buffer, count, datatype, root,
                                           comm);

/*        }*/
    }


    return mpi_errno;

}



int reduce__mvapich2(const void *sendbuf,
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
  int pof2;
  int comm_size = 0;
  long nbytes = 0;
  int sendtype_size;
  bool is_two_level = false;

  comm_size = comm->size();
  sendtype_size=datatype->size();
  nbytes = count * sendtype_size;

  if (count == 0)
    return MPI_SUCCESS;

  bool is_commutative = (op == MPI_OP_NULL || op->is_commutative());

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
  if (mv2_reduce_thresholds_table[range].is_two_level_reduce[range_threshold]) {
    is_two_level = true;
  }
  /* We call Reduce function */
  if (is_two_level) {
    if (is_commutative) {
         if(comm->get_leaders_comm()==MPI_COMM_NULL){
           comm->init_smp();
         }
         mpi_errno = MPIR_Reduce_two_level_helper_MV2(sendbuf, recvbuf, count,
                                           datatype, op, root, comm);
    } else {
      mpi_errno = MPIR_Reduce_binomial_MV2(sendbuf, recvbuf, count,
          datatype, op, root, comm);
    }
    } else if(MV2_Reduce_function == &MPIR_Reduce_inter_knomial_wrapper_MV2 ){
        if (is_commutative)
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


int reduce_scatter__mvapich2(const void *sendbuf, void *recvbuf, const int *recvcnts,
    MPI_Datatype datatype, MPI_Op op,
    MPI_Comm comm)
{
  int mpi_errno = MPI_SUCCESS;
  int i = 0, comm_size = comm->size(), total_count = 0, type_size =
      0, nbytes = 0;
  int* disps          = new int[comm_size];

  if(mv2_red_scat_thresholds_table==NULL)
    init_mv2_reduce_scatter_tables_stampede();

  bool is_commutative = (op == MPI_OP_NULL || op->is_commutative());
  for (i = 0; i < comm_size; i++) {
      disps[i] = total_count;
      total_count += recvcnts[i];
  }

  type_size=datatype->size();
  nbytes = total_count * type_size;

  if (is_commutative) {
    int range           = 0;
    int range_threshold = 0;

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
      bool is_block_regular = true;
      for (i = 0; i < (comm_size - 1); ++i) {
          if (recvcnts[i] != recvcnts[i+1]) {
              is_block_regular = false;
              break;
          }
      }
      int pof2 = 1;
      while (pof2 < comm_size) pof2 <<= 1;
      if (pof2 == comm_size && is_block_regular) {
          /* noncommutative, pof2 size, and block regular */
          MPIR_Reduce_scatter_non_comm_MV2(sendbuf, recvbuf,
              recvcnts, datatype,
              op, comm);
      }
      mpi_errno =  reduce_scatter__mpich_rdb(sendbuf, recvbuf,
                                             recvcnts, datatype,
                                             op, comm);
  }
  delete[] disps;
  return mpi_errno;

}



int scatter__mvapich2(const void *sendbuf,
    int sendcnt,
    MPI_Datatype sendtype,
    void *recvbuf,
    int recvcnt,
    MPI_Datatype recvtype,
    int root, MPI_Comm comm)
{
  int range = 0, range_threshold = 0, range_threshold_intra = 0;
  int mpi_errno = MPI_SUCCESS;
  //   int mpi_errno_ret = MPI_SUCCESS;
  int rank, nbytes, comm_size;
  bool partial_sub_ok = false;
  int conf_index = 0;
     MPI_Comm shmem_comm;
  //    MPID_Comm *shmem_commptr=NULL;
  if(mv2_scatter_thresholds_table==NULL)
    init_mv2_scatter_tables_stampede();

  if(comm->get_leaders_comm()==MPI_COMM_NULL){
    comm->init_smp();
  }

  comm_size = comm->size();

  rank = comm->rank();

  if (rank == root) {
    int sendtype_size = sendtype->size();
    nbytes            = sendcnt * sendtype_size;
  } else {
    int recvtype_size = recvtype->size();
    nbytes            = recvcnt * recvtype_size;
  }

    // check if safe to use partial subscription mode
    if (comm->is_uniform()) {

        shmem_comm = comm->get_intra_comm();
        if (mv2_scatter_table_ppn_conf[0] == -1) {
            // Indicating user defined tuning
            conf_index = 0;
        }else{
          int local_size = shmem_comm->size();
          int i          = 0;
            do {
                if (local_size == mv2_scatter_table_ppn_conf[i]) {
                    conf_index = i;
                    partial_sub_ok = true;
                    break;
                }
                i++;
            } while(i < mv2_scatter_num_ppn_conf);
        }
    }

  if (not partial_sub_ok) {
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
      if(comm->ch.is_mcast_ok == 1
          && mv2_use_mcast_scatter == 1
          && comm->ch.shmem_coll_ok == 1) {
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
       if( comm->is_blocked()) {
             MV2_Scatter_intra_function = mv2_scatter_thresholds_table[conf_index][range].intra_node[range_threshold_intra]
                                .MV2_pt_Scatter_function;

             mpi_errno =
                   MV2_Scatter_function(sendbuf, sendcnt, sendtype,
                                        recvbuf, recvcnt, recvtype, root,
                                        comm);
         } else {
      mpi_errno = MPIR_Scatter_MV2_Binomial(sendbuf, sendcnt, sendtype,
          recvbuf, recvcnt, recvtype, root,
          comm);

      }
  } else {
      mpi_errno = MV2_Scatter_function(sendbuf, sendcnt, sendtype,
          recvbuf, recvcnt, recvtype, root,
          comm);
  }
  return (mpi_errno);
}

}
}

void smpi_coll_cleanup_mvapich2()
{
  if (mv2_alltoall_thresholds_table)
    delete[] mv2_alltoall_thresholds_table[0];
  delete[] mv2_alltoall_thresholds_table;
  delete[] mv2_size_alltoall_tuning_table;
  delete[] mv2_alltoall_table_ppn_conf;

  delete[] mv2_gather_thresholds_table;
  if (mv2_allgather_thresholds_table)
    delete[] mv2_allgather_thresholds_table[0];
  delete[] mv2_size_allgather_tuning_table;
  delete[] mv2_allgather_table_ppn_conf;
  delete[] mv2_allgather_thresholds_table;

  delete[] mv2_allgatherv_thresholds_table;
  delete[] mv2_reduce_thresholds_table;
  delete[] mv2_red_scat_thresholds_table;
  delete[] mv2_allreduce_thresholds_table;
  delete[] mv2_bcast_thresholds_table;
  if (mv2_scatter_thresholds_table)
    delete[] mv2_scatter_thresholds_table[0];
  delete[] mv2_scatter_thresholds_table;
  delete[] mv2_size_scatter_tuning_table;
  delete[] mv2_scatter_table_ppn_conf;
}
