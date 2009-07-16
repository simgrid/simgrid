

#include "private.h"
#include "smpi_coll_private.h"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(smpi_mpi, smpi,
                                "Logging specific to SMPI (mpi)");

int SMPI_MPI_Init(int *argc, char ***argv)
{
  smpi_process_init(argc, argv);
  smpi_bench_begin();
  return MPI_SUCCESS;
}

int SMPI_MPI_Finalize()
{
  smpi_bench_end();
  smpi_process_finalize();
  return MPI_SUCCESS;
}

// right now this just exits the current node, should send abort signal to all
// hosts in the communicator (TODO)
int SMPI_MPI_Abort(MPI_Comm comm, int errorcode)
{
  smpi_exit(errorcode);
  return 0;
}

int SMPI_MPI_Comm_size(MPI_Comm comm, int *size)
{
  int retval = MPI_SUCCESS;

  smpi_bench_end();

  if (NULL == comm) {
    retval = MPI_ERR_COMM;
  } else if (NULL == size) {
    retval = MPI_ERR_ARG;
  } else {
    *size = comm->size;
  }

  smpi_bench_begin();

  return retval;
}

int SMPI_MPI_Comm_rank(MPI_Comm comm, int *rank)
{
  int retval = MPI_SUCCESS;

  smpi_bench_end();

  if (NULL == comm) {
    retval = MPI_ERR_COMM;
  } else if (NULL == rank) {
    retval = MPI_ERR_ARG;
  } else {
    *rank = smpi_mpi_comm_rank(comm);
  }

  smpi_bench_begin();

  return retval;
}

int SMPI_MPI_Type_size(MPI_Datatype datatype, size_t * size)
{
  int retval = MPI_SUCCESS;

  smpi_bench_end();

  if (NULL == datatype) {
    retval = MPI_ERR_TYPE;
  } else if (NULL == size) {
    retval = MPI_ERR_ARG;
  } else {
    *size = datatype->size;
  }

  smpi_bench_begin();

  return retval;
}

int SMPI_MPI_Barrier(MPI_Comm comm)
{
  int retval = MPI_SUCCESS;
  int arity=4;

  smpi_bench_end();

  if (NULL == comm) {
    retval = MPI_ERR_COMM;
  } else {

    /*
     * original implemantation:
     * retval = smpi_mpi_barrier(comm);
     * this one is unrealistic: it just cond_waits, means no time.
     */
     retval = nary_tree_barrier( comm, arity );
  }

  smpi_bench_begin();

  return retval;
}

int SMPI_MPI_Irecv(void *buf, int count, MPI_Datatype datatype, int src,
                   int tag, MPI_Comm comm, MPI_Request * request)
{
  int retval = MPI_SUCCESS;

  smpi_bench_end();

  retval = smpi_create_request(buf, count, datatype, src, 0, tag, comm,
                               request);
  if (NULL != *request && MPI_SUCCESS == retval) {
    retval = smpi_mpi_irecv(*request);
  }

  smpi_bench_begin();

  return retval;
}

int SMPI_MPI_Recv(void *buf, int count, MPI_Datatype datatype, int src,
                  int tag, MPI_Comm comm, MPI_Status * status)
{
  int retval = MPI_SUCCESS;
  smpi_mpi_request_t request;

  smpi_bench_end();

  retval = smpi_create_request(buf, count, datatype, src, 0, tag, comm,
                               &request);
  if (NULL != request && MPI_SUCCESS == retval) {
    retval = smpi_mpi_irecv(request);
    if (MPI_SUCCESS == retval) {
      retval = smpi_mpi_wait(request, status);
    }
    xbt_mallocator_release(smpi_global->request_mallocator, request);
  }

  smpi_bench_begin();

  return retval;
}

