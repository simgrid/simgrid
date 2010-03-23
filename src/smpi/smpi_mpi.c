/* $Id$tag */

#include "private.h"
#include "smpi_coll_private.h"
#include "smpi_mpi_dt_private.h"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(smpi_mpi, smpi,
                                "Logging specific to SMPI (mpi)");

/* MPI User level calls */

int MPI_Init(int* argc, char*** argv) {
  smpi_process_init(argc, argv);
  smpi_bench_begin(-1, NULL);
  return MPI_SUCCESS;
}

int MPI_Finalize(void) {
  smpi_bench_end(-1, NULL);
  smpi_process_destroy();
  return MPI_SUCCESS;
}

int MPI_Init_thread(int* argc, char*** argv, int required, int* provided) {
  if(provided != NULL) {
    *provided = MPI_THREAD_MULTIPLE;
  }
  return MPI_Init(argc, argv);
}

int MPI_Query_thread(int* provided) {
  int retval;

  smpi_bench_end(-1, NULL);
  if(provided == NULL) {
    retval = MPI_ERR_ARG;
  } else {
    *provided = MPI_THREAD_MULTIPLE;
    retval = MPI_SUCCESS;
  }
  smpi_bench_begin(-1, NULL);
  return retval;
}

int MPI_Is_thread_main(int* flag) {
  int retval;

  smpi_bench_end(-1, NULL);
  if(flag == NULL) {
    retval = MPI_ERR_ARG;
  } else {
    *flag = smpi_process_index() == 0;
    retval = MPI_SUCCESS;
  }
  smpi_bench_begin(-1, NULL);
  return retval;
}

int MPI_Abort(MPI_Comm comm, int errorcode) {
  smpi_bench_end(-1, NULL);
  smpi_process_destroy();
  // FIXME: should kill all processes in comm instead
  SIMIX_process_kill(SIMIX_process_self());
  return MPI_SUCCESS;
}

double MPI_Wtime(void) {
  double time;

  smpi_bench_end(-1, NULL);
  time = SIMIX_get_clock();
  smpi_bench_begin(-1, NULL);
  return time;
}

int MPI_Type_size(MPI_Datatype datatype, size_t* size) {
  int retval;

  smpi_bench_end(-1, NULL);
  if(datatype == MPI_DATATYPE_NULL) {
    retval = MPI_ERR_TYPE;
  } else if(size == NULL) {
    retval = MPI_ERR_ARG;
  } else {
    *size = smpi_datatype_size(datatype);
    retval = MPI_SUCCESS;
  }
  smpi_bench_begin(-1, NULL);
  return retval;
}

int MPI_Type_get_extent(MPI_Datatype datatype, MPI_Aint* lb, MPI_Aint* extent) {
  int retval;

  smpi_bench_end(-1, NULL);
  if(datatype == MPI_DATATYPE_NULL) {
    retval = MPI_ERR_TYPE;
  } else if(lb == NULL || extent == NULL) {
    retval = MPI_ERR_ARG;
  } else {
    retval = smpi_datatype_extent(datatype, lb, extent);
  }
  smpi_bench_begin(-1, NULL);
  return retval;
}

int MPI_Type_lb(MPI_Datatype datatype, MPI_Aint* disp) {
  int retval;

  smpi_bench_end(-1, NULL);
  if(datatype == MPI_DATATYPE_NULL) {
    retval = MPI_ERR_TYPE;
  } else if(disp == NULL) {
    retval = MPI_ERR_ARG;
  } else {
    *disp = smpi_datatype_lb(datatype);
    retval = MPI_SUCCESS;
  }
  smpi_bench_begin(-1, NULL);
  return retval;
}

int MPI_Type_ub(MPI_Datatype datatype, MPI_Aint* disp) {
  int retval;

  smpi_bench_end(-1, NULL);
  if(datatype == MPI_DATATYPE_NULL) {
    retval = MPI_ERR_TYPE;
  } else if(disp == NULL) {
    retval = MPI_ERR_ARG;
  } else {
    *disp = smpi_datatype_ub(datatype);
    retval = MPI_SUCCESS;
  }
  smpi_bench_begin(-1, NULL);
  return retval;
}

int MPI_Op_create(MPI_User_function* function, int commute, MPI_Op* op) {
  int retval;

  smpi_bench_end(-1, NULL);
  if(function == NULL || op == NULL) {
    retval = MPI_ERR_ARG;
  } else {
    *op = smpi_op_new(function, commute);
    retval = MPI_SUCCESS;
  }
  smpi_bench_begin(-1, NULL);
  return retval;
}

int MPI_Op_free(MPI_Op* op) {
  int retval;

  smpi_bench_end(-1, NULL);
  if(op == NULL) {
    retval = MPI_ERR_ARG;
  } else if(*op == MPI_OP_NULL) {
    retval = MPI_ERR_OP;
  } else {
    smpi_op_destroy(*op);
    *op = MPI_OP_NULL;
    retval = MPI_SUCCESS;
  }
  smpi_bench_begin(-1, NULL);
  return retval;
}

