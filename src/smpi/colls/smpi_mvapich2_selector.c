/* selector for collective algorithms based on mvapich decision logic */

/* Copyright (c) 2009-2010, 2013-2014. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "colls_private.h"

#include "smpi_mvapich2_selector_stampede.h"


static void init_mv2_alltoall_tables_stampede(){
int i;
  int agg_table_sum = 0;
mv2_alltoall_tuning_table **table_ptrs = NULL;
   mv2_alltoall_num_ppn_conf = 3;
        mv2_alltoall_thresholds_table
	  = malloc(sizeof(mv2_alltoall_tuning_table *)
			* mv2_alltoall_num_ppn_conf);
        table_ptrs = malloc(sizeof(mv2_alltoall_tuning_table *)
				 * mv2_alltoall_num_ppn_conf);
        mv2_size_alltoall_tuning_table = malloc(sizeof(int) *
						     mv2_alltoall_num_ppn_conf);
        mv2_alltoall_table_ppn_conf =malloc(mv2_alltoall_num_ppn_conf * sizeof(int));
        mv2_alltoall_table_ppn_conf[0] = 1;
        mv2_size_alltoall_tuning_table[0] = 6;
        mv2_alltoall_tuning_table mv2_tmp_alltoall_thresholds_table_1ppn[] = {
	  {2,
	   1, 
	   {{0, -1, &MPIR_Alltoall_pairwise_MV2},
	   },
  
	   {{0, -1, &MPIR_Alltoall_inplace_MV2},
	   },
	  },
  
	  {4,
	   2,
	   {{0, 262144, &MPIR_Alltoall_Scatter_dest_MV2},
	    {262144, -1, &MPIR_Alltoall_pairwise_MV2},
	   },
                
	   {{0, -1, &MPIR_Alltoall_inplace_MV2},
	   },
	  },
  
	  {8,
	   2,
	   {{0, 8, &MPIR_Alltoall_RD_MV2},
	    {8, -1, &MPIR_Alltoall_Scatter_dest_MV2},
	   },
  
	   {{0, -1, &MPIR_Alltoall_inplace_MV2},
	   },
	  },
  
	  {16,
	   3,
	   {{0, 64, &MPIR_Alltoall_RD_MV2},
	    {64, 512, &MPIR_Alltoall_bruck_MV2},
	    {512, -1, &MPIR_Alltoall_Scatter_dest_MV2},
	   },
  
	   {{0,-1, &MPIR_Alltoall_inplace_MV2},
	   },
	  },
  
	  {32,
	   3,
	   {{0, 32, &MPIR_Alltoall_RD_MV2},
	    {32, 2048, &MPIR_Alltoall_bruck_MV2},
	    {2048, -1, &MPIR_Alltoall_Scatter_dest_MV2},
	   },
  
	   {{0, -1, &MPIR_Alltoall_inplace_MV2},
	   },
	  },
  
	  {64,
	   3,
	   {{0, 8, &MPIR_Alltoall_RD_MV2},
	    {8, 1024, &MPIR_Alltoall_bruck_MV2},
	    {1024, -1, &MPIR_Alltoall_Scatter_dest_MV2},
	   },
  
	   {{0, -1, &MPIR_Alltoall_inplace_MV2},
	   },
	  },
        };
        table_ptrs[0] = mv2_tmp_alltoall_thresholds_table_1ppn;
        mv2_alltoall_table_ppn_conf[1] = 2;
        mv2_size_alltoall_tuning_table[1] = 6;
        mv2_alltoall_tuning_table mv2_tmp_alltoall_thresholds_table_2ppn[] = {
	  {4,
	   2,
	   {{0, 32, &MPIR_Alltoall_RD_MV2},
	    {32, -1, &MPIR_Alltoall_Scatter_dest_MV2},
	   },
                
	   {{0, -1, &MPIR_Alltoall_inplace_MV2},
	   },
	  },
  
	  {8,
	   2,
	   {{0, 64, &MPIR_Alltoall_RD_MV2},
	    {64, -1, &MPIR_Alltoall_Scatter_dest_MV2},
	   },
                
	   {{0, -1, &MPIR_Alltoall_inplace_MV2},
	   },
	  },
  
	  {16,
	   3,
	   {{0, 64, &MPIR_Alltoall_RD_MV2},
	    {64, 2048, &MPIR_Alltoall_bruck_MV2},
	    {2048, -1, &MPIR_Alltoall_Scatter_dest_MV2},
	   },
  
	   {{0,-1, &MPIR_Alltoall_inplace_MV2},
	   },
	  },
  
	  {32,
	   3,
	   {{0, 16, &MPIR_Alltoall_RD_MV2},
	    {16, 2048, &MPIR_Alltoall_bruck_MV2},
	    {2048, -1, &MPIR_Alltoall_Scatter_dest_MV2},
	   },
  
	   {{0, -1, &MPIR_Alltoall_inplace_MV2},
	   },
	  },
  
	  {64,
	   3,
	   {{0, 8, &MPIR_Alltoall_RD_MV2},
	    {8, 1024, &MPIR_Alltoall_bruck_MV2},
	    {1024, -1, &MPIR_Alltoall_Scatter_dest_MV2},
	   },
  
	   {{0, -1, &MPIR_Alltoall_inplace_MV2},
	   },
	  },

	  {128,
	   3,
	   {{0, 4, &MPIR_Alltoall_RD_MV2},
	    {4, 2048, &MPIR_Alltoall_bruck_MV2},
	    {2048, -1, &MPIR_Alltoall_Scatter_dest_MV2},
	   },
  
	   {{0, -1, &MPIR_Alltoall_inplace_MV2},
	   },
	  },
        };
        table_ptrs[1] = mv2_tmp_alltoall_thresholds_table_2ppn;
        mv2_alltoall_table_ppn_conf[2] = 16;
        mv2_size_alltoall_tuning_table[2] = 7;
        mv2_alltoall_tuning_table mv2_tmp_alltoall_thresholds_table_16ppn[] = {
	  {16,
	   2, 
	   {{0, 2048, &MPIR_Alltoall_bruck_MV2},
	    {2048, -1,  &MPIR_Alltoall_Scatter_dest_MV2},
	   },
  
	   {{32768, -1, &MPIR_Alltoall_inplace_MV2},
	   },
	  },
  
	  {32,
	   2,
	   {{0, 2048, &MPIR_Alltoall_bruck_MV2},
	    {2048, -1, &MPIR_Alltoall_Scatter_dest_MV2},
	   },
                
	   {{16384, -1, &MPIR_Alltoall_inplace_MV2},
	   },
	  },
  
	  {64,
	   3,
	   {{0, 2048, &MPIR_Alltoall_bruck_MV2},
	    {2048, 16384, &MPIR_Alltoall_Scatter_dest_MV2},
	    {16384, -1, &MPIR_Alltoall_pairwise_MV2},
	   },
  
	   {{32768, 131072, &MPIR_Alltoall_inplace_MV2},
	   },
	  },
  
	  {128,
	   2,
	   {{0, 2048, &MPIR_Alltoall_bruck_MV2},
	    {2048, -1, &MPIR_Alltoall_pairwise_MV2},
	   },
  
	   {{16384,65536, &MPIR_Alltoall_inplace_MV2},
	   },
	  },
  
	  {256,
	   2,
	   {{0, 1024, &MPIR_Alltoall_bruck_MV2},
	    {1024, -1, &MPIR_Alltoall_pairwise_MV2},
	   },
  
	   {{16384, 65536, &MPIR_Alltoall_inplace_MV2},
	   },
	  },
  
	  {512,
	   2,
	   {{0, 1024, &MPIR_Alltoall_bruck_MV2},
	    {1024, -1, &MPIR_Alltoall_pairwise_MV2},
	   },
  
	   {{16384, 65536, &MPIR_Alltoall_inplace_MV2},
	   },
	  },
	  {1024,
	   2,
	   {{0, 1024, &MPIR_Alltoall_bruck_MV2},
	    {1024, -1, &MPIR_Alltoall_pairwise_MV2},
	   },
  
	   {{16384, 65536, &MPIR_Alltoall_inplace_MV2},
	   },
	  },
  
        };
        table_ptrs[2] = mv2_tmp_alltoall_thresholds_table_16ppn;
        agg_table_sum = 0;
        for (i = 0; i < mv2_alltoall_num_ppn_conf; i++) {
	  agg_table_sum += mv2_size_alltoall_tuning_table[i];
        }
        mv2_alltoall_thresholds_table[0] =
	  malloc(agg_table_sum * sizeof (mv2_alltoall_tuning_table));
        memcpy(mv2_alltoall_thresholds_table[0], table_ptrs[0],
                    (sizeof(mv2_alltoall_tuning_table)
                     * mv2_size_alltoall_tuning_table[0]));
        for (i = 1; i < mv2_alltoall_num_ppn_conf; i++) {
	  mv2_alltoall_thresholds_table[i] =
            mv2_alltoall_thresholds_table[i - 1]
            + mv2_size_alltoall_tuning_table[i - 1];
	  memcpy(mv2_alltoall_thresholds_table[i], table_ptrs[i],
                      (sizeof(mv2_alltoall_tuning_table)
                       * mv2_size_alltoall_tuning_table[i]));
        }
        free(table_ptrs);
        
        
}
                            
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
            tmp_buf = (char *)malloc( comm_size * recvcount * recvtype_size );
            mpi_errno = smpi_datatype_copy((char *)recvbuf,
                                       comm_size*recvcount, recvtype,
                                       (char *)tmp_buf,
                                       comm_size*recvcount, recvtype);

            mpi_errno = MV2_Alltoall_function(tmp_buf, recvcount, recvtype,
                                               recvbuf, recvcount, recvtype,
                                                comm );        
            free(tmp_buf);
        } else { 
            mpi_errno = MPIR_Alltoall_inplace_MV2(sendbuf, sendcount, sendtype,
                                              recvbuf, recvcount, recvtype,
                                               comm );
        } 
    }

    
    return (mpi_errno);
}


static void init_mv2_allgather_tables_stampede(){
int i;
  int agg_table_sum = 0;
mv2_allgather_tuning_table **table_ptrs = NULL;
 mv2_allgather_num_ppn_conf = 3;
        mv2_allgather_thresholds_table
            = malloc(sizeof(mv2_allgather_tuning_table *)
                  * mv2_allgather_num_ppn_conf);
        table_ptrs = malloc(sizeof(mv2_allgather_tuning_table *)
                                 * mv2_allgather_num_ppn_conf);
        mv2_size_allgather_tuning_table = malloc(sizeof(int) *
                                                      mv2_allgather_num_ppn_conf);
        mv2_allgather_table_ppn_conf 
            = malloc(mv2_allgather_num_ppn_conf * sizeof(int));
        mv2_allgather_table_ppn_conf[0] = 1;
        mv2_size_allgather_tuning_table[0] = 6;
        mv2_allgather_tuning_table mv2_tmp_allgather_thresholds_table_1ppn[] = {
            {
                2,
                {0},
                1,
                {
                    {0, -1, &MPIR_Allgather_Ring_MV2},
                },
            },
            {
                4,
                {0,0},
                2,
                {
                    {0, 262144, &MPIR_Allgather_RD_MV2},
                    {262144, -1, &MPIR_Allgather_Ring_MV2},
                },
            },
            {
                8,
                {0,0},
                2,
                {
                    {0, 131072, &MPIR_Allgather_RD_MV2},
                    {131072, -1, &MPIR_Allgather_Ring_MV2},
                },
            },
            {
                16,
                {0,0},
                2,
                {
                    {0, 131072, &MPIR_Allgather_RD_MV2},
                    {131072, -1, &MPIR_Allgather_Ring_MV2},
                },
            },
            {
                32,
                {0,0},
                2,
                {
                    {0, 65536, &MPIR_Allgather_RD_MV2},
                    {65536, -1, &MPIR_Allgather_Ring_MV2},
                },
            },
            {
                64,
                {0,0},
                2,
                {
                    {0, 32768, &MPIR_Allgather_RD_MV2},
                    {32768, -1, &MPIR_Allgather_Ring_MV2},
                },
            },
        };
        table_ptrs[0] = mv2_tmp_allgather_thresholds_table_1ppn;
        mv2_allgather_table_ppn_conf[1] = 2;
        mv2_size_allgather_tuning_table[1] = 6;
        mv2_allgather_tuning_table mv2_tmp_allgather_thresholds_table_2ppn[] = {
            {
                4,
                {0,0},
                2,
                {
                    {0, 524288, &MPIR_Allgather_RD_MV2},
                    {524288, -1, &MPIR_Allgather_Ring_MV2},
                },
            },
            {
                8,
                {0,1,0},
                2,
                {
                    {0, 32768, &MPIR_Allgather_RD_MV2},
                    {32768, 524288, &MPIR_Allgather_Ring_MV2},
                    {524288, -1, &MPIR_Allgather_Ring_MV2},
                },
            },
            {
                16,
                {0,1,0},
                2,
                {
                    {0, 16384, &MPIR_Allgather_RD_MV2},
                    {16384, 524288, &MPIR_Allgather_Ring_MV2},
                    {524288, -1, &MPIR_Allgather_Ring_MV2},
                },
            },
            {
                32,
                {1,1,0},
                2,
                {
                    {0, 65536, &MPIR_Allgather_RD_MV2},
                    {65536, 524288, &MPIR_Allgather_Ring_MV2},
                    {524288, -1, &MPIR_Allgather_Ring_MV2},
                },
            },
            {
                64,
                {1,1,0},
                2,
                {
                    {0, 32768, &MPIR_Allgather_RD_MV2},
                    {32768, 524288, &MPIR_Allgather_Ring_MV2},
                    {524288, -1, &MPIR_Allgather_Ring_MV2},
                },
            },
            {
                128,
                {1,1,0},
                2,
                {
                    {0, 65536, &MPIR_Allgather_RD_MV2},
                    {65536, 524288, &MPIR_Allgather_Ring_MV2},
                    {524288, -1, &MPIR_Allgather_Ring_MV2},
                },
            },
        };
        table_ptrs[1] = mv2_tmp_allgather_thresholds_table_2ppn;
        mv2_allgather_table_ppn_conf[2] = 16;
        mv2_size_allgather_tuning_table[2] = 6;
        mv2_allgather_tuning_table mv2_tmp_allgather_thresholds_table_16ppn[] = {
            {
                16,
                {0,0},
                2,
                {
                    {0, 1024, &MPIR_Allgather_RD_MV2},
                    {1024, -1, &MPIR_Allgather_Ring_MV2},
                },
            },
            {
                32,
                {0,0},
                2,
                {
                    {0, 1024, &MPIR_Allgather_RD_Allgather_Comm_MV2},
                    {1024, -1, &MPIR_Allgather_Ring_MV2},
                },
            },
            {
                64,
                {0,0},
                2,
                {
                    {0, 1024, &MPIR_Allgather_RD_Allgather_Comm_MV2},
                    {1024, -1, &MPIR_Allgather_Ring_MV2},
                },
            },
            {
                128,
                {0,0},
                2,
                {
                    {0, 1024, &MPIR_Allgather_RD_Allgather_Comm_MV2},
                    {1024, -1, &MPIR_Allgather_Ring_MV2},
                },
            },
            {
                256,
                {0,0},
                2,
                {
                    {0, 1024, &MPIR_Allgather_RD_Allgather_Comm_MV2},
                    {1024, -1, &MPIR_Allgather_Ring_MV2},
                },
            },
            {
                512,
                {0,0},
                2,
                {
                    {0, 1024, &MPIR_Allgather_RD_Allgather_Comm_MV2},
                    {1024, -1, &MPIR_Allgather_Ring_MV2},
                },
            },

        };
        table_ptrs[2] = mv2_tmp_allgather_thresholds_table_16ppn;
        agg_table_sum = 0;
        for (i = 0; i < mv2_allgather_num_ppn_conf; i++) {
            agg_table_sum += mv2_size_allgather_tuning_table[i];
        }
        mv2_allgather_thresholds_table[0] =
            malloc(agg_table_sum * sizeof (mv2_allgather_tuning_table));
        memcpy(mv2_allgather_thresholds_table[0], table_ptrs[0],
            (sizeof(mv2_allgather_tuning_table)
                     * mv2_size_allgather_tuning_table[0]));
        for (i = 1; i < mv2_allgather_num_ppn_conf; i++) {
            mv2_allgather_thresholds_table[i] =
            mv2_allgather_thresholds_table[i - 1]
            + mv2_size_allgather_tuning_table[i - 1];
            memcpy(mv2_allgather_thresholds_table[i], table_ptrs[i],
                      (sizeof(mv2_allgather_tuning_table)
                       * mv2_size_allgather_tuning_table[i]));
        }
        free(table_ptrs);
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

static void init_mv2_gather_tables_stampede(){

 mv2_size_gather_tuning_table=7;
      mv2_gather_thresholds_table = malloc(mv2_size_gather_tuning_table*
						sizeof (mv2_gather_tuning_table)); 
      mv2_gather_tuning_table mv2_tmp_gather_thresholds_table[]={
	{16,
	 2,{{0, 524288, &MPIR_Gather_MV2_Direct},
	    {524288, -1, &MPIR_Gather_intra}},
	 1,{{0, -1, &MPIR_Gather_MV2_Direct}}},
	{32,
	 3,{{0, 16384, &MPIR_Gather_MV2_Direct}, 
	    {16384, 131072, &MPIR_Gather_intra},
	    {131072, -1, &MPIR_Gather_MV2_two_level_Direct}},
	 1,{{0, -1, &MPIR_Gather_intra}}},
	{64,
	 3,{{0, 256, &MPIR_Gather_MV2_two_level_Direct}, 
	    {256, 16384, &MPIR_Gather_MV2_Direct},
	    {256, -1, &MPIR_Gather_MV2_two_level_Direct}},
	 1,{{0, -1, &MPIR_Gather_intra}}},
	{128,
	 3,{{0, 512, &MPIR_Gather_MV2_two_level_Direct}, 
	    {512, 16384, &MPIR_Gather_MV2_Direct},
	    {16384, -1, &MPIR_Gather_MV2_two_level_Direct}},
	 1,{{0, -1, &MPIR_Gather_intra}}},
	{256,
	 3,{{0, 512, &MPIR_Gather_MV2_two_level_Direct}, 
	    {512, 16384, &MPIR_Gather_MV2_Direct},
	    {16384, -1, &MPIR_Gather_MV2_two_level_Direct}},
	 1,{{0, -1, &MPIR_Gather_intra}}},
	{512,
	 3,{{0, 512, &MPIR_Gather_MV2_two_level_Direct}, 
	    {512, 16384, &MPIR_Gather_MV2_Direct},
	    {8196, -1, &MPIR_Gather_MV2_two_level_Direct}},
	 1,{{0, -1, &MPIR_Gather_intra}}},
	{1024,
	 3,{{0, 512, &MPIR_Gather_MV2_two_level_Direct}, 
	    {512, 16384, &MPIR_Gather_MV2_Direct},
	    {8196, -1, &MPIR_Gather_MV2_two_level_Direct}},
	 1,{{0, -1, &MPIR_Gather_intra}}},
      };

      memcpy(mv2_gather_thresholds_table, mv2_tmp_gather_thresholds_table,
		  mv2_size_gather_tuning_table * sizeof (mv2_gather_tuning_table));

}


int smpi_coll_tuned_gather_mvapich2(void *sendbuf,
                    int sendcnt,
                    MPI_Datatype sendtype,
                    void *recvbuf,
                    int recvcnt,
                    MPI_Datatype recvtype,
                    int root, MPI_Comm  comm)
{
    if(mv2_alltoall_table_ppn_conf==NULL)
        init_mv2_alltoall_tables_stampede();
        
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