int SMPI_MPI_Isend(void *buf, int count, MPI_Datatype datatype, int dst,
                   int tag, MPI_Comm comm, MPI_Request * request)
{
  int retval = MPI_SUCCESS;

  smpi_bench_end();

  retval = smpi_create_request(buf, count, datatype, 0, dst, tag, comm,
                               request);
  if (NULL != *request && MPI_SUCCESS == retval) {
    retval = smpi_mpi_isend(*request);
  }

  smpi_bench_begin();

  return retval;
}

int SMPI_MPI_Send(void *buf, int count, MPI_Datatype datatype, int dst,
                  int tag, MPI_Comm comm)
{
  int retval = MPI_SUCCESS;
  smpi_mpi_request_t request;

  smpi_bench_end();

  retval = smpi_create_request(buf, count, datatype, 0, dst, tag, comm,
                               &request);
  if (NULL != request && MPI_SUCCESS == retval) {
    retval = smpi_mpi_isend(request);
    if (MPI_SUCCESS == retval) {
      smpi_mpi_wait(request, MPI_STATUS_IGNORE);
    }
    xbt_mallocator_release(smpi_global->request_mallocator, request);
  }

  smpi_bench_begin();

  return retval;
}

/**
 * MPI_Sendrecv
 **/
int SMPI_MPI_Sendrecv(void *sendbuf, int sendcount, MPI_Datatype sendtype, int dest, int sendtag, 
		    void *recvbuf, int recvcount, MPI_Datatype recvtype, int source, int recvtag,
		    MPI_Comm comm, MPI_Status *status)
{
int rank;
int retval = MPI_SUCCESS;
smpi_mpi_request_t srequest;
smpi_mpi_request_t rrequest;

	  rank = smpi_mpi_comm_rank(comm);

	  /* send */
	  /* -------------*/
	    retval = smpi_create_request(sendbuf, sendcount, sendtype, 
				rank,dest,sendtag, 
				comm, &srequest);
	  printf("[%d] isend request src=%d -> dst=%d (retval=%d)\n",rank,rank,dest,retval);
	  smpi_mpi_isend(srequest);
        
	  
	  //retval = MPI_Isend( sendbuf, sendcount, sendtype, dest, sendtag, MPI_COMM_WORLD, &srequest);


	  /* recv */
	  retval = smpi_create_request(recvbuf, recvcount, recvtype, 
				source, rank,recvtag, 
				comm, &rrequest);
	  printf("[%d] irecv request src=%d -> dst=%d (retval=%d)\n",rank,source,rank,retval);
	  smpi_mpi_irecv(rrequest);

	  //retval = MPI_Irecv( recvbuf, recvcount, recvtype, source, recvtag, MPI_COMM_WORLD, &rrequest);


	  smpi_mpi_wait(srequest, MPI_STATUS_IGNORE);
	  printf("[%d] isend request src=%d dst=%d tag=%d COMPLETED (retval=%d) \n",rank,rank,dest,sendtag,retval);

	  smpi_mpi_wait(rrequest, MPI_STATUS_IGNORE);
	  printf("[%d] irecv request src=%d -> dst=%d tag=%d COMPLETED (retval=%d)\n",rank,source,rank,recvtag,retval);

	  return(retval);
}


/**
 * MPI_Wait and friends
 **/
int SMPI_MPI_Wait(MPI_Request * request, MPI_Status * status)
{
  return smpi_mpi_wait(*request, status);
}

int SMPI_MPI_Waitall(int count, MPI_Request requests[], MPI_Status status[])
{
  return smpi_mpi_waitall(count, requests, status);
}

int SMPI_MPI_Waitany(int count, MPI_Request requests[], int *index,
                     MPI_Status status[])
{
  return smpi_mpi_waitany(count, requests, index, status);
}

/**
 * MPI_Bcast
 **/

/**
 * flat bcast 
 **/