int MPI_Group_free(MPI_Group *group) {
  int retval;

  smpi_bench_end(-1, NULL);
  if(group == NULL) {
    retval = MPI_ERR_ARG;
  } else {
    smpi_group_destroy(*group);
    *group = MPI_GROUP_NULL;
    retval = MPI_SUCCESS;
  }
  smpi_bench_begin(-1, NULL);
  return retval;
}

int MPI_Group_size(MPI_Group group, int* size) {
  int retval;

  smpi_bench_end(-1, NULL);
  if(group == MPI_GROUP_NULL) {
    retval = MPI_ERR_GROUP;
  } else if(size == NULL) {
    retval = MPI_ERR_ARG;
  } else {
    *size = smpi_group_size(group);
    retval = MPI_SUCCESS;
  }
  smpi_bench_begin(-1, NULL);
  return retval;
}

int MPI_Group_rank(MPI_Group group, int* rank) {
  int retval;

  smpi_bench_end(-1, NULL);
  if(group == MPI_GROUP_NULL) {
    retval = MPI_ERR_GROUP;
  } else if(rank == NULL) {
    retval = MPI_ERR_ARG;
  } else {
    *rank = smpi_group_rank(group, smpi_process_index());
    retval = MPI_SUCCESS;
  }
  smpi_bench_begin(-1, NULL);
  return retval;
}

int MPI_Group_translate_ranks (MPI_Group group1, int n, int* ranks1, MPI_Group group2, int* ranks2) {
  int retval, i, index;

  smpi_bench_end(-1, NULL);
  if(group1 == MPI_GROUP_NULL || group2 == MPI_GROUP_NULL) {
    retval = MPI_ERR_GROUP;
  } else {
    for(i = 0; i < n; i++) {
      index = smpi_group_index(group1, ranks1[i]);
      ranks2[i] = smpi_group_rank(group2, index);
    }
    retval = MPI_SUCCESS;
  }
  smpi_bench_begin(-1, NULL);
  return retval;
}

int MPI_Group_compare(MPI_Group group1, MPI_Group group2, int* result) {
  int retval;

  smpi_bench_end(-1, NULL);
  if(group1 == MPI_GROUP_NULL || group2 == MPI_GROUP_NULL) {
    retval = MPI_ERR_GROUP;
  } else if(result == NULL) {
    retval = MPI_ERR_ARG;
  } else {
    *result = smpi_group_compare(group1, group2);
    retval = MPI_SUCCESS;
  }
  smpi_bench_begin(-1, NULL);
  return retval;
}

int MPI_Group_union(MPI_Group group1, MPI_Group group2, MPI_Group* newgroup) {
  int retval, i, proc1, proc2, size, size2;

  smpi_bench_end(-1, NULL);
  if(group1 == MPI_GROUP_NULL || group2 == MPI_GROUP_NULL) {
    retval = MPI_ERR_GROUP;
  } else if(newgroup == NULL) {
    retval = MPI_ERR_ARG;
  } else {
    size = smpi_group_size(group1);
    size2 = smpi_group_size(group2);
    for(i = 0; i < size2; i++) {
      proc2 = smpi_group_index(group2, i);
      proc1 = smpi_group_rank(group1, proc2);
      if(proc1 == MPI_UNDEFINED) {
        size++;
      }
    }
    if(size == 0) {
      *newgroup = MPI_GROUP_EMPTY;
    } else {
      *newgroup = smpi_group_new(size);
      size2 = smpi_group_size(group1);
      for(i = 0; i < size2; i++) {
        proc1 = smpi_group_index(group1, i);
        smpi_group_set_mapping(*newgroup, proc1, i);
      }
      for(i = size2; i < size; i++) {
        proc2 = smpi_group_index(group2, i - size2);
        smpi_group_set_mapping(*newgroup, proc2, i);
      }
    }
    smpi_group_use(*newgroup);
    retval = MPI_SUCCESS;
  }
  smpi_bench_begin(-1, NULL);
  return retval;
}

int MPI_Group_intersection(MPI_Group group1, MPI_Group group2, MPI_Group* newgroup) {
   int retval, i, proc1, proc2, size, size2;

  smpi_bench_end(-1, NULL);
  if(group1 == MPI_GROUP_NULL || group2 == MPI_GROUP_NULL) {
    retval = MPI_ERR_GROUP;
  } else if(newgroup == NULL) {
    retval = MPI_ERR_ARG;
  } else {
    size = smpi_group_size(group1);
    size2 = smpi_group_size(group2);
    for(i = 0; i < size2; i++) {
      proc2 = smpi_group_index(group2, i);
      proc1 = smpi_group_rank(group1, proc2);
      if(proc1 == MPI_UNDEFINED) {
        size--;
      }
    }
    if(size == 0) {
      *newgroup = MPI_GROUP_EMPTY;
    } else {
      *newgroup = smpi_group_new(size);
      size2 = smpi_group_size(group1);
      for(i = 0; i < size2; i++) {
        proc1 = smpi_group_index(group1, i);
        proc2 = smpi_group_rank(group2, proc1);
        if(proc2 != MPI_UNDEFINED) {
          smpi_group_set_mapping(*newgroup, proc1, i);
        }
      }
    }
    smpi_group_use(*newgroup);
    retval = MPI_SUCCESS;
  }
  smpi_bench_begin(-1, NULL);
  return retval;
}