int flat_tree_bcast(void *buf, int count, MPI_Datatype datatype, int root, MPI_Comm comm);
int flat_tree_bcast(void *buf, int count, MPI_Datatype datatype, int root,
                MPI_Comm comm)
{
        int rank;
        int retval = MPI_SUCCESS;
        smpi_mpi_request_t request;

        rank = smpi_mpi_comm_rank(comm);
        if (rank == root) {
                retval = smpi_create_request(buf, count, datatype, root,
                                (root + 1) % comm->size, 0, comm, &request);
                request->forward = comm->size - 1;
                smpi_mpi_isend(request);
        } else {
                retval = smpi_create_request(buf, count, datatype, MPI_ANY_SOURCE, rank,
                                0, comm, &request);
                smpi_mpi_irecv(request);
        }

        smpi_mpi_wait(request, MPI_STATUS_IGNORE);
        xbt_mallocator_release(smpi_global->request_mallocator, request);

        return(retval);

}
/**
 * Bcast internal level 
 **/
int smpi_mpi_bcast(void *buf, int count, MPI_Datatype datatype, int root,
                   MPI_Comm comm)
{
  int retval = MPI_SUCCESS;
  //retval = flat_tree_bcast(buf, count, datatype, root, comm);
  retval = nary_tree_bcast(buf, count, datatype, root, comm, 2 );
  return retval;
}

/**
 * Bcast user entry point
 **/
int SMPI_MPI_Bcast(void *buf, int count, MPI_Datatype datatype, int root,
                   MPI_Comm comm)
{
  int retval = MPI_SUCCESS;

  smpi_bench_end();
  smpi_mpi_bcast(buf,count,datatype,root,comm);
  smpi_bench_begin();

  return retval;
}



#ifdef DEBUG_REDUCE
/**
 * debugging helper function
 **/
static void print_buffer_int(void *buf, int len, char *msg, int rank)
{
  int tmp, *v;
  printf("**[%d] %s: ", rank, msg);
  for (tmp = 0; tmp < len; tmp++) {
    v = buf;
    printf("[%d]", v[tmp]);
  }
  printf("\n");
  free(msg);
}
static void print_buffer_double(void *buf, int len, char *msg, int rank)
{
  int tmp;
  double *v;
  printf("**[%d] %s: ", rank, msg);
  for (tmp = 0; tmp < len; tmp++) {
    v = buf;
    printf("[%lf]", v[tmp]);
  }
  printf("\n");
  free(msg);
}


#endif
/**
 * MPI_Reduce
 **/
int smpi_mpi_reduce(void *sendbuf, void *recvbuf, int count,
                MPI_Datatype datatype, MPI_Op op, int root, MPI_Comm comm)
{
        int retval = MPI_SUCCESS;
        int rank;
        int size;
        int i;
        int tag = 0;
        smpi_mpi_request_t *requests;
        smpi_mpi_request_t request;

        smpi_bench_end();

        rank = smpi_mpi_comm_rank(comm);
        size = comm->size;

        if (rank != root) {           // if i am not ROOT, simply send my buffer to root

#ifdef DEBUG_REDUCE
                print_buffer_int(sendbuf, count, xbt_strdup("sndbuf"), rank);
#endif
                retval = smpi_create_request(sendbuf, count, datatype, rank, root, tag, comm,
                                        &request);
                smpi_mpi_isend(request);
                smpi_mpi_wait(request, MPI_STATUS_IGNORE);
                xbt_mallocator_release(smpi_global->request_mallocator, request);

        } else {
                // i am the ROOT: wait for all buffers by creating one request by sender
                int src;
                requests = xbt_malloc((size-1) * sizeof(smpi_mpi_request_t));

                void **tmpbufs = xbt_malloc((size-1) * sizeof(void *));
                for (i = 0; i < size-1; i++) {
                        // we need 1 buffer per request to store intermediate receptions
                        tmpbufs[i] = xbt_malloc(count * datatype->size);
                }  
                // root: initiliaze recv buf with my own snd buf
                memcpy(recvbuf, sendbuf, count * datatype->size * sizeof(char));  

                // i can not use: 'request->forward = size-1;' (which would progagate size-1 receive reqs)
                // since we should op values as soon as one receiving request matches.
                for (i = 0; i < size-1; i++) {
                        // reminder: for smpi_create_request() the src is always the process sending.
                        src = i < root ? i : i + 1;
                        retval = smpi_create_request(tmpbufs[i], count, datatype,
                                        src, root, tag, comm, &(requests[i]));
                        if (NULL != requests[i] && MPI_SUCCESS == retval) {
                                if (MPI_SUCCESS == retval) {
                                        smpi_mpi_irecv(requests[i]);
                                }
                        }
                }
                // now, wait for completion of all irecv's.
                for (i = 0; i < size-1; i++) {
                        int index = MPI_UNDEFINED;
                        smpi_mpi_waitany( size-1, requests, &index, MPI_STATUS_IGNORE);
#ifdef DEBUG_REDUCE
                        printf ("MPI_Waitany() unblocked: root received (completes req[index=%d])\n",index);
                        print_buffer_int(tmpbufs[index], count, bprintf("tmpbufs[index=%d] (value received)", index),
                                        rank);
#endif

                        // arg 2 is modified
                        op->func(tmpbufs[index], recvbuf, &count, &datatype);
#ifdef DEBUG_REDUCE
                        print_buffer_int(recvbuf, count, xbt_strdup("rcvbuf"), rank);
#endif
                        xbt_free(tmpbufs[index]);
                        /* FIXME: with the following line, it  generates an
                         * [xbt_ex/CRITICAL] Conditional list not empty 162518800.
                         */
                        // xbt_mallocator_release(smpi_global->request_mallocator, requests[index]);
                }
                xbt_free(requests);
                xbt_free(tmpbufs);
        }
        return retval;
}

/**
 * MPI_Reduce user entry point
 **/
int SMPI_MPI_Reduce(void *sendbuf, void *recvbuf, int count,
		    MPI_Datatype datatype, MPI_Op op, int root, MPI_Comm comm)
{
int retval = MPI_SUCCESS;

	  smpi_bench_end();

	  retval = smpi_mpi_reduce(sendbuf, recvbuf, count, datatype, op, root, comm);

	  smpi_bench_begin();
	  return retval;
}



/**
 * MPI_Allreduce
 *
 * Same as MPI_REDUCE except that the result appears in the receive buffer of all the group members.
 **/
int SMPI_MPI_Allreduce( void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype,
		    MPI_Op op, MPI_Comm comm )
{
int retval = MPI_SUCCESS;
int root=0;  // arbitrary choice

	  smpi_bench_end();

	  retval = smpi_mpi_reduce( sendbuf, recvbuf, count, datatype, op, root, comm);
	  if (MPI_SUCCESS != retval)
		    return(retval);

	  retval = smpi_mpi_bcast( sendbuf, count, datatype, root, comm);

	  smpi_bench_end();
	  return( retval );
}


/**
 * MPI_Scatter user entry point
 **/