int MPI_Group_difference(MPI_Group group1, MPI_Group group2, MPI_Group* newgroup) {
  int retval, i, proc1, proc2, size, size2;

  smpi_bench_end(-1, NULL);
  if(group1 == MPI_GROUP_NULL || group2 == MPI_GROUP_NULL) {
    retval = MPI_ERR_GROUP;
  } else if(newgroup == NULL) {
    retval = MPI_ERR_ARG;
  } else {
    size = size2 = smpi_group_size(group1);
    for(i = 0; i < size2; i++) {
      proc1 = smpi_group_index(group1, i);
      proc2 = smpi_group_rank(group2, proc1);
      if(proc2 != MPI_UNDEFINED) {
        size--;
      }
    }
    if(size == 0) {
      *newgroup = MPI_GROUP_EMPTY;
    } else {
      *newgroup = smpi_group_new(size);
      for(i = 0; i < size2; i++) {
        proc1 = smpi_group_index(group1, i);
        proc2 = smpi_group_rank(group2, proc1);
        if(proc2 == MPI_UNDEFINED) {
          smpi_group_set_mapping(*newgroup, proc1, i);
        }
      }
    }
    smpi_group_use(*newgroup);
    retval = MPI_SUCCESS;
  }
  smpi_bench_begin(-1, NULL);
  return retval;
}

int MPI_Group_incl(MPI_Group group, int n, int* ranks, MPI_Group* newgroup) {
  int retval, i, index;

  smpi_bench_end(-1, NULL);
  if(group == MPI_GROUP_NULL) {
    retval = MPI_ERR_GROUP;
  } else if(newgroup == NULL) {
    retval = MPI_ERR_ARG;
  } else {
    if(n == 0) {
      *newgroup = MPI_GROUP_EMPTY;
    } else if(n == smpi_group_size(group)) {
      *newgroup = group;
    } else {
      *newgroup = smpi_group_new(n);
      for(i = 0; i < n; i++) {
        index = smpi_group_index(group, ranks[i]);
        smpi_group_set_mapping(*newgroup, index, i);
      }
    }
    smpi_group_use(*newgroup);
    retval = MPI_SUCCESS;
  }
  smpi_bench_begin(-1, NULL);
  return retval;
}

int MPI_Group_excl(MPI_Group group, int n, int* ranks, MPI_Group* newgroup) {
  int retval, i, size, rank, index;

  smpi_bench_end(-1, NULL);
  if(group == MPI_GROUP_NULL) {
    retval = MPI_ERR_GROUP;
  } else if(newgroup == NULL) {
    retval = MPI_ERR_ARG;
  } else {
    if(n == 0) {
      *newgroup = group;
    } else if(n == smpi_group_size(group)) {
      *newgroup = MPI_GROUP_EMPTY;
    } else {
      size = smpi_group_size(group) - n;
      *newgroup = smpi_group_new(size);
      rank = 0;
      while(rank < size) {
        for(i = 0; i < n; i++) {
          if(ranks[i] == rank) {
            break;
          }
        }
        if(i >= n) {
          index = smpi_group_index(group, rank);
          smpi_group_set_mapping(*newgroup, index, rank);
          rank++;
        }
      }
    }
    smpi_group_use(*newgroup);
    retval = MPI_SUCCESS;
  }
  smpi_bench_begin(-1, NULL);
  return retval;
}

int MPI_Group_range_incl(MPI_Group group, int n, int ranges[][3], MPI_Group* newgroup) {
  int retval, i, j, rank, size, index;

  smpi_bench_end(-1, NULL);
  if(group == MPI_GROUP_NULL) {
    retval = MPI_ERR_GROUP;
  } else if(newgroup == NULL) {
    retval = MPI_ERR_ARG;
  } else {
    if(n == 0) {
      *newgroup = MPI_GROUP_EMPTY;
    } else {
      size = 0;
      for(i = 0; i < n; i++) {
        for(rank = ranges[i][0]; /* First */
            rank >= 0 && rank <= ranges[i][1]; /* Last */
            rank += ranges[i][2] /* Stride */) {
          size++;
        }
      }
      if(size == smpi_group_size(group)) {
        *newgroup = group;
      } else {
        *newgroup = smpi_group_new(size);
        j = 0;
        for(i = 0; i < n; i++) {
          for(rank = ranges[i][0]; /* First */
              rank >= 0 && rank <= ranges[i][1]; /* Last */
              rank += ranges[i][2] /* Stride */) {
            index = smpi_group_index(group, rank);
            smpi_group_set_mapping(*newgroup, index, j);
            j++;
          }
        }
      }
    }
    smpi_group_use(*newgroup);
    retval = MPI_SUCCESS;
  }
  smpi_bench_begin(-1, NULL);
  return retval;
}