//int SMPI_MPI_Scatter(void *sendbuf, int sendcount, MPI_Datatype datatype, 
//		         void *recvbuf, int recvcount, MPI_Datatype recvtype,int root,
//		        MPI_Comm comm);
int SMPI_MPI_Scatter(void *sendbuf, int sendcount, MPI_Datatype datatype, 
		         void *recvbuf, int recvcount, MPI_Datatype recvtype,
			   int root, MPI_Comm comm)
{
  int retval = MPI_SUCCESS;
  int i;
  int cnt=0;  
  int rank;
  int tag=0;
  char *cptr;  // to manipulate the void * buffers
  smpi_mpi_request_t *requests;
  smpi_mpi_request_t request;
  smpi_mpi_status_t status;


  smpi_bench_end();

  rank = smpi_mpi_comm_rank(comm);

  requests = xbt_malloc((comm->size-1) * sizeof(smpi_mpi_request_t));
  if (rank == root) {
          // i am the root: distribute my sendbuf
          //print_buffer_int(sendbuf, comm->size, xbt_strdup("rcvbuf"), rank);
          cptr = sendbuf;
          for (i=0; i < comm->size; i++) {
                  if ( i!=root ) { // send to processes ...

                          retval = smpi_create_request((void *)cptr, sendcount, 
                                          datatype, root, i, tag, comm, &(requests[cnt]));
                          if (NULL != requests[cnt] && MPI_SUCCESS == retval) {
                                  if (MPI_SUCCESS == retval) {
                                          smpi_mpi_isend(requests[cnt]);
                                  }
				  }
				  cnt++;
			} 
			else { // ... except if it's me.
				  memcpy(recvbuf, (void *)cptr, recvcount*recvtype->size*sizeof(char));
			}
                  cptr += sendcount*datatype->size;
	    }
	    for(i=0; i<cnt; i++) { // wait for send to complete
			    /* FIXME: waitall() should be slightly better */
			    smpi_mpi_wait(requests[i], &status);
			    xbt_mallocator_release(smpi_global->request_mallocator, requests[i]);

	    }
  } 
  else {  // i am a non-root process: wait data from the root
	    retval = smpi_create_request(recvbuf,recvcount, 
				  recvtype, root, rank, tag, comm, &request);
	    if (NULL != request && MPI_SUCCESS == retval) {
			if (MPI_SUCCESS == retval) {
				  smpi_mpi_irecv(request);
			}
	    }
	    smpi_mpi_wait(request, &status);
	    xbt_mallocator_release(smpi_global->request_mallocator, request);
  }
  xbt_free(requests);

  smpi_bench_begin();

  return retval;
}


/**
 * MPI_Alltoall user entry point
 * 
 * Uses the logic of OpenMPI (upto 1.2.7 or greater) for the optimizations
 * ompi/mca/coll/tuned/coll_tuned_module.c
 **/
int SMPI_MPI_Alltoall(void *sendbuf, int sendcount, MPI_Datatype datatype, 
		         void *recvbuf, int recvcount, MPI_Datatype recvtype,
			   MPI_Comm comm)
{
  int retval = MPI_SUCCESS;
  int block_dsize;
  int rank;

  smpi_bench_end();

  rank = smpi_mpi_comm_rank(comm);
  block_dsize = datatype->size * sendcount;

  if ((block_dsize < 200) && (comm->size > 12)) {
	    retval = smpi_coll_tuned_alltoall_bruck(sendbuf, sendcount, datatype,
				  recvbuf, recvcount, recvtype, comm);

  } else if (block_dsize < 3000) {
/* use this one !!	    retval = smpi_coll_tuned_alltoall_basic_linear(sendbuf, sendcount, datatype,
				  recvbuf, recvcount, recvtype, comm);
				  */
  retval = smpi_coll_tuned_alltoall_pairwise(sendbuf, sendcount, datatype,
				  recvbuf, recvcount, recvtype, comm);
  } else {

  retval = smpi_coll_tuned_alltoall_pairwise(sendbuf, sendcount, datatype,
				  recvbuf, recvcount, recvtype, comm);
  }

  smpi_bench_begin();

  return retval;
}




// used by comm_split to sort ranks based on key values
int smpi_compare_rankkeys(const void *a, const void *b);
int smpi_compare_rankkeys(const void *a, const void *b)
{
  int *x = (int *) a;
  int *y = (int *) b;

  if (x[1] < y[1])
    return -1;

  if (x[1] == y[1]) {
    if (x[0] < y[0])
      return -1;
    if (x[0] == y[0])
      return 0;
    return 1;
  }

  return 1;
}