int MPI_Group_range_excl(MPI_Group group, int n, int ranges[][3], MPI_Group* newgroup) {
  int retval, i, newrank, rank, size, index, add;

  smpi_bench_end(-1, NULL);
  if(group == MPI_GROUP_NULL) {
    retval = MPI_ERR_GROUP;
  } else if(newgroup == NULL) {
    retval = MPI_ERR_ARG;
  } else {
    if(n == 0) {
      *newgroup = group;
    } else {
      size = smpi_group_size(group);
      for(i = 0; i < n; i++) {
        for(rank = ranges[i][0]; /* First */
            rank >= 0 && rank <= ranges[i][1]; /* Last */
            rank += ranges[i][2] /* Stride */) {
          size--;
        }
      }
      if(size == 0) {
        *newgroup = MPI_GROUP_EMPTY;
      } else {
        *newgroup = smpi_group_new(size);
        newrank = 0;
        while(newrank < size) {
          for(i = 0; i < n; i++) {
            add = 1;
            for(rank = ranges[i][0]; /* First */
                rank >= 0 && rank <= ranges[i][1]; /* Last */
                rank += ranges[i][2] /* Stride */) {
              if(rank == newrank) {
                add = 0;
                break;
              }
            }
            if(add == 1) {
              index = smpi_group_index(group, newrank);
              smpi_group_set_mapping(*newgroup, index, newrank);
            }
          }
        }
      }
    }
    smpi_group_use(*newgroup);
    retval = MPI_SUCCESS;
  }
  smpi_bench_begin(-1, NULL);
  return retval;
}

int MPI_Comm_rank(MPI_Comm comm, int* rank) {
  int retval;

  smpi_bench_end(-1, NULL);
  if(comm == MPI_COMM_NULL) {
    retval = MPI_ERR_COMM;
  } else {
    *rank = smpi_comm_rank(comm);
    retval = MPI_SUCCESS;
  }
  smpi_bench_begin(-1, NULL);
  return retval;
}

int MPI_Comm_size(MPI_Comm comm, int* size) {
  int retval;

  smpi_bench_end(-1, NULL);
  if(comm == MPI_COMM_NULL) {
    retval = MPI_ERR_COMM;
  } else if(size == NULL) {
    retval = MPI_ERR_ARG;
  } else {
    *size = smpi_comm_size(comm);
    retval = MPI_SUCCESS;
  }
  smpi_bench_begin(-1, NULL);
  return retval;
}

int MPI_Comm_group(MPI_Comm comm, MPI_Group* group) {
  int retval;

  smpi_bench_end(-1, NULL);
  if(comm == MPI_COMM_NULL) {
    retval = MPI_ERR_COMM;
  } else if(group == NULL) {
    retval = MPI_ERR_ARG;
  } else {
    *group = smpi_comm_group(comm);
    retval = MPI_SUCCESS;
  }
  smpi_bench_begin(-1, NULL);
  return retval;
}

int MPI_Comm_compare(MPI_Comm comm1, MPI_Comm comm2, int* result) {
  int retval;

  smpi_bench_end(-1, NULL);
  if(comm1 == MPI_COMM_NULL || comm2 == MPI_COMM_NULL) {
    retval = MPI_ERR_COMM;
  } else if(result == NULL) {
    retval = MPI_ERR_ARG;
  } else {
    if(comm1 == comm2) { /* Same communicators means same groups */
      *result = MPI_IDENT;
    } else {
      *result = smpi_group_compare(smpi_comm_group(comm1), smpi_comm_group(comm2));
      if(*result == MPI_IDENT) {
        *result = MPI_CONGRUENT;
      }
    }
    retval = MPI_SUCCESS;
  }
  smpi_bench_begin(-1, NULL);
  return retval;
}

int MPI_Comm_dup(MPI_Comm comm, MPI_Comm* newcomm) {
  int retval;

  smpi_bench_end(-1, NULL);
  if(comm == MPI_COMM_NULL) {
    retval = MPI_ERR_COMM;
  } else if(newcomm == NULL) {
    retval = MPI_ERR_ARG;
  } else {
    *newcomm = smpi_comm_new(smpi_comm_group(comm));
    retval = MPI_SUCCESS;
  }
  smpi_bench_begin(-1, NULL);
  return retval;
}

int MPI_Comm_create(MPI_Comm comm, MPI_Group group, MPI_Comm* newcomm) {
  int retval;

  smpi_bench_end(-1, NULL);
  if(comm == MPI_COMM_NULL) {
    retval = MPI_ERR_COMM;
  } else if(group == MPI_GROUP_NULL) {
    retval = MPI_ERR_GROUP;
  } else if(newcomm == NULL) {
    retval = MPI_ERR_ARG;
  } else {
    *newcomm = smpi_comm_new(group);
    retval = MPI_SUCCESS;
  }
  smpi_bench_begin(-1, NULL);
  return retval;
}

int MPI_Comm_free(MPI_Comm* comm) {
  int retval;

  smpi_bench_end(-1, NULL);
  if(comm == NULL) {
    retval = MPI_ERR_ARG;
  } else if(*comm == MPI_COMM_NULL) {
    retval = MPI_ERR_COMM;
  } else {
    smpi_comm_destroy(*comm);
    *comm = MPI_COMM_NULL;
    retval = MPI_SUCCESS;
  }
  smpi_bench_begin(-1, NULL);
  return retval;
}

int MPI_Irecv(void* buf, int count, MPI_Datatype datatype, int src, int tag, MPI_Comm comm, MPI_Request* request) {
  int retval;
  int rank = comm != MPI_COMM_NULL ? smpi_comm_rank(comm) : -1;

  smpi_bench_end(rank, "Irecv");
  if(request == NULL) {
    retval = MPI_ERR_ARG;
  } else if (comm == MPI_COMM_NULL) {
    retval = MPI_ERR_COMM;
  } else {
    *request = smpi_mpi_irecv(buf, count, datatype, src, tag, comm);
    retval = MPI_SUCCESS;
  }
  smpi_bench_begin(rank, "Irecv");
  return retval;
}

int MPI_Isend(void* buf, int count, MPI_Datatype datatype, int dst, int tag, MPI_Comm comm, MPI_Request* request) {
  int retval;
  int rank = comm != MPI_COMM_NULL ? smpi_comm_rank(comm) : -1;

  smpi_bench_end(rank, "Isend");
  if(request == NULL) {
    retval = MPI_ERR_ARG;
  } else if (comm == MPI_COMM_NULL) {
    retval = MPI_ERR_COMM;
  } else {
    *request = smpi_mpi_isend(buf, count, datatype, dst, tag, comm);
    retval = MPI_SUCCESS;
  }
  smpi_bench_begin(rank, "Isend");
  return retval;
}

int MPI_Recv(void* buf, int count, MPI_Datatype datatype, int src, int tag, MPI_Comm comm, MPI_Status* status) {
  int retval;
  int rank = comm != MPI_COMM_NULL ? smpi_comm_rank(comm) : -1;

  smpi_bench_end(rank, "Recv");
  if (comm == MPI_COMM_NULL) {
    retval = MPI_ERR_COMM;
  } else {
    smpi_mpi_recv(buf, count, datatype, src, tag, comm, status);
    retval = MPI_SUCCESS;
  }
  smpi_bench_begin(rank, "Recv");
  return retval;
}

int MPI_Send(void* buf, int count, MPI_Datatype datatype, int dst, int tag, MPI_Comm comm) {
  int retval;
  int rank = comm != MPI_COMM_NULL ? smpi_comm_rank(comm) : -1;

  smpi_bench_end(rank, "Send");
  if (comm == MPI_COMM_NULL) {
    retval = MPI_ERR_COMM;
  } else {
    smpi_mpi_send(buf, count, datatype, dst, tag, comm);
    retval = MPI_SUCCESS;
  }
  smpi_bench_begin(rank, "Send");
  return retval;
}

int MPI_Sendrecv(void* sendbuf, int sendcount, MPI_Datatype sendtype, int dst, int sendtag, void* recvbuf, int recvcount, MPI_Datatype recvtype, int src, int recvtag, MPI_Comm comm, MPI_Status* status) {
  int retval;
  int rank = comm != MPI_COMM_NULL ? smpi_comm_rank(comm) : -1;

  smpi_bench_end(rank, "Sendrecv");
  if (comm == MPI_COMM_NULL) {
    retval = MPI_ERR_COMM;
  } else if (sendtype == MPI_DATATYPE_NULL || recvtype == MPI_DATATYPE_NULL) {
    retval = MPI_ERR_TYPE;
  } else {
    smpi_mpi_sendrecv(sendbuf, sendcount, sendtype, dst, sendtag, recvbuf, recvcount, recvtype, src, recvtag, comm, status);
    retval = MPI_SUCCESS;
  }
  smpi_bench_begin(rank, "Sendrecv");
  return retval;
}

int MPI_Sendrecv_replace(void* buf, int count, MPI_Datatype datatype, int dst, int sendtag, int src, int recvtag, MPI_Comm comm, MPI_Status* status) {
  //TODO: suboptimal implementation
  void* recvbuf;
  int retval, size;

  size = smpi_datatype_size(datatype) * count;
  recvbuf = xbt_new(char, size);
  retval = MPI_Sendrecv(buf, count, datatype, dst, sendtag, recvbuf, count, datatype, src, recvtag, comm, status);
  memcpy(buf, recvbuf, size * sizeof(char));
  xbt_free(recvbuf);
  return retval;
}

int MPI_Get_count(MPI_Status *status, MPI_Datatype datatype, int *count) {
  int retval;
/*
 * Returns the number of entries received. (Again, we count entries, each of type datatype, not bytes.) 
 * The datatype argument should match the argument provided by the receive call that set the status variable.
 * If the size of the datatype is zero, this routine will return a count of zero. 
 * If the amount of data in status is not an exact multiple of the size of datatype 
 * (so that count would not be integral), a count of MPI_UNDEFINED is returned instead.
 *
 */
  smpi_bench_end(-1, NULL); //FIXME

  if( 0==smpi_datatype_size(datatype)) {
          // also check that the type is 'committed' when we have MPI_Type_commit (s.g. 23/03/21010)
	    retval = MPI_ERR_TYPE;
  } else {
	    smpi_mpi_get_count(status, datatype, count);
	    retval = MPI_SUCCESS;
  }
  smpi_bench_begin(-1, NULL);
  return retval;
}