int SMPI_MPI_Comm_split(MPI_Comm comm, int color, int key,
                        MPI_Comm * comm_out)
{
  int retval = MPI_SUCCESS;

  int index, rank;
  smpi_mpi_request_t request;
  int colorkey[2];
  smpi_mpi_status_t status;

  smpi_bench_end();

  // FIXME: need to test parameters

  index = smpi_process_index();
  rank = comm->index_to_rank_map[index];

  // default output
  comm_out = NULL;

  // root node does most of the real work
  if (0 == rank) {
    int colormap[comm->size];
    int keymap[comm->size];
    int rankkeymap[comm->size * 2];
    int i, j;
    smpi_mpi_communicator_t tempcomm = NULL;
    int count;
    int indextmp;

    colormap[0] = color;
    keymap[0] = key;

    // FIXME: use scatter/gather or similar instead of individual comms
    for (i = 1; i < comm->size; i++) {
      retval = smpi_create_request(colorkey, 2, MPI_INT, MPI_ANY_SOURCE,
                                   rank, MPI_ANY_TAG, comm, &request);
      smpi_mpi_irecv(request);
      smpi_mpi_wait(request, &status);
      colormap[status.MPI_SOURCE] = colorkey[0];
      keymap[status.MPI_SOURCE] = colorkey[1];
      xbt_mallocator_release(smpi_global->request_mallocator, request);
    }

    for (i = 0; i < comm->size; i++) {
      if (MPI_UNDEFINED == colormap[i]) {
        continue;
      }
      // make a list of nodes with current color and sort by keys
      count = 0;
      for (j = i; j < comm->size; j++) {
        if (colormap[i] == colormap[j]) {
          colormap[j] = MPI_UNDEFINED;
          rankkeymap[count * 2] = j;
          rankkeymap[count * 2 + 1] = keymap[j];
          count++;
        }
      }
      qsort(rankkeymap, count, sizeof(int) * 2, &smpi_compare_rankkeys);

      // new communicator
      tempcomm = xbt_new(s_smpi_mpi_communicator_t, 1);
      tempcomm->barrier_count = 0;
      tempcomm->size = count;
      tempcomm->barrier_mutex = SIMIX_mutex_init();
      tempcomm->barrier_cond = SIMIX_cond_init();
      tempcomm->rank_to_index_map = xbt_new(int, count);
      tempcomm->index_to_rank_map = xbt_new(int, smpi_global->process_count);
      for (j = 0; j < smpi_global->process_count; j++) {
        tempcomm->index_to_rank_map[j] = -1;
      }
      for (j = 0; j < count; j++) {
        indextmp = comm->rank_to_index_map[rankkeymap[j * 2]];
        tempcomm->rank_to_index_map[j] = indextmp;
        tempcomm->index_to_rank_map[indextmp] = j;
      }
      for (j = 0; j < count; j++) {
        if (rankkeymap[j * 2]) {
          retval = smpi_create_request(&j, 1, MPI_INT, 0,
                                       rankkeymap[j * 2], 0, comm, &request);
          request->data = tempcomm;
          smpi_mpi_isend(request);
          smpi_mpi_wait(request, &status);
          xbt_mallocator_release(smpi_global->request_mallocator, request);
        } else {
          *comm_out = tempcomm;
        }
      }
    }
  } else {
    colorkey[0] = color;
    colorkey[1] = key;
    retval = smpi_create_request(colorkey, 2, MPI_INT, rank, 0, 0, comm,
                                 &request);
    smpi_mpi_isend(request);
    smpi_mpi_wait(request, &status);
    xbt_mallocator_release(smpi_global->request_mallocator, request);
    if (MPI_UNDEFINED != color) {
      retval = smpi_create_request(colorkey, 1, MPI_INT, 0, rank, 0, comm,
                                   &request);
      smpi_mpi_irecv(request);
      smpi_mpi_wait(request, &status);
      *comm_out = request->data;
    }
  }

  smpi_bench_begin();

  return retval;
}

double SMPI_MPI_Wtime(void)
{
  return (SIMIX_get_clock());
}