int MPI_Test(MPI_Request* request, int* flag, MPI_Status* status) {
  int retval;
  int rank = request && (*request)->comm != MPI_COMM_NULL
             ? smpi_comm_rank((*request)->comm)
             : -1;

  smpi_bench_end(rank, "Test");
  if(request == NULL || flag == NULL) {
    retval = MPI_ERR_ARG;
  } else if(*request == MPI_REQUEST_NULL) {
    retval = MPI_ERR_REQUEST;
  } else {
    *flag = smpi_mpi_test(request, status);
    retval = MPI_SUCCESS;
  }
  smpi_bench_begin(rank, "Test");
  return retval;
}

int MPI_Testany(int count, MPI_Request requests[], int* index, int* flag, MPI_Status* status) {
  int retval;

  smpi_bench_end(-1, NULL); //FIXME
  if(index == NULL || flag == NULL) {
    retval = MPI_ERR_ARG;
  } else {
    *flag = smpi_mpi_testany(count, requests, index, status);
    retval = MPI_SUCCESS;
  }
  smpi_bench_begin(-1, NULL);
  return retval;
}

int MPI_Wait(MPI_Request* request, MPI_Status* status) {
  int retval;
  int rank = request && (*request)->comm != MPI_COMM_NULL
             ? smpi_comm_rank((*request)->comm)
             : -1;

  smpi_bench_end(rank, "Wait");
  if(request == NULL) {
    retval = MPI_ERR_ARG;
  } else if(*request == MPI_REQUEST_NULL) {
    retval = MPI_ERR_REQUEST;
  } else {
    smpi_mpi_wait(request, status);
    retval = MPI_SUCCESS;
  }
  smpi_bench_begin(rank, "Wait");
  return retval;
}

int MPI_Waitany(int count, MPI_Request requests[], int* index, MPI_Status* status) {
  int retval;

  smpi_bench_end(-1, NULL); //FIXME
  if(index == NULL) {
    retval = MPI_ERR_ARG;
  } else {
    *index = smpi_mpi_waitany(count, requests, status);
    retval = MPI_SUCCESS;
  }
  smpi_bench_begin(-1, NULL);
  return retval;
}

int MPI_Waitall(int count, MPI_Request requests[],  MPI_Status status[]) {
  smpi_bench_end(-1, NULL); //FIXME
  smpi_mpi_waitall(count, requests, status);
  smpi_bench_begin(-1, NULL);
  return MPI_SUCCESS;
}

int MPI_Waitsome(int incount, MPI_Request requests[], int* outcount, int* indices, MPI_Status status[]) {
  int retval;

  smpi_bench_end(-1, NULL); //FIXME
  if(outcount == NULL || indices == NULL) {
    retval = MPI_ERR_ARG;
  } else {
    *outcount = smpi_mpi_waitsome(incount, requests, indices, status);
    retval = MPI_SUCCESS;
  }
  smpi_bench_begin(-1, NULL);
  return retval;
}

int MPI_Bcast(void* buf, int count, MPI_Datatype datatype, int root, MPI_Comm comm) {
  int retval;
  int rank = comm != MPI_COMM_NULL ? smpi_comm_rank(comm) : -1;

  smpi_bench_end(rank, "Bcast");
  if(comm == MPI_COMM_NULL) {
    retval = MPI_ERR_COMM;
  } else {
    smpi_mpi_bcast(buf, count, datatype, root, comm);
    retval = MPI_SUCCESS;
  }
  smpi_bench_begin(rank, "Bcast");
  return retval;
}

int MPI_Barrier(MPI_Comm comm) {
  int retval;
  int rank = comm != MPI_COMM_NULL ? smpi_comm_rank(comm) : -1;

  smpi_bench_end(rank, "Barrier");
  if(comm == MPI_COMM_NULL) {
    retval = MPI_ERR_COMM;
  } else {
    smpi_mpi_barrier(comm);
    retval = MPI_SUCCESS;
  }
  smpi_bench_begin(rank, "Barrier");
  return retval;
}

int MPI_Gather(void* sendbuf, int sendcount, MPI_Datatype sendtype, void* recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm) {
  int retval;
  int rank = comm != MPI_COMM_NULL ? smpi_comm_rank(comm) : -1;

  smpi_bench_end(rank, "Gather");
  if(comm == MPI_COMM_NULL) {
    retval = MPI_ERR_COMM;
  } else if(sendtype == MPI_DATATYPE_NULL || recvtype == MPI_DATATYPE_NULL) {
    retval = MPI_ERR_TYPE;
  } else {
    smpi_mpi_gather(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, root, comm);
    retval = MPI_SUCCESS;
  }
  smpi_bench_begin(rank, "Gather");
  return retval;
}

int MPI_Gatherv(void* sendbuf, int sendcount, MPI_Datatype sendtype, void* recvbuf, int* recvcounts, int* displs, MPI_Datatype recvtype, int root, MPI_Comm comm) {
  int retval;
  int rank = comm != MPI_COMM_NULL ? smpi_comm_rank(comm) : -1;

  smpi_bench_end(rank, "Gatherv");
  if(comm == MPI_COMM_NULL) {
    retval = MPI_ERR_COMM;
  } else if(sendtype == MPI_DATATYPE_NULL || recvtype == MPI_DATATYPE_NULL) {
    retval = MPI_ERR_TYPE;
  } else if(recvcounts == NULL || displs == NULL) {
    retval = MPI_ERR_ARG;
  } else {
    smpi_mpi_gatherv(sendbuf, sendcount, sendtype, recvbuf, recvcounts, displs, recvtype, root, comm);
    retval = MPI_SUCCESS;
  }
  smpi_bench_begin(rank, "Gatherv");
  return retval;
}

int MPI_Allgather(void* sendbuf, int sendcount, MPI_Datatype sendtype, void* recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm) {
  int retval;
  int rank = comm != MPI_COMM_NULL ? smpi_comm_rank(comm) : -1;

  smpi_bench_end(rank, "Allgather");
  if(comm == MPI_COMM_NULL) {
    retval = MPI_ERR_COMM;
  } else if(sendtype == MPI_DATATYPE_NULL || recvtype == MPI_DATATYPE_NULL) {
    retval = MPI_ERR_TYPE;
  } else {
    smpi_mpi_allgather(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, comm);
    retval = MPI_SUCCESS;
  }
  smpi_bench_begin(rank, "Allgather");
  return retval;
}

int MPI_Allgatherv(void* sendbuf, int sendcount, MPI_Datatype sendtype, void* recvbuf, int* recvcounts, int* displs, MPI_Datatype recvtype, MPI_Comm comm) {
  int retval;
  int rank = comm != MPI_COMM_NULL ? smpi_comm_rank(comm) : -1;

  smpi_bench_end(rank, "Allgatherv");
  if(comm == MPI_COMM_NULL) {
    retval = MPI_ERR_COMM;
  } else if(sendtype == MPI_DATATYPE_NULL || recvtype == MPI_DATATYPE_NULL) {
    retval = MPI_ERR_TYPE;
  } else if(recvcounts == NULL || displs == NULL) {
    retval = MPI_ERR_ARG;
  } else {
    smpi_mpi_allgatherv(sendbuf, sendcount, sendtype, recvbuf, recvcounts, displs, recvtype, comm);
    retval = MPI_SUCCESS;
  }
  smpi_bench_begin(rank, "Allgatherv");
  return retval;
}

int MPI_Scatter(void* sendbuf, int sendcount, MPI_Datatype sendtype, void* recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm) {
  int retval;
  int rank = comm != MPI_COMM_NULL ? smpi_comm_rank(comm) : -1;

  smpi_bench_end(rank, "Scatter");
  if(comm == MPI_COMM_NULL) {
    retval = MPI_ERR_COMM;
  } else if(sendtype == MPI_DATATYPE_NULL || recvtype == MPI_DATATYPE_NULL) {
    retval = MPI_ERR_TYPE;
  } else {
    smpi_mpi_scatter(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, root, comm);
    retval = MPI_SUCCESS;
  }
  smpi_bench_begin(rank, "Scatter");
  return retval;
}

int MPI_Scatterv(void* sendbuf, int* sendcounts, int* displs, MPI_Datatype sendtype, void* recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm) {
  int retval;
  int rank = comm != MPI_COMM_NULL ? smpi_comm_rank(comm) : -1;

  smpi_bench_end(rank, "Scatterv");
  if(comm == MPI_COMM_NULL) {
    retval = MPI_ERR_COMM;
  } else if(sendtype == MPI_DATATYPE_NULL || recvtype == MPI_DATATYPE_NULL) {
    retval = MPI_ERR_TYPE;
  } else if(sendcounts == NULL || displs == NULL) {
    retval = MPI_ERR_ARG;
  } else {
    smpi_mpi_scatterv(sendbuf, sendcounts, displs, sendtype, recvbuf, recvcount, recvtype, root, comm);
    retval = MPI_SUCCESS;
  }
  smpi_bench_begin(rank, "Scatterv");
  return retval;
}

int MPI_Reduce(void* sendbuf, void* recvbuf, int count, MPI_Datatype datatype, MPI_Op op, int root, MPI_Comm comm) {
  int retval;
  int rank = comm != MPI_COMM_NULL ? smpi_comm_rank(comm) : -1;

  smpi_bench_end(rank, "Reduce");
  if(comm == MPI_COMM_NULL) {
    retval = MPI_ERR_COMM;
  } else if(datatype == MPI_DATATYPE_NULL || op == MPI_OP_NULL) {
    retval = MPI_ERR_ARG;
  } else {
    smpi_mpi_reduce(sendbuf, recvbuf, count, datatype, op, root, comm);
    retval = MPI_SUCCESS;
  }
  smpi_bench_begin(rank, "Reduce");
  return retval;
}

int MPI_Allreduce(void* sendbuf, void* recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm) {
  int retval;
  int rank = comm != MPI_COMM_NULL ? smpi_comm_rank(comm) : -1;

  smpi_bench_end(rank, "Allreduce");
  if(comm == MPI_COMM_NULL) {
    retval = MPI_ERR_COMM;
  } else if(datatype == MPI_DATATYPE_NULL) {
    retval = MPI_ERR_TYPE;
  } else if(op == MPI_OP_NULL) {
    retval = MPI_ERR_OP;
  } else {
    smpi_mpi_allreduce(sendbuf, recvbuf, count, datatype, op, comm);
    retval = MPI_SUCCESS;
  }
  smpi_bench_begin(rank, "Allreduce");
  return retval;
}

int MPI_Reduce_scatter(void* sendbuf, void* recvbuf, int* recvcounts, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm) {
  int retval, i, size, count;
  int* displs;
  int rank = comm != MPI_COMM_NULL ? smpi_comm_rank(comm) : -1;

  smpi_bench_end(rank, "Reduce_scatter");
  if(comm == MPI_COMM_NULL) {
    retval = MPI_ERR_COMM;
  } else if(datatype == MPI_DATATYPE_NULL) {
    retval = MPI_ERR_TYPE;
  } else if(op == MPI_OP_NULL) {
    retval = MPI_ERR_OP;
  } else if(recvcounts == NULL) {
    retval = MPI_ERR_ARG;
  } else {
    /* arbitrarily choose root as rank 0 */
    /* TODO: faster direct implementation ? */
    size = smpi_comm_size(comm);
    count = 0;
    displs = xbt_new(int, size);
    for(i = 0; i < size; i++) {
      count += recvcounts[i];
      displs[i] = 0;
    }
    smpi_mpi_reduce(sendbuf, recvbuf, count, datatype, op, 0, comm);
    smpi_mpi_scatterv(recvbuf, recvcounts, displs, datatype, recvbuf, recvcounts[rank], datatype, 0, comm);
    xbt_free(displs);
    retval = MPI_SUCCESS;
  }
  smpi_bench_begin(rank, "Reduce_scatter");
  return retval;
}

int MPI_Alltoall(void* sendbuf, int sendcount, MPI_Datatype sendtype, void* recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm) {
  int retval, size, sendsize;
  int rank = comm != MPI_COMM_NULL ? smpi_comm_rank(comm) : -1;

  smpi_bench_end(rank, "Alltoall");
  if(comm == MPI_COMM_NULL) {
    retval = MPI_ERR_COMM;
  } else if(sendtype == MPI_DATATYPE_NULL || recvtype == MPI_DATATYPE_NULL) {
    retval = MPI_ERR_TYPE;
  } else {
    size = smpi_comm_size(comm);
    sendsize = smpi_datatype_size(sendtype) * sendcount;
    if(sendsize < 200 && size > 12) {
      retval = smpi_coll_tuned_alltoall_bruck(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, comm);
    } else if(sendsize < 3000) {
      retval = smpi_coll_tuned_alltoall_basic_linear(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, comm);
    } else {
      retval = smpi_coll_tuned_alltoall_pairwise(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, comm);
    }
  }
  smpi_bench_begin(rank, "Alltoall");
  return retval;
}

int MPI_Alltoallv(void* sendbuf, int* sendcounts, int* senddisps, MPI_Datatype sendtype, void* recvbuf, int *recvcounts, int* recvdisps, MPI_Datatype recvtype, MPI_Comm comm) {
  int retval;
  int rank = comm != MPI_COMM_NULL ? smpi_comm_rank(comm) : -1;

  smpi_bench_end(rank, "Alltoallv");
  if(comm == MPI_COMM_NULL) {
    retval = MPI_ERR_COMM;
  } else if(sendtype == MPI_DATATYPE_NULL || recvtype == MPI_DATATYPE_NULL) {
    retval = MPI_ERR_TYPE;
  } else if(sendcounts == NULL || senddisps == NULL || recvcounts == NULL || recvdisps == NULL) {
    retval = MPI_ERR_ARG;
  } else {
    retval = smpi_coll_basic_alltoallv(sendbuf, sendcounts, senddisps, sendtype, recvbuf, recvcounts, recvdisps, recvtype, comm); 
  }
  smpi_bench_begin(rank, "Alltoallv");
  return retval;
}


int MPI_Get_processor_name( char *name, int *resultlen ) {
  int retval = MPI_SUCCESS;
  smpi_bench_end(-1, NULL);
  strncpy( name , SIMIX_host_get_name(SIMIX_host_self()), MPI_MAX_PROCESSOR_NAME-1);
  *resultlen= strlen(name) > MPI_MAX_PROCESSOR_NAME ? MPI_MAX_PROCESSOR_NAME : strlen(name);

  smpi_bench_begin(-1, NULL);
  return retval;
}

