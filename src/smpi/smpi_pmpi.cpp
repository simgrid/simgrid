
/* Copyright (c) 2007-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "private.h"
#include "smpi_mpi_dt_private.h"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(smpi_pmpi, smpi, "Logging specific to SMPI (pmpi)");

//this function need to be here because of the calls to smpi_bench
void TRACE_smpi_set_category(const char *category)
{
  //need to end bench otherwise categories for execution tasks are wrong
  smpi_bench_end();
  TRACE_internal_smpi_set_category (category);
  //begin bench after changing process's category
  smpi_bench_begin();
}

/* PMPI User level calls */

int PMPI_Init(int *argc, char ***argv)
{
  // PMPI_Init is call only one time by only by SMPI process
  int already_init;
  MPI_Initialized(&already_init);
  if(!(already_init)){
    smpi_process_init(argc, argv);
    smpi_process_mark_as_initialized();
    int rank = smpi_process_index();
    TRACE_smpi_init(rank);
    TRACE_smpi_computing_init(rank);
    instr_extra_data extra = xbt_new0(s_instr_extra_data_t,1);
    extra->type = TRACING_INIT;
    TRACE_smpi_collective_in(rank, -1, __FUNCTION__, extra);
    TRACE_smpi_collective_out(rank, -1, __FUNCTION__);
    smpi_bench_begin();
  }
  return MPI_SUCCESS;
}

int PMPI_Finalize(void)
{
  smpi_bench_end();
  int rank = smpi_process_index();
  instr_extra_data extra = xbt_new0(s_instr_extra_data_t,1);
  extra->type = TRACING_FINALIZE;
  TRACE_smpi_collective_in(rank, -1, __FUNCTION__, extra);

  smpi_process_finalize();

  TRACE_smpi_collective_out(rank, -1, __FUNCTION__);
  TRACE_smpi_finalize(smpi_process_index());
  smpi_process_destroy();
  return MPI_SUCCESS;
}

int PMPI_Finalized(int* flag)
{
  *flag=smpi_process_finalized();
  return MPI_SUCCESS;
}

int PMPI_Get_version (int *version,int *subversion){
  *version = MPI_VERSION;
  *subversion= MPI_SUBVERSION;
  return MPI_SUCCESS;
}

int PMPI_Get_library_version (char *version,int *len){
  int retval = MPI_SUCCESS;
  smpi_bench_end();
  snprintf(version,MPI_MAX_LIBRARY_VERSION_STRING,"SMPI Version %d.%d. Copyright The Simgrid Team 2007-2015",
           SIMGRID_VERSION_MAJOR, SIMGRID_VERSION_MINOR);
  *len = strlen(version) > MPI_MAX_LIBRARY_VERSION_STRING ? MPI_MAX_LIBRARY_VERSION_STRING : strlen(version);
  smpi_bench_begin();
  return retval;
}

int PMPI_Init_thread(int *argc, char ***argv, int required, int *provided)
{
  if (provided != NULL) {
    *provided = MPI_THREAD_SINGLE;
  }
  return MPI_Init(argc, argv);
}

int PMPI_Query_thread(int *provided)
{
  int retval = 0;

  if (provided == NULL) {
    retval = MPI_ERR_ARG;
  } else {
    *provided = MPI_THREAD_SINGLE;
    retval = MPI_SUCCESS;
  }
  return retval;
}

int PMPI_Is_thread_main(int *flag)
{
  int retval = 0;

  if (flag == NULL) {
    retval = MPI_ERR_ARG;
  } else {
    *flag = smpi_process_index() == 0;
    retval = MPI_SUCCESS;
  }
  return retval;
}

int PMPI_Abort(MPI_Comm comm, int errorcode)
{
  smpi_bench_end();
  smpi_process_destroy();
  // FIXME: should kill all processes in comm instead
  simcall_process_kill(SIMIX_process_self());
  return MPI_SUCCESS;
}

double PMPI_Wtime(void)
{
  return smpi_mpi_wtime();
}

extern double sg_maxmin_precision;
double PMPI_Wtick(void)
{
  return sg_maxmin_precision;
}

int PMPI_Address(void *location, MPI_Aint * address)
{
  int retval = 0;

  if (!address) {
    retval = MPI_ERR_ARG;
  } else {
    *address = (MPI_Aint) location;
    retval = MPI_SUCCESS;
  }
  return retval;
}

int PMPI_Get_address(void *location, MPI_Aint * address)
{
  return PMPI_Address(location, address);
}

int PMPI_Type_free(MPI_Datatype * datatype)
{
  int retval = 0;
  /* Free a predefined datatype is an error according to the standard, and should be checked for */
  if (*datatype == MPI_DATATYPE_NULL) {
    retval = MPI_ERR_ARG;
  } else {
    smpi_datatype_free(datatype);
    retval = MPI_SUCCESS;
  }
  return retval;
}

int PMPI_Type_size(MPI_Datatype datatype, int *size)
{
  int retval = 0;

  if (datatype == MPI_DATATYPE_NULL) {
    retval = MPI_ERR_TYPE;
  } else if (size == NULL) {
    retval = MPI_ERR_ARG;
  } else {
    *size = (int) smpi_datatype_size(datatype);
    retval = MPI_SUCCESS;
  }
  return retval;
}

int PMPI_Type_get_extent(MPI_Datatype datatype, MPI_Aint * lb, MPI_Aint * extent)
{
  int retval = 0;

  if (datatype == MPI_DATATYPE_NULL) {
    retval = MPI_ERR_TYPE;
  } else if (lb == NULL || extent == NULL) {
    retval = MPI_ERR_ARG;
  } else {
    retval = smpi_datatype_extent(datatype, lb, extent);
  }
  return retval;
}

int PMPI_Type_get_true_extent(MPI_Datatype datatype, MPI_Aint * lb, MPI_Aint * extent)
{
  return PMPI_Type_get_extent(datatype, lb, extent);
}

int PMPI_Type_extent(MPI_Datatype datatype, MPI_Aint * extent)
{
  int retval = 0;

  if (datatype == MPI_DATATYPE_NULL) {
    retval = MPI_ERR_TYPE;
  } else if (extent == NULL) {
    retval = MPI_ERR_ARG;
  } else {
    *extent = smpi_datatype_get_extent(datatype);
    retval = MPI_SUCCESS;
  }
  return retval;
}

int PMPI_Type_lb(MPI_Datatype datatype, MPI_Aint * disp)
{
  int retval = 0;

  if (datatype == MPI_DATATYPE_NULL) {
    retval = MPI_ERR_TYPE;
  } else if (disp == NULL) {
    retval = MPI_ERR_ARG;
  } else {
    *disp = smpi_datatype_lb(datatype);
    retval = MPI_SUCCESS;
  }
  return retval;
}

int PMPI_Type_ub(MPI_Datatype datatype, MPI_Aint * disp)
{
  int retval = 0;

  if (datatype == MPI_DATATYPE_NULL) {
    retval = MPI_ERR_TYPE;
  } else if (disp == NULL) {
    retval = MPI_ERR_ARG;
  } else {
    *disp = smpi_datatype_ub(datatype);
    retval = MPI_SUCCESS;
  }
  return retval;
}

int PMPI_Type_dup(MPI_Datatype datatype, MPI_Datatype *newtype){
  int retval = 0;

  if (datatype == MPI_DATATYPE_NULL) {
    retval = MPI_ERR_TYPE;
  } else {
    retval = smpi_datatype_dup(datatype, newtype);
  }
  return retval;
}

int PMPI_Op_create(MPI_User_function * function, int commute, MPI_Op * op)
{
  int retval = 0;

  if (function == NULL || op == NULL) {
    retval = MPI_ERR_ARG;
  } else {
    *op = smpi_op_new(function, commute);
    retval = MPI_SUCCESS;
  }
  return retval;
}

int PMPI_Op_free(MPI_Op * op)
{
  int retval = 0;

  if (op == NULL) {
    retval = MPI_ERR_ARG;
  } else if (*op == MPI_OP_NULL) {
    retval = MPI_ERR_OP;
  } else {
    smpi_op_destroy(*op);
    *op = MPI_OP_NULL;
    retval = MPI_SUCCESS;
  }
  return retval;
}

int PMPI_Group_free(MPI_Group * group)
{
  int retval = 0;

  if (group == NULL) {
    retval = MPI_ERR_ARG;
  } else {
    smpi_group_destroy(*group);
    *group = MPI_GROUP_NULL;
    retval = MPI_SUCCESS;
  }
  return retval;
}

int PMPI_Group_size(MPI_Group group, int *size)
{
  int retval = 0;

  if (group == MPI_GROUP_NULL) {
    retval = MPI_ERR_GROUP;
  } else if (size == NULL) {
    retval = MPI_ERR_ARG;
  } else {
    *size = smpi_group_size(group);
    retval = MPI_SUCCESS;
  }
  return retval;
}

int PMPI_Group_rank(MPI_Group group, int *rank)
{
  int retval = 0;

  if (group == MPI_GROUP_NULL) {
    retval = MPI_ERR_GROUP;
  } else if (rank == NULL) {
    retval = MPI_ERR_ARG;
  } else {
    *rank = smpi_group_rank(group, smpi_process_index());
    retval = MPI_SUCCESS;
  }
  return retval;
}

int PMPI_Group_translate_ranks(MPI_Group group1, int n, int *ranks1, MPI_Group group2, int *ranks2)
{
  int retval, i, index;
  if (group1 == MPI_GROUP_NULL || group2 == MPI_GROUP_NULL) {
    retval = MPI_ERR_GROUP;
  } else {
    for (i = 0; i < n; i++) {
      if(ranks1[i]==MPI_PROC_NULL){
        ranks2[i]=MPI_PROC_NULL;
      }else{
        index = smpi_group_index(group1, ranks1[i]);
        ranks2[i] = smpi_group_rank(group2, index);
      }
    }
    retval = MPI_SUCCESS;
  }
  return retval;
}

int PMPI_Group_compare(MPI_Group group1, MPI_Group group2, int *result)
{
  int retval = 0;

  if (group1 == MPI_GROUP_NULL || group2 == MPI_GROUP_NULL) {
    retval = MPI_ERR_GROUP;
  } else if (result == NULL) {
    retval = MPI_ERR_ARG;
  } else {
    *result = smpi_group_compare(group1, group2);
    retval = MPI_SUCCESS;
  }
  return retval;
}

int PMPI_Group_union(MPI_Group group1, MPI_Group group2, MPI_Group * newgroup)
{
  int retval, i, proc1, proc2, size, size2;

  if (group1 == MPI_GROUP_NULL || group2 == MPI_GROUP_NULL) {
    retval = MPI_ERR_GROUP;
  } else if (newgroup == NULL) {
    retval = MPI_ERR_ARG;
  } else {
    size = smpi_group_size(group1);
    size2 = smpi_group_size(group2);
    for (i = 0; i < size2; i++) {
      proc2 = smpi_group_index(group2, i);
      proc1 = smpi_group_rank(group1, proc2);
      if (proc1 == MPI_UNDEFINED) {
        size++;
      }
    }
    if (size == 0) {
      *newgroup = MPI_GROUP_EMPTY;
    } else {
      *newgroup = smpi_group_new(size);
      size2 = smpi_group_size(group1);
      for (i = 0; i < size2; i++) {
        proc1 = smpi_group_index(group1, i);
        smpi_group_set_mapping(*newgroup, proc1, i);
      }
      for (i = size2; i < size; i++) {
        proc2 = smpi_group_index(group2, i - size2);
        smpi_group_set_mapping(*newgroup, proc2, i);
      }
    }
    retval = MPI_SUCCESS;
  }
  return retval;
}

int PMPI_Group_intersection(MPI_Group group1, MPI_Group group2, MPI_Group * newgroup)
{
  int retval, i, proc1, proc2, size;

  if (group1 == MPI_GROUP_NULL || group2 == MPI_GROUP_NULL) {
    retval = MPI_ERR_GROUP;
  } else if (newgroup == NULL) {
    retval = MPI_ERR_ARG;
  } else {
    size = smpi_group_size(group2);
    for (i = 0; i < size; i++) {
      proc2 = smpi_group_index(group2, i);
      proc1 = smpi_group_rank(group1, proc2);
      if (proc1 == MPI_UNDEFINED) {
        size--;
      }
    }
    if (size == 0) {
      *newgroup = MPI_GROUP_EMPTY;
    } else {
      *newgroup = smpi_group_new(size);
      int j=0;
      for (i = 0; i < smpi_group_size(group2); i++) {
        proc2 = smpi_group_index(group2, i);
        proc1 = smpi_group_rank(group1, proc2);
        if (proc1 != MPI_UNDEFINED) {
          smpi_group_set_mapping(*newgroup, proc2, j);
          j++;
        }
      }
    }
    retval = MPI_SUCCESS;
  }
  return retval;
}

int PMPI_Group_difference(MPI_Group group1, MPI_Group group2, MPI_Group * newgroup)
{
  int retval, i, proc1, proc2, size, size2;

  if (group1 == MPI_GROUP_NULL || group2 == MPI_GROUP_NULL) {
    retval = MPI_ERR_GROUP;
  } else if (newgroup == NULL) {
    retval = MPI_ERR_ARG;
  } else {
    size = size2 = smpi_group_size(group1);
    for (i = 0; i < size2; i++) {
      proc1 = smpi_group_index(group1, i);
      proc2 = smpi_group_rank(group2, proc1);
      if (proc2 != MPI_UNDEFINED) {
        size--;
      }
    }
    if (size == 0) {
      *newgroup = MPI_GROUP_EMPTY;
    } else {
      *newgroup = smpi_group_new(size);
      for (i = 0; i < size2; i++) {
        proc1 = smpi_group_index(group1, i);
        proc2 = smpi_group_rank(group2, proc1);
        if (proc2 == MPI_UNDEFINED) {
          smpi_group_set_mapping(*newgroup, proc1, i);
        }
      }
    }
    retval = MPI_SUCCESS;
  }
  return retval;
}

int PMPI_Group_incl(MPI_Group group, int n, int *ranks, MPI_Group * newgroup)
{
  int retval;

  if (group == MPI_GROUP_NULL) {
    retval = MPI_ERR_GROUP;
  } else if (newgroup == NULL) {
    retval = MPI_ERR_ARG;
  } else {
    retval = smpi_group_incl(group, n, ranks, newgroup);
  }
  return retval;
}

int PMPI_Group_excl(MPI_Group group, int n, int *ranks, MPI_Group * newgroup)
{
  int retval, i, j, newsize, oldsize, index;

  if (group == MPI_GROUP_NULL) {
    retval = MPI_ERR_GROUP;
  } else if (newgroup == NULL) {
    retval = MPI_ERR_ARG;
  } else {
    if (n == 0) {
      *newgroup = group;
      if(group!= smpi_comm_group(MPI_COMM_WORLD) && group != MPI_GROUP_NULL
                && group != smpi_comm_group(MPI_COMM_SELF) && group != MPI_GROUP_EMPTY)
      smpi_group_use(group);
    } else if (n == smpi_group_size(group)) {
      *newgroup = MPI_GROUP_EMPTY;
    } else {
      oldsize=smpi_group_size(group);
      newsize = oldsize - n;
      *newgroup = smpi_group_new(newsize);

      int* to_exclude=xbt_new0(int, smpi_group_size(group));
      for(i=0; i<oldsize; i++)
        to_exclude[i]=0;
      for(i=0; i<n; i++)
        to_exclude[ranks[i]]=1;

      j=0;
      for(i=0; i<oldsize; i++){
        if(to_exclude[i]==0){
          index = smpi_group_index(group, i);
          smpi_group_set_mapping(*newgroup, index, j);
          j++;
        }
      }

      xbt_free(to_exclude);
    }
    retval = MPI_SUCCESS;
  }
  return retval;
}

int PMPI_Group_range_incl(MPI_Group group, int n, int ranges[][3], MPI_Group * newgroup)
{
  int retval, i, j, rank, size, index;

  if (group == MPI_GROUP_NULL) {
    retval = MPI_ERR_GROUP;
  } else if (newgroup == NULL) {
    retval = MPI_ERR_ARG;
  } else {
    if (n == 0) {
      *newgroup = MPI_GROUP_EMPTY;
    } else {
      size = 0;
      for (i = 0; i < n; i++) {
        for (rank = ranges[i][0];       /* First */
             rank >= 0 && rank < smpi_group_size(group); /* Last */
              ) {
          size++;
          if(rank == ranges[i][1]){/*already last ?*/
            break;
          }
          rank += ranges[i][2]; /* Stride */
    if (ranges[i][0]<ranges[i][1]){
        if(rank > ranges[i][1])
          break;
    }else{
        if(rank < ranges[i][1])
          break;
    }
        }
      }

      *newgroup = smpi_group_new(size);
      j = 0;
      for (i = 0; i < n; i++) {
        for (rank = ranges[i][0];     /* First */
             rank >= 0 && rank < smpi_group_size(group); /* Last */
             ) {
          index = smpi_group_index(group, rank);
          smpi_group_set_mapping(*newgroup, index, j);
          j++;
          if(rank == ranges[i][1]){/*already last ?*/
            break;
          }
          rank += ranges[i][2]; /* Stride */
    if (ranges[i][0]<ranges[i][1]){
      if(rank > ranges[i][1])
        break;
    }else{
      if(rank < ranges[i][1])
        break;
    }
        }
      }
    }
    retval = MPI_SUCCESS;
  }
  return retval;
}

int PMPI_Group_range_excl(MPI_Group group, int n, int ranges[][3], MPI_Group * newgroup)
{
  int retval, i, rank, newrank,oldrank, size, index, add;

  if (group == MPI_GROUP_NULL) {
    retval = MPI_ERR_GROUP;
  } else if (newgroup == NULL) {
    retval = MPI_ERR_ARG;
  } else {
    if (n == 0) {
      *newgroup = group;
      if(group!= smpi_comm_group(MPI_COMM_WORLD) && group != MPI_GROUP_NULL
                && group != smpi_comm_group(MPI_COMM_SELF) && group != MPI_GROUP_EMPTY)
      smpi_group_use(group);
    } else {
      size = smpi_group_size(group);
      for (i = 0; i < n; i++) {
        for (rank = ranges[i][0];       /* First */
             rank >= 0 && rank < smpi_group_size(group); /* Last */
              ) {
          size--;
          if(rank == ranges[i][1]){/*already last ?*/
            break;
          }
          rank += ranges[i][2]; /* Stride */
    if (ranges[i][0]<ranges[i][1]){
        if(rank > ranges[i][1])
          break;
    }else{
        if(rank < ranges[i][1])
          break;
    }
        }
      }
      if (size == 0) {
        *newgroup = MPI_GROUP_EMPTY;
      } else {
        *newgroup = smpi_group_new(size);
        newrank=0;
        oldrank=0;
        while (newrank < size) {
          add=1;
          for (i = 0; i < n; i++) {
            for (rank = ranges[i][0];
                rank >= 0 && rank < smpi_group_size(group);
                ){
              if(rank==oldrank){
                  add=0;
                  break;
              }
              if(rank == ranges[i][1]){/*already last ?*/
                break;
              }
              rank += ranges[i][2]; /* Stride */
              if (ranges[i][0]<ranges[i][1]){
                  if(rank > ranges[i][1])
                    break;
              }else{
                  if(rank < ranges[i][1])
                    break;
              }
            }
          }
          if(add==1){
            index = smpi_group_index(group, oldrank);
            smpi_group_set_mapping(*newgroup, index, newrank);
            newrank++;
          }
          oldrank++;
        }
      }
    }

    retval = MPI_SUCCESS;
  }
  return retval;
}

int PMPI_Comm_rank(MPI_Comm comm, int *rank)
{
  int retval = 0;
  if (comm == MPI_COMM_NULL) {
    retval = MPI_ERR_COMM;
  } else if (rank == NULL) {
    retval = MPI_ERR_ARG;
  } else {
    *rank = smpi_comm_rank(comm);
    retval = MPI_SUCCESS;
  }
  return retval;
}

int PMPI_Comm_size(MPI_Comm comm, int *size)
{
  int retval = 0;
  if (comm == MPI_COMM_NULL) {
    retval = MPI_ERR_COMM;
  } else if (size == NULL) {
    retval = MPI_ERR_ARG;
  } else {
    *size = smpi_comm_size(comm);
    retval = MPI_SUCCESS;
  }
  return retval;
}

int PMPI_Comm_get_name (MPI_Comm comm, char* name, int* len)
{
  int retval = 0;

  if (comm == MPI_COMM_NULL)  {
    retval = MPI_ERR_COMM;
  } else if (name == NULL || len == NULL)  {
    retval = MPI_ERR_ARG;
  } else {
    smpi_comm_get_name(comm, name, len);
    retval = MPI_SUCCESS;
  }
  return retval;
}

int PMPI_Comm_group(MPI_Comm comm, MPI_Group * group)
{
  int retval = 0;

  if (comm == MPI_COMM_NULL) {
    retval = MPI_ERR_COMM;
  } else if (group == NULL) {
    retval = MPI_ERR_ARG;
  } else {
    *group = smpi_comm_group(comm);
    if(*group!= smpi_comm_group(MPI_COMM_WORLD) && *group != MPI_GROUP_NULL
              && *group != smpi_comm_group(MPI_COMM_SELF) && *group != MPI_GROUP_EMPTY)
    smpi_group_use(*group);
    retval = MPI_SUCCESS;
  }
  return retval;
}

int PMPI_Comm_compare(MPI_Comm comm1, MPI_Comm comm2, int *result)
{
  int retval = 0;

  if (comm1 == MPI_COMM_NULL || comm2 == MPI_COMM_NULL) {
    retval = MPI_ERR_COMM;
  } else if (result == NULL) {
    retval = MPI_ERR_ARG;
  } else {
    if (comm1 == comm2) {       /* Same communicators means same groups */
      *result = MPI_IDENT;
    } else {
      *result = smpi_group_compare(smpi_comm_group(comm1), smpi_comm_group(comm2));
      if (*result == MPI_IDENT) {
        *result = MPI_CONGRUENT;
      }
    }
    retval = MPI_SUCCESS;
  }
  return retval;
}

int PMPI_Comm_dup(MPI_Comm comm, MPI_Comm * newcomm)
{
  int retval = 0;

  if (comm == MPI_COMM_NULL) {
    retval = MPI_ERR_COMM;
  } else if (newcomm == NULL) {
    retval = MPI_ERR_ARG;
  } else {
    retval = smpi_comm_dup(comm, newcomm);
  }
  return retval;
}

int PMPI_Comm_create(MPI_Comm comm, MPI_Group group, MPI_Comm * newcomm)
{
  int retval = 0;

  if (comm == MPI_COMM_NULL) {
    retval = MPI_ERR_COMM;
  } else if (group == MPI_GROUP_NULL) {
    retval = MPI_ERR_GROUP;
  } else if (newcomm == NULL) {
    retval = MPI_ERR_ARG;
  } else if(smpi_group_rank(group,smpi_process_index())==MPI_UNDEFINED){
    *newcomm= MPI_COMM_NULL;
    retval = MPI_SUCCESS;
  }else{

    *newcomm = smpi_comm_new(group, NULL);
    retval = MPI_SUCCESS;
  }
  return retval;
}

int PMPI_Comm_free(MPI_Comm * comm)
{
  int retval = 0;

  if (comm == NULL) {
    retval = MPI_ERR_ARG;
  } else if (*comm == MPI_COMM_NULL) {
    retval = MPI_ERR_COMM;
  } else {
    smpi_comm_destroy(*comm);
    *comm = MPI_COMM_NULL;
    retval = MPI_SUCCESS;
  }
  return retval;
}

int PMPI_Comm_disconnect(MPI_Comm * comm)
{
  /* TODO: wait until all communication in comm are done */
  int retval = 0;

  if (comm == NULL) {
    retval = MPI_ERR_ARG;
  } else if (*comm == MPI_COMM_NULL) {
    retval = MPI_ERR_COMM;
  } else {
    smpi_comm_destroy(*comm);
    *comm = MPI_COMM_NULL;
    retval = MPI_SUCCESS;
  }
  return retval;
}

int PMPI_Comm_split(MPI_Comm comm, int color, int key, MPI_Comm* comm_out)
{
  int retval = 0;
  smpi_bench_end();

  if (comm_out == NULL) {
    retval = MPI_ERR_ARG;
  } else if (comm == MPI_COMM_NULL) {
    retval = MPI_ERR_COMM;
  } else {
    *comm_out = smpi_comm_split(comm, color, key);
    retval = MPI_SUCCESS;
  }
  smpi_bench_begin();

  return retval;
}

int PMPI_Send_init(void *buf, int count, MPI_Datatype datatype, int dst, int tag, MPI_Comm comm, MPI_Request * request)
{
  int retval = 0;

  smpi_bench_end();
  if (request == NULL) {
      retval = MPI_ERR_ARG;
  } else if (comm == MPI_COMM_NULL) {
      retval = MPI_ERR_COMM;
  } else if (!is_datatype_valid(datatype)) {
      retval = MPI_ERR_TYPE;
  } else if (dst == MPI_PROC_NULL) {
      retval = MPI_SUCCESS;
  } else {
      *request = smpi_mpi_send_init(buf, count, datatype, dst, tag, comm);
      retval = MPI_SUCCESS;
  }
  smpi_bench_begin();
  if (retval != MPI_SUCCESS && request)
    *request = MPI_REQUEST_NULL;
  return retval;
}

int PMPI_Recv_init(void *buf, int count, MPI_Datatype datatype, int src, int tag, MPI_Comm comm, MPI_Request * request)
{
  int retval = 0;

  smpi_bench_end();
  if (request == NULL) {
    retval = MPI_ERR_ARG;
  } else if (comm == MPI_COMM_NULL) {
    retval = MPI_ERR_COMM;
  } else if (!is_datatype_valid(datatype)) {
      retval = MPI_ERR_TYPE;
  } else if (src == MPI_PROC_NULL) {
    retval = MPI_SUCCESS;
  } else {
    *request = smpi_mpi_recv_init(buf, count, datatype, src, tag, comm);
    retval = MPI_SUCCESS;
  }
  smpi_bench_begin();
  if (retval != MPI_SUCCESS && request)
    *request = MPI_REQUEST_NULL;
  return retval;
}

int PMPI_Ssend_init(void* buf, int count, MPI_Datatype datatype, int dst, int tag, MPI_Comm comm, MPI_Request* request)
{
  int retval = 0;

  smpi_bench_end();
  if (request == NULL) {
    retval = MPI_ERR_ARG;
  } else if (comm == MPI_COMM_NULL) {
    retval = MPI_ERR_COMM;
  } else if (!is_datatype_valid(datatype)) {
      retval = MPI_ERR_TYPE;
  } else if (dst == MPI_PROC_NULL) {
    retval = MPI_SUCCESS;
  } else {
    *request = smpi_mpi_ssend_init(buf, count, datatype, dst, tag, comm);
    retval = MPI_SUCCESS;
  }
  smpi_bench_begin();
  if (retval != MPI_SUCCESS && request)
    *request = MPI_REQUEST_NULL;
  return retval;
}

int PMPI_Start(MPI_Request * request)
{
  int retval = 0;

  smpi_bench_end();
  if (request == NULL || *request == MPI_REQUEST_NULL) {
    retval = MPI_ERR_REQUEST;
  } else {
    smpi_mpi_start(*request);
    retval = MPI_SUCCESS;
  }
  smpi_bench_begin();
  return retval;
}

int PMPI_Startall(int count, MPI_Request * requests)
{
  int retval;
  int i = 0;
  smpi_bench_end();
  if (requests == NULL) {
    retval = MPI_ERR_ARG;
  } else {
    retval = MPI_SUCCESS;
    for (i = 0 ;  i < count ; i++) {
      if(requests[i] == MPI_REQUEST_NULL) {
        retval = MPI_ERR_REQUEST;
      }
    }
    if(retval != MPI_ERR_REQUEST) {
      smpi_mpi_startall(count, requests);
    }
  }
  smpi_bench_begin();
  return retval;
}

int PMPI_Request_free(MPI_Request * request)
{
  int retval = 0;

  smpi_bench_end();
  if (*request == MPI_REQUEST_NULL) {
    retval = MPI_ERR_ARG;
  } else {
    smpi_mpi_request_free(request);
    retval = MPI_SUCCESS;
  }
  smpi_bench_begin();
  return retval;
}

int PMPI_Irecv(void *buf, int count, MPI_Datatype datatype, int src, int tag, MPI_Comm comm, MPI_Request * request)
{
  int retval = 0;

  smpi_bench_end();

  if (request == NULL) {
    retval = MPI_ERR_ARG;
  } else if (comm == MPI_COMM_NULL) {
    retval = MPI_ERR_COMM;
  } else if (src == MPI_PROC_NULL) {
    *request = MPI_REQUEST_NULL;
    retval = MPI_SUCCESS;
  } else if (src!=MPI_ANY_SOURCE && (src >= smpi_group_size(smpi_comm_group(comm)) || src <0)){
    retval = MPI_ERR_RANK;
  } else if (count < 0) {
    retval = MPI_ERR_COUNT;
  } else if (buf==NULL && count > 0) {
    retval = MPI_ERR_COUNT;
  } else if (!is_datatype_valid(datatype)) {
      retval = MPI_ERR_TYPE;
  } else if(tag<0 && tag !=  MPI_ANY_TAG){
    retval = MPI_ERR_TAG;
  } else {

    int rank = comm != MPI_COMM_NULL ? smpi_process_index() : -1;
    int src_traced = smpi_group_index(smpi_comm_group(comm), src);

    instr_extra_data extra = xbt_new0(s_instr_extra_data_t,1);
    extra->type = TRACING_IRECV;
    extra->src = src_traced;
    extra->dst = rank;
    int known=0;
    extra->datatype1 = encode_datatype(datatype, &known);
    int dt_size_send = 1;
    if(!known)
      dt_size_send = smpi_datatype_size(datatype);
    extra->send_size = count*dt_size_send;
    TRACE_smpi_ptp_in(rank, src_traced, rank, __FUNCTION__, extra);

    *request = smpi_mpi_irecv(buf, count, datatype, src, tag, comm);
    retval = MPI_SUCCESS;

    TRACE_smpi_ptp_out(rank, src_traced, rank, __FUNCTION__);
    (*request)->recv = 1;
  }

  smpi_bench_begin();
  if (retval != MPI_SUCCESS && request)
    *request = MPI_REQUEST_NULL;
  return retval;
}


int PMPI_Isend(void *buf, int count, MPI_Datatype datatype, int dst, int tag, MPI_Comm comm, MPI_Request * request)
{
  int retval = 0;

  smpi_bench_end();
  if (request == NULL) {
    retval = MPI_ERR_ARG;
  } else if (comm == MPI_COMM_NULL) {
    retval = MPI_ERR_COMM;
  } else if (dst == MPI_PROC_NULL) {
    *request = MPI_REQUEST_NULL;
    retval = MPI_SUCCESS;
  } else if (dst >= smpi_group_size(smpi_comm_group(comm)) || dst <0){
    retval = MPI_ERR_RANK;
  } else if (count < 0) {
    retval = MPI_ERR_COUNT;
  } else if (buf==NULL && count > 0) {
    retval = MPI_ERR_COUNT;
  } else if (!is_datatype_valid(datatype)) {
      retval = MPI_ERR_TYPE;
  } else if(tag<0 && tag !=  MPI_ANY_TAG){
    retval = MPI_ERR_TAG;
  } else {

    int rank = comm != MPI_COMM_NULL ? smpi_process_index() : -1;
    int dst_traced = smpi_group_index(smpi_comm_group(comm), dst);
    instr_extra_data extra = xbt_new0(s_instr_extra_data_t,1);
    extra->type = TRACING_ISEND;
    extra->src = rank;
    extra->dst = dst_traced;
    int known=0;
    extra->datatype1 = encode_datatype(datatype, &known);
    int dt_size_send = 1;
    if(!known)
      dt_size_send = smpi_datatype_size(datatype);
    extra->send_size = count*dt_size_send;
    TRACE_smpi_ptp_in(rank, rank, dst_traced, __FUNCTION__, extra);
    TRACE_smpi_send(rank, rank, dst_traced, count*smpi_datatype_size(datatype));

    *request = smpi_mpi_isend(buf, count, datatype, dst, tag, comm);
    retval = MPI_SUCCESS;

    TRACE_smpi_ptp_out(rank, rank, dst_traced, __FUNCTION__);
    (*request)->send = 1;
  }

  smpi_bench_begin();
  if (retval != MPI_SUCCESS && request)
    *request = MPI_REQUEST_NULL;
  return retval;
}

int PMPI_Issend(void* buf, int count, MPI_Datatype datatype, int dst, int tag, MPI_Comm comm, MPI_Request* request)
{
  int retval = 0;

  smpi_bench_end();
  if (request == NULL) {
    retval = MPI_ERR_ARG;
  } else if (comm == MPI_COMM_NULL) {
    retval = MPI_ERR_COMM;
  } else if (dst == MPI_PROC_NULL) {
    *request = MPI_REQUEST_NULL;
    retval = MPI_SUCCESS;
  } else if (dst >= smpi_group_size(smpi_comm_group(comm)) || dst <0){
    retval = MPI_ERR_RANK;
  } else if (count < 0) {
    retval = MPI_ERR_COUNT;
  } else if (buf==NULL && count > 0) {
    retval = MPI_ERR_COUNT;
  } else if (!is_datatype_valid(datatype)) {
      retval = MPI_ERR_TYPE;
  } else if(tag<0 && tag !=  MPI_ANY_TAG){
    retval = MPI_ERR_TAG;
  } else {

    int rank = comm != MPI_COMM_NULL ? smpi_process_index() : -1;
    int dst_traced = smpi_group_index(smpi_comm_group(comm), dst);
    instr_extra_data extra = xbt_new0(s_instr_extra_data_t,1);
    extra->type = TRACING_ISSEND;
    extra->src = rank;
    extra->dst = dst_traced;
    int known=0;
    extra->datatype1 = encode_datatype(datatype, &known);
    int dt_size_send = 1;
    if(!known)
      dt_size_send = smpi_datatype_size(datatype);
    extra->send_size = count*dt_size_send;
    TRACE_smpi_ptp_in(rank, rank, dst_traced, __FUNCTION__, extra);
    TRACE_smpi_send(rank, rank, dst_traced, count*smpi_datatype_size(datatype));

    *request = smpi_mpi_issend(buf, count, datatype, dst, tag, comm);
    retval = MPI_SUCCESS;

    TRACE_smpi_ptp_out(rank, rank, dst_traced, __FUNCTION__);
    (*request)->send = 1;
  }

  smpi_bench_begin();
  if (retval != MPI_SUCCESS && request)
    *request = MPI_REQUEST_NULL;
  return retval;
}

int PMPI_Recv(void *buf, int count, MPI_Datatype datatype, int src, int tag, MPI_Comm comm, MPI_Status * status)
{
  int retval = 0;

  smpi_bench_end();
  if (comm == MPI_COMM_NULL) {
    retval = MPI_ERR_COMM;
  } else if (src == MPI_PROC_NULL) {
    smpi_empty_status(status);
    status->MPI_SOURCE = MPI_PROC_NULL;
    retval = MPI_SUCCESS;
  } else if (src!=MPI_ANY_SOURCE && (src >= smpi_group_size(smpi_comm_group(comm)) || src <0)){
    retval = MPI_ERR_RANK;
  } else if (count < 0) {
    retval = MPI_ERR_COUNT;
  } else if (buf==NULL && count > 0) {
    retval = MPI_ERR_COUNT;
  } else if (!is_datatype_valid(datatype)) {
      retval = MPI_ERR_TYPE;
  } else if(tag<0 && tag !=  MPI_ANY_TAG){
    retval = MPI_ERR_TAG;
  } else {
  int rank = comm != MPI_COMM_NULL ? smpi_process_index() : -1;
  int src_traced = smpi_group_index(smpi_comm_group(comm), src);
  instr_extra_data extra = xbt_new0(s_instr_extra_data_t,1);
  extra->type = TRACING_RECV;
  extra->src = src_traced;
  extra->dst = rank;
  int known=0;
  extra->datatype1 = encode_datatype(datatype, &known);
  int dt_size_send = 1;
  if(!known)
    dt_size_send = smpi_datatype_size(datatype);
  extra->send_size = count*dt_size_send;
  TRACE_smpi_ptp_in(rank, src_traced, rank, __FUNCTION__, extra);

    smpi_mpi_recv(buf, count, datatype, src, tag, comm, status);
    retval = MPI_SUCCESS;

  //the src may not have been known at the beginning of the recv (MPI_ANY_SOURCE)
  if(status!=MPI_STATUS_IGNORE){
    src_traced = smpi_group_index(smpi_comm_group(comm), status->MPI_SOURCE);
    if (!TRACE_smpi_view_internals()) {
      TRACE_smpi_recv(rank, src_traced, rank);
    }
  }
  TRACE_smpi_ptp_out(rank, src_traced, rank, __FUNCTION__);
  }

  smpi_bench_begin();
  return retval;
}

int PMPI_Send(void *buf, int count, MPI_Datatype datatype, int dst, int tag, MPI_Comm comm)
{
  int retval = 0;

  smpi_bench_end();

  if (comm == MPI_COMM_NULL) {
    retval = MPI_ERR_COMM;
  } else if (dst == MPI_PROC_NULL) {
    retval = MPI_SUCCESS;
  } else if (dst >= smpi_group_size(smpi_comm_group(comm)) || dst <0){
    retval = MPI_ERR_RANK;
  } else if (count < 0) {
    retval = MPI_ERR_COUNT;
  } else if (buf==NULL && count > 0) {
    retval = MPI_ERR_COUNT;
  } else if (!is_datatype_valid(datatype)) {
      retval = MPI_ERR_TYPE;
  } else if(tag<0 && tag !=  MPI_ANY_TAG){
    retval = MPI_ERR_TAG;
  } else {

  int rank = comm != MPI_COMM_NULL ? smpi_process_index() : -1;
  int dst_traced = smpi_group_index(smpi_comm_group(comm), dst);
  instr_extra_data extra = xbt_new0(s_instr_extra_data_t,1);
  extra->type = TRACING_SEND;
  extra->src = rank;
  extra->dst = dst_traced;
  int known=0;
  extra->datatype1 = encode_datatype(datatype, &known);
  int dt_size_send = 1;
  if(!known)
    dt_size_send = smpi_datatype_size(datatype);
  extra->send_size = count*dt_size_send;
  TRACE_smpi_ptp_in(rank, rank, dst_traced, __FUNCTION__, extra);
  if (!TRACE_smpi_view_internals()) {
    TRACE_smpi_send(rank, rank, dst_traced,count*smpi_datatype_size(datatype));
  }

    smpi_mpi_send(buf, count, datatype, dst, tag, comm);
    retval = MPI_SUCCESS;

  TRACE_smpi_ptp_out(rank, rank, dst_traced, __FUNCTION__);
  }

  smpi_bench_begin();
  return retval;
}



int PMPI_Ssend(void* buf, int count, MPI_Datatype datatype, int dst, int tag, MPI_Comm comm) {
  int retval = 0;

   smpi_bench_end();

   if (comm == MPI_COMM_NULL) {
     retval = MPI_ERR_COMM;
   } else if (dst == MPI_PROC_NULL) {
     retval = MPI_SUCCESS;
   } else if (dst >= smpi_group_size(smpi_comm_group(comm)) || dst <0){
     retval = MPI_ERR_RANK;
   } else if (count < 0) {
     retval = MPI_ERR_COUNT;
   } else if (buf==NULL && count > 0) {
     retval = MPI_ERR_COUNT;
   } else if (!is_datatype_valid(datatype)){
     retval = MPI_ERR_TYPE;
   } else if(tag<0 && tag !=  MPI_ANY_TAG){
     retval = MPI_ERR_TAG;
   } else {

   int rank = comm != MPI_COMM_NULL ? smpi_process_index() : -1;
   int dst_traced = smpi_group_index(smpi_comm_group(comm), dst);
   instr_extra_data extra = xbt_new0(s_instr_extra_data_t,1);
   extra->type = TRACING_SSEND;
   extra->src = rank;
   extra->dst = dst_traced;
   int known=0;
   extra->datatype1 = encode_datatype(datatype, &known);
   int dt_size_send = 1;
   if(!known)
     dt_size_send = smpi_datatype_size(datatype);
   extra->send_size = count*dt_size_send;
   TRACE_smpi_ptp_in(rank, rank, dst_traced, __FUNCTION__, extra);
   TRACE_smpi_send(rank, rank, dst_traced,count*smpi_datatype_size(datatype));

     smpi_mpi_ssend(buf, count, datatype, dst, tag, comm);
     retval = MPI_SUCCESS;

   TRACE_smpi_ptp_out(rank, rank, dst_traced, __FUNCTION__);
   }

   smpi_bench_begin();
   return retval;}

int PMPI_Sendrecv(void *sendbuf, int sendcount, MPI_Datatype sendtype, int dst, int sendtag, void *recvbuf,
                 int recvcount, MPI_Datatype recvtype, int src, int recvtag, MPI_Comm comm, MPI_Status * status)
{
  int retval = 0;

  smpi_bench_end();

  if (comm == MPI_COMM_NULL) {
    retval = MPI_ERR_COMM;
  } else if (!is_datatype_valid(sendtype)
             || !is_datatype_valid(recvtype)) {
    retval = MPI_ERR_TYPE;
  } else if (src == MPI_PROC_NULL || dst == MPI_PROC_NULL) {
      smpi_empty_status(status);
      status->MPI_SOURCE = MPI_PROC_NULL;
      retval = MPI_SUCCESS;
  }else if (dst >= smpi_group_size(smpi_comm_group(comm)) || dst <0 ||
      (src!=MPI_ANY_SOURCE && (src >= smpi_group_size(smpi_comm_group(comm)) || src <0))){
    retval = MPI_ERR_RANK;
  } else if (sendcount < 0 || recvcount<0) {
      retval = MPI_ERR_COUNT;
  } else if ((sendbuf==NULL && sendcount > 0)||(recvbuf==NULL && recvcount>0)) {
    retval = MPI_ERR_COUNT;
  } else if((sendtag<0 && sendtag !=  MPI_ANY_TAG)||(recvtag<0 && recvtag != MPI_ANY_TAG)){
    retval = MPI_ERR_TAG;
  } else {

  int rank = comm != MPI_COMM_NULL ? smpi_process_index() : -1;
  int dst_traced = smpi_group_index(smpi_comm_group(comm), dst);
  int src_traced = smpi_group_index(smpi_comm_group(comm), src);
  instr_extra_data extra = xbt_new0(s_instr_extra_data_t,1);
  extra->type = TRACING_SENDRECV;
  extra->src = src_traced;
  extra->dst = dst_traced;
  int known=0;
  extra->datatype1 = encode_datatype(sendtype, &known);
  int dt_size_send = 1;
  if(!known)
    dt_size_send = smpi_datatype_size(sendtype);
  extra->send_size = sendcount*dt_size_send;
  extra->datatype2 = encode_datatype(recvtype, &known);
  int dt_size_recv = 1;
  if(!known)
    dt_size_recv = smpi_datatype_size(recvtype);
  extra->recv_size = recvcount*dt_size_recv;

  TRACE_smpi_ptp_in(rank, src_traced, dst_traced, __FUNCTION__, extra);
  TRACE_smpi_send(rank, rank, dst_traced,sendcount*smpi_datatype_size(sendtype));

    smpi_mpi_sendrecv(sendbuf, sendcount, sendtype, dst, sendtag, recvbuf,
                      recvcount, recvtype, src, recvtag, comm, status);
    retval = MPI_SUCCESS;

  TRACE_smpi_ptp_out(rank, src_traced, dst_traced, __FUNCTION__);
  TRACE_smpi_recv(rank, src_traced, rank);
  }

  smpi_bench_begin();
  return retval;
}

int PMPI_Sendrecv_replace(void *buf, int count, MPI_Datatype datatype, int dst, int sendtag, int src, int recvtag,
                         MPI_Comm comm, MPI_Status * status)
{
  //TODO: suboptimal implementation
  void *recvbuf;
  int retval = 0;
  if (!is_datatype_valid(datatype)) {
      retval = MPI_ERR_TYPE;
  } else if (count < 0) {
      retval = MPI_ERR_COUNT;
  } else {
    int size = smpi_datatype_get_extent(datatype) * count;
    recvbuf = xbt_new0(char, size);
    retval = MPI_Sendrecv(buf, count, datatype, dst, sendtag, recvbuf, count, datatype, src, recvtag, comm, status);
    if(retval==MPI_SUCCESS){
        smpi_datatype_copy(recvbuf, count, datatype, buf, count, datatype);
    }
    xbt_free(recvbuf);

  }
  return retval;
}

int PMPI_Test(MPI_Request * request, int *flag, MPI_Status * status)
{
  int retval = 0;
  smpi_bench_end();
  if (request == NULL || flag == NULL) {
    retval = MPI_ERR_ARG;
  } else if (*request == MPI_REQUEST_NULL) {
    *flag= true;
    smpi_empty_status(status);
    retval = MPI_SUCCESS;
  } else {
    int rank = request && (*request)->comm != MPI_COMM_NULL ? smpi_process_index() : -1;

    instr_extra_data extra = xbt_new0(s_instr_extra_data_t,1);
    extra->type = TRACING_TEST;
    TRACE_smpi_testing_in(rank, extra);

    *flag = smpi_mpi_test(request, status);

    TRACE_smpi_testing_out(rank);
    retval = MPI_SUCCESS;
  }
  smpi_bench_begin();
  return retval;
}

int PMPI_Testany(int count, MPI_Request requests[], int *index, int *flag, MPI_Status * status)
{
  int retval = 0;

  smpi_bench_end();
  if (index == NULL || flag == NULL) {
    retval = MPI_ERR_ARG;
  } else {
    *flag = smpi_mpi_testany(count, requests, index, status);
    retval = MPI_SUCCESS;
  }
  smpi_bench_begin();
  return retval;
}

int PMPI_Testall(int count, MPI_Request* requests, int* flag, MPI_Status* statuses)
{
  int retval = 0;

  smpi_bench_end();
  if (flag == NULL) {
    retval = MPI_ERR_ARG;
  } else {
    *flag = smpi_mpi_testall(count, requests, statuses);
    retval = MPI_SUCCESS;
  }
  smpi_bench_begin();
  return retval;
}

int PMPI_Probe(int source, int tag, MPI_Comm comm, MPI_Status* status) {
  int retval = 0;
  smpi_bench_end();

  if (status == NULL) {
    retval = MPI_ERR_ARG;
  } else if (comm == MPI_COMM_NULL) {
    retval = MPI_ERR_COMM;
  } else if (source == MPI_PROC_NULL) {
    smpi_empty_status(status);
    status->MPI_SOURCE = MPI_PROC_NULL;
    retval = MPI_SUCCESS;
  } else {
    smpi_mpi_probe(source, tag, comm, status);
    retval = MPI_SUCCESS;
  }
  smpi_bench_begin();
  return retval;
}

int PMPI_Iprobe(int source, int tag, MPI_Comm comm, int* flag, MPI_Status* status) {
  int retval = 0;
  smpi_bench_end();

  if (flag == NULL) {
    retval = MPI_ERR_ARG;
  } else if (status == NULL) {
    retval = MPI_ERR_ARG;
  } else if (comm == MPI_COMM_NULL) {
    retval = MPI_ERR_COMM;
  } else if (source == MPI_PROC_NULL) {
    *flag=true;
    smpi_empty_status(status);
    status->MPI_SOURCE = MPI_PROC_NULL;
    retval = MPI_SUCCESS;
  } else {
    smpi_mpi_iprobe(source, tag, comm, flag, status);
    retval = MPI_SUCCESS;
  }
  smpi_bench_begin();
  return retval;
}

int PMPI_Wait(MPI_Request * request, MPI_Status * status)
{
  int retval = 0;

  smpi_bench_end();

  smpi_empty_status(status);

  if (request == NULL) {
    retval = MPI_ERR_ARG;
  } else if (*request == MPI_REQUEST_NULL) {
    retval = MPI_SUCCESS;
  } else {

    int rank = request && (*request)->comm != MPI_COMM_NULL ? smpi_process_index() : -1;

    int src_traced = (*request)->src;
    int dst_traced = (*request)->dst;
    MPI_Comm comm = (*request)->comm;
    int is_wait_for_receive = (*request)->recv;
    instr_extra_data extra = xbt_new0(s_instr_extra_data_t,1);
    extra->type = TRACING_WAIT;
    TRACE_smpi_ptp_in(rank, src_traced, dst_traced, __FUNCTION__, extra);

    smpi_mpi_wait(request, status);
    retval = MPI_SUCCESS;

    //the src may not have been known at the beginning of the recv (MPI_ANY_SOURCE)
    TRACE_smpi_ptp_out(rank, src_traced, dst_traced, __FUNCTION__);
    if (is_wait_for_receive) {
      if(src_traced==MPI_ANY_SOURCE)
        src_traced = (status!=MPI_STATUS_IGNORE) ?
          smpi_group_rank(smpi_comm_group(comm), status->MPI_SOURCE) :
          src_traced;
      TRACE_smpi_recv(rank, src_traced, dst_traced);
    }
  }

  smpi_bench_begin();
  return retval;
}

int PMPI_Waitany(int count, MPI_Request requests[], int *index, MPI_Status * status)
{
  if (index == NULL)
    return MPI_ERR_ARG;

  smpi_bench_end();
  //save requests information for tracing
  int i;
  int *srcs = xbt_new0(int, count);
  int *dsts = xbt_new0(int, count);
  int *recvs = xbt_new0(int, count);
  MPI_Comm *comms = xbt_new0(MPI_Comm, count);

  for (i = 0; i < count; i++) {
    MPI_Request req = requests[i];      //already received requests are no longer valid
    if (req) {
      srcs[i] = req->src;
      dsts[i] = req->dst;
      recvs[i] = req->recv;
      comms[i] = req->comm;
    }
  }
  int rank_traced = smpi_process_index();
  instr_extra_data extra = xbt_new0(s_instr_extra_data_t,1);
  extra->type = TRACING_WAITANY;
  extra->send_size=count;
  TRACE_smpi_ptp_in(rank_traced, -1, -1, __FUNCTION__,extra);

  *index = smpi_mpi_waitany(count, requests, status);

  if(*index!=MPI_UNDEFINED){
    int src_traced = srcs[*index];
    //the src may not have been known at the beginning of the recv (MPI_ANY_SOURCE)
    int dst_traced = dsts[*index];
    int is_wait_for_receive = recvs[*index];
    if (is_wait_for_receive) {
      if(srcs[*index]==MPI_ANY_SOURCE)
        src_traced = (status!=MPI_STATUSES_IGNORE) ?
                      smpi_group_rank(smpi_comm_group(comms[*index]), status->MPI_SOURCE) : srcs[*index];
      TRACE_smpi_recv(rank_traced, src_traced, dst_traced);
    }
    TRACE_smpi_ptp_out(rank_traced, src_traced, dst_traced, __FUNCTION__);
    xbt_free(srcs);
    xbt_free(dsts);
    xbt_free(recvs);
    xbt_free(comms);

  }
  smpi_bench_begin();
  return MPI_SUCCESS;
}

int PMPI_Waitall(int count, MPI_Request requests[], MPI_Status status[])
{
  smpi_bench_end();
  //save information from requests
  int i;
  int *srcs = xbt_new0(int, count);
  int *dsts = xbt_new0(int, count);
  int *recvs = xbt_new0(int, count);
  int *valid = xbt_new0(int, count);
  MPI_Comm *comms = xbt_new0(MPI_Comm, count);

  //int valid_count = 0;
  for (i = 0; i < count; i++) {
    MPI_Request req = requests[i];
    if(req!=MPI_REQUEST_NULL){
      srcs[i] = req->src;
      dsts[i] = req->dst;
      recvs[i] = req->recv;
      comms[i] = req->comm;
      valid[i]=1;;
    }else{
      valid[i]=0;
    }
  }
  int rank_traced = smpi_process_index();
  instr_extra_data extra = xbt_new0(s_instr_extra_data_t,1);
  extra->type = TRACING_WAITALL;
  extra->send_size=count;
  TRACE_smpi_ptp_in(rank_traced, -1, -1, __FUNCTION__,extra);

  int retval = smpi_mpi_waitall(count, requests, status);

  for (i = 0; i < count; i++) {
    if(valid[i]){
    //int src_traced = srcs[*index];
    //the src may not have been known at the beginning of the recv (MPI_ANY_SOURCE)
      int src_traced = srcs[i];
      int dst_traced = dsts[i];
      int is_wait_for_receive = recvs[i];
      if (is_wait_for_receive) {
        if(src_traced==MPI_ANY_SOURCE)
        src_traced = (status!=MPI_STATUSES_IGNORE) ?
                          smpi_group_rank(smpi_comm_group(comms[i]), status[i].MPI_SOURCE) : srcs[i];
        TRACE_smpi_recv(rank_traced, src_traced, dst_traced);
      }
    }
  }
  TRACE_smpi_ptp_out(rank_traced, -1, -1, __FUNCTION__);
  xbt_free(srcs);
  xbt_free(dsts);
  xbt_free(recvs);
  xbt_free(valid);
  xbt_free(comms);

  smpi_bench_begin();
  return retval;
}

int PMPI_Waitsome(int incount, MPI_Request requests[], int *outcount, int *indices, MPI_Status status[])
{
  int retval = 0;

  smpi_bench_end();
  if (outcount == NULL) {
    retval = MPI_ERR_ARG;
  } else {
    *outcount = smpi_mpi_waitsome(incount, requests, indices, status);
    retval = MPI_SUCCESS;
  }
  smpi_bench_begin();
  return retval;
}

int PMPI_Testsome(int incount, MPI_Request requests[], int* outcount, int* indices, MPI_Status status[])
{
  int retval = 0;

   smpi_bench_end();
   if (outcount == NULL) {
     retval = MPI_ERR_ARG;
   } else {
     *outcount = smpi_mpi_testsome(incount, requests, indices, status);
     retval = MPI_SUCCESS;
   }
   smpi_bench_begin();
   return retval;
}


int PMPI_Bcast(void *buf, int count, MPI_Datatype datatype, int root, MPI_Comm comm)
{
  int retval = 0;

  smpi_bench_end();

  if (comm == MPI_COMM_NULL) {
    retval = MPI_ERR_COMM;
  } else if (!is_datatype_valid(datatype)) {
      retval = MPI_ERR_ARG;
  } else {
  int rank = comm != MPI_COMM_NULL ? smpi_process_index() : -1;
  int root_traced = smpi_group_index(smpi_comm_group(comm), root);

  instr_extra_data extra = xbt_new0(s_instr_extra_data_t,1);
  extra->type = TRACING_BCAST;
  extra->root = root_traced;
  int known=0;
  extra->datatype1 = encode_datatype(datatype, &known);
  int dt_size_send = 1;
  if(!known)
    dt_size_send = smpi_datatype_size(datatype);
  extra->send_size = count*dt_size_send;
  TRACE_smpi_collective_in(rank, root_traced, __FUNCTION__, extra);

    mpi_coll_bcast_fun(buf, count, datatype, root, comm);
    retval = MPI_SUCCESS;

    TRACE_smpi_collective_out(rank, root_traced, __FUNCTION__);
  }

  smpi_bench_begin();
  return retval;
}

int PMPI_Barrier(MPI_Comm comm)
{
  int retval = 0;

  smpi_bench_end();

  if (comm == MPI_COMM_NULL) {
    retval = MPI_ERR_COMM;
  } else {
  int rank = comm != MPI_COMM_NULL ? smpi_process_index() : -1;
  instr_extra_data extra = xbt_new0(s_instr_extra_data_t,1);
  extra->type = TRACING_BARRIER;
  TRACE_smpi_collective_in(rank, -1, __FUNCTION__, extra);

  mpi_coll_barrier_fun(comm);
  retval = MPI_SUCCESS;

  TRACE_smpi_collective_out(rank, -1, __FUNCTION__);
  }

  smpi_bench_begin();
  return retval;
}

int PMPI_Gather(void *sendbuf, int sendcount, MPI_Datatype sendtype,void *recvbuf, int recvcount, MPI_Datatype recvtype,
                int root, MPI_Comm comm)
{
  int retval = 0;

  smpi_bench_end();

  if (comm == MPI_COMM_NULL) {
    retval = MPI_ERR_COMM;
  } else if ((( sendbuf != MPI_IN_PLACE) && (sendtype == MPI_DATATYPE_NULL)) ||
            ((smpi_comm_rank(comm) == root) && (recvtype == MPI_DATATYPE_NULL))){
    retval = MPI_ERR_TYPE;
  } else if ((( sendbuf != MPI_IN_PLACE) && (sendcount <0)) || ((smpi_comm_rank(comm) == root) && (recvcount <0))){
    retval = MPI_ERR_COUNT;
  } else {

    char* sendtmpbuf = (char*) sendbuf;
    int sendtmpcount = sendcount;
    MPI_Datatype sendtmptype = sendtype;
    if( (smpi_comm_rank(comm) == root) && (sendbuf == MPI_IN_PLACE )) {
      sendtmpcount=0;
      sendtmptype=recvtype;
    }
  int rank = comm != MPI_COMM_NULL ? smpi_process_index() : -1;
  int root_traced = smpi_group_index(smpi_comm_group(comm), root);
  instr_extra_data extra = xbt_new0(s_instr_extra_data_t,1);
  extra->type = TRACING_GATHER;
  extra->root = root_traced;
  int known=0;
  extra->datatype1 = encode_datatype(sendtmptype, &known);
  int dt_size_send = 1;
  if(!known)
    dt_size_send = smpi_datatype_size(sendtmptype);
  extra->send_size = sendtmpcount*dt_size_send;
  extra->datatype2 = encode_datatype(recvtype, &known);
  int dt_size_recv = 1;
  if((smpi_comm_rank(comm)==root) && !known)
    dt_size_recv = smpi_datatype_size(recvtype);
  extra->recv_size = recvcount*dt_size_recv;

  TRACE_smpi_collective_in(rank, root_traced, __FUNCTION__, extra);

  mpi_coll_gather_fun(sendtmpbuf, sendtmpcount, sendtmptype, recvbuf, recvcount, recvtype, root, comm);

  retval = MPI_SUCCESS;
  TRACE_smpi_collective_out(rank, root_traced, __FUNCTION__);
  }

  smpi_bench_begin();
  return retval;
}

int PMPI_Gatherv(void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int *recvcounts, int *displs,
                MPI_Datatype recvtype, int root, MPI_Comm comm)
{
  int retval = 0;

  smpi_bench_end();

  if (comm == MPI_COMM_NULL) {
    retval = MPI_ERR_COMM;
  } else if ((( sendbuf != MPI_IN_PLACE) && (sendtype == MPI_DATATYPE_NULL)) ||
            ((smpi_comm_rank(comm) == root) && (recvtype == MPI_DATATYPE_NULL))){
    retval = MPI_ERR_TYPE;
  } else if (( sendbuf != MPI_IN_PLACE) && (sendcount <0)){
    retval = MPI_ERR_COUNT;
  } else if (recvcounts == NULL || displs == NULL) {
    retval = MPI_ERR_ARG;
  } else {
    char* sendtmpbuf = (char*) sendbuf;
    int sendtmpcount = sendcount;
    MPI_Datatype sendtmptype = sendtype;
    if( (smpi_comm_rank(comm) == root) && (sendbuf == MPI_IN_PLACE )) {
      sendtmpcount=0;
      sendtmptype=recvtype;
    }

  int rank = comm != MPI_COMM_NULL ? smpi_process_index() : -1;
  int root_traced = smpi_group_index(smpi_comm_group(comm), root);
  int i=0;
  int size = smpi_comm_size(comm);
  instr_extra_data extra = xbt_new0(s_instr_extra_data_t,1);
  extra->type = TRACING_GATHERV;
  extra->num_processes = size;
  extra->root = root_traced;
  int known=0;
  extra->datatype1 = encode_datatype(sendtmptype, &known);
  int dt_size_send = 1;
  if(!known)
    dt_size_send = smpi_datatype_size(sendtype);
  extra->send_size = sendtmpcount*dt_size_send;
  extra->datatype2 = encode_datatype(recvtype, &known);
  int dt_size_recv = 1;
  if(!known)
    dt_size_recv = smpi_datatype_size(recvtype);
  if((smpi_comm_rank(comm)==root)){
  extra->recvcounts= xbt_new(int,size);
  for(i=0; i< size; i++)//copy data to avoid bad free
    extra->recvcounts[i] = recvcounts[i]*dt_size_recv;
  }
  TRACE_smpi_collective_in(rank, root_traced, __FUNCTION__,extra);

  smpi_mpi_gatherv(sendtmpbuf, sendtmpcount, sendtmptype, recvbuf, recvcounts, displs, recvtype, root, comm);
    retval = MPI_SUCCESS;
  TRACE_smpi_collective_out(rank, root_traced, __FUNCTION__);
  }

  smpi_bench_begin();
  return retval;
}

int PMPI_Allgather(void *sendbuf, int sendcount, MPI_Datatype sendtype,
                   void *recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm)
{
  int retval = 0;

  smpi_bench_end();

  if (comm == MPI_COMM_NULL) {
    retval = MPI_ERR_COMM;
  } else if ((( sendbuf != MPI_IN_PLACE) && (sendtype == MPI_DATATYPE_NULL)) ||
            (recvtype == MPI_DATATYPE_NULL)){
    retval = MPI_ERR_TYPE;
  } else if ((( sendbuf != MPI_IN_PLACE) && (sendcount <0)) ||
            (recvcount <0)){
    retval = MPI_ERR_COUNT;
  } else {
    if(sendbuf == MPI_IN_PLACE) {
      sendbuf=((char*)recvbuf)+smpi_datatype_get_extent(recvtype)*recvcount*smpi_comm_rank(comm);
      sendcount=recvcount;
      sendtype=recvtype;
    }
  int rank = comm != MPI_COMM_NULL ? smpi_process_index() : -1;
  instr_extra_data extra = xbt_new0(s_instr_extra_data_t,1);
  extra->type = TRACING_ALLGATHER;
  int known=0;
  extra->datatype1 = encode_datatype(sendtype, &known);
  int dt_size_send = 1;
  if(!known)
    dt_size_send = smpi_datatype_size(sendtype);
  extra->send_size = sendcount*dt_size_send;
  extra->datatype2 = encode_datatype(recvtype, &known);
  int dt_size_recv = 1;
  if(!known)
    dt_size_recv = smpi_datatype_size(recvtype);
  extra->recv_size = recvcount*dt_size_recv;

  TRACE_smpi_collective_in(rank, -1, __FUNCTION__, extra);

  mpi_coll_allgather_fun(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, comm);
    retval = MPI_SUCCESS;
  TRACE_smpi_collective_out(rank, -1, __FUNCTION__);
  }
  smpi_bench_begin();
  return retval;
}

int PMPI_Allgatherv(void *sendbuf, int sendcount, MPI_Datatype sendtype,
                   void *recvbuf, int *recvcounts, int *displs, MPI_Datatype recvtype, MPI_Comm comm)
{
  int retval = 0;

  smpi_bench_end();

  if (comm == MPI_COMM_NULL) {
    retval = MPI_ERR_COMM;
  } else if ((( sendbuf != MPI_IN_PLACE) && (sendtype == MPI_DATATYPE_NULL)) ||
            (recvtype == MPI_DATATYPE_NULL)){
    retval = MPI_ERR_TYPE;
  } else if (( sendbuf != MPI_IN_PLACE) && (sendcount <0)){
    retval = MPI_ERR_COUNT;
  } else if (recvcounts == NULL || displs == NULL) {
    retval = MPI_ERR_ARG;
  } else {

    if(sendbuf == MPI_IN_PLACE) {
      sendbuf=((char*)recvbuf)+smpi_datatype_get_extent(recvtype)*displs[smpi_comm_rank(comm)];
      sendcount=recvcounts[smpi_comm_rank(comm)];
      sendtype=recvtype;
    }
  int rank = comm != MPI_COMM_NULL ? smpi_process_index() : -1;
  int i=0;
  int size = smpi_comm_size(comm);
  instr_extra_data extra = xbt_new0(s_instr_extra_data_t,1);
  extra->type = TRACING_ALLGATHERV;
  extra->num_processes = size;
  int known=0;
  extra->datatype1 = encode_datatype(sendtype, &known);
  int dt_size_send = 1;
  if(!known)
    dt_size_send = smpi_datatype_size(sendtype);
  extra->send_size = sendcount*dt_size_send;
  extra->datatype2 = encode_datatype(recvtype, &known);
  int dt_size_recv = 1;
  if(!known)
    dt_size_recv = smpi_datatype_size(recvtype);
  extra->recvcounts= xbt_new(int, size);
  for(i=0; i< size; i++)//copy data to avoid bad free
    extra->recvcounts[i] = recvcounts[i]*dt_size_recv;

  TRACE_smpi_collective_in(rank, -1, __FUNCTION__,extra);

    mpi_coll_allgatherv_fun(sendbuf, sendcount, sendtype, recvbuf, recvcounts, displs, recvtype, comm);
    retval = MPI_SUCCESS;
  TRACE_smpi_collective_out(rank, -1, __FUNCTION__);
  }

  smpi_bench_begin();
  return retval;
}

int PMPI_Scatter(void *sendbuf, int sendcount, MPI_Datatype sendtype,
                void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm)
{
  int retval = 0;

  smpi_bench_end();

  if (comm == MPI_COMM_NULL) {
    retval = MPI_ERR_COMM;
  } else if (((smpi_comm_rank(comm)==root) && (!is_datatype_valid(sendtype)))
             || ((recvbuf !=MPI_IN_PLACE) && (!is_datatype_valid(recvtype)))){
    retval = MPI_ERR_TYPE;
  } else if ((sendbuf == recvbuf) ||
      ((smpi_comm_rank(comm)==root) && sendcount>0 && (sendbuf == NULL))){
    retval = MPI_ERR_BUFFER;
  }else {

    if (recvbuf == MPI_IN_PLACE) {
        recvtype=sendtype;
        recvcount=sendcount;
    }
  int rank = comm != MPI_COMM_NULL ? smpi_process_index() : -1;
  int root_traced = smpi_group_index(smpi_comm_group(comm), root);
  instr_extra_data extra = xbt_new0(s_instr_extra_data_t,1);
  extra->type = TRACING_SCATTER;
  extra->root = root_traced;
  int known=0;
  extra->datatype1 = encode_datatype(sendtype, &known);
  int dt_size_send = 1;
  if((smpi_comm_rank(comm)==root) && !known)
    dt_size_send = smpi_datatype_size(sendtype);
  extra->send_size = sendcount*dt_size_send;
  extra->datatype2 = encode_datatype(recvtype, &known);
  int dt_size_recv = 1;
  if(!known)
    dt_size_recv = smpi_datatype_size(recvtype);
  extra->recv_size = recvcount*dt_size_recv;
  TRACE_smpi_collective_in(rank, root_traced, __FUNCTION__,extra);

  mpi_coll_scatter_fun(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, root, comm);
    retval = MPI_SUCCESS;
  TRACE_smpi_collective_out(rank, root_traced, __FUNCTION__);
  }

  smpi_bench_begin();
  return retval;
}

int PMPI_Scatterv(void *sendbuf, int *sendcounts, int *displs,
                 MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm)
{
  int retval = 0;

  smpi_bench_end();

  if (comm == MPI_COMM_NULL) {
    retval = MPI_ERR_COMM;
  } else if (sendcounts == NULL || displs == NULL) {
    retval = MPI_ERR_ARG;
  } else if (((smpi_comm_rank(comm)==root) && (sendtype == MPI_DATATYPE_NULL))
             || ((recvbuf !=MPI_IN_PLACE) && (recvtype == MPI_DATATYPE_NULL))) {
    retval = MPI_ERR_TYPE;
  } else {
    if (recvbuf == MPI_IN_PLACE) {
        recvtype=sendtype;
        recvcount=sendcounts[smpi_comm_rank(comm)];
    }
  int rank = comm != MPI_COMM_NULL ? smpi_process_index() : -1;
  int root_traced = smpi_group_index(smpi_comm_group(comm), root);
  int i=0;
  int size = smpi_comm_size(comm);
  instr_extra_data extra = xbt_new0(s_instr_extra_data_t,1);
  extra->type = TRACING_SCATTERV;
  extra->num_processes = size;
  extra->root = root_traced;
  int known=0;
  extra->datatype1 = encode_datatype(sendtype, &known);
  int dt_size_send = 1;
  if(!known)
    dt_size_send = smpi_datatype_size(sendtype);
  if((smpi_comm_rank(comm)==root)){
  extra->sendcounts= xbt_new(int, size);
  for(i=0; i< size; i++)//copy data to avoid bad free
    extra->sendcounts[i] = sendcounts[i]*dt_size_send;
  }
  extra->datatype2 = encode_datatype(recvtype, &known);
  int dt_size_recv = 1;
  if(!known)
    dt_size_recv = smpi_datatype_size(recvtype);
  extra->recv_size = recvcount*dt_size_recv;
  TRACE_smpi_collective_in(rank, root_traced, __FUNCTION__,extra);

    smpi_mpi_scatterv(sendbuf, sendcounts, displs, sendtype, recvbuf, recvcount, recvtype, root, comm);

    retval = MPI_SUCCESS;
  TRACE_smpi_collective_out(rank, root_traced, __FUNCTION__);
  }

  smpi_bench_begin();
  return retval;
}

int PMPI_Reduce(void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, int root, MPI_Comm comm)
{
  int retval = 0;

  smpi_bench_end();

  if (comm == MPI_COMM_NULL) {
    retval = MPI_ERR_COMM;
  } else if (!is_datatype_valid(datatype) || op == MPI_OP_NULL) {
    retval = MPI_ERR_ARG;
  } else {
  int rank = comm != MPI_COMM_NULL ? smpi_process_index() : -1;
  int root_traced = smpi_group_index(smpi_comm_group(comm), root);
  instr_extra_data extra = xbt_new0(s_instr_extra_data_t,1);
  extra->type = TRACING_REDUCE;
  int known=0;
  extra->datatype1 = encode_datatype(datatype, &known);
  int dt_size_send = 1;
  if(!known)
    dt_size_send = smpi_datatype_size(datatype);
  extra->send_size = count*dt_size_send;
  extra->root = root_traced;

  TRACE_smpi_collective_in(rank, root_traced, __FUNCTION__,extra);

  mpi_coll_reduce_fun(sendbuf, recvbuf, count, datatype, op, root, comm);

    retval = MPI_SUCCESS;
  TRACE_smpi_collective_out(rank, root_traced, __FUNCTION__);
  }

  smpi_bench_begin();
  return retval;
}

int PMPI_Reduce_local(void *inbuf, void *inoutbuf, int count, MPI_Datatype datatype, MPI_Op op){
  int retval = 0;

    smpi_bench_end();
    if (!is_datatype_valid(datatype) || op == MPI_OP_NULL) {
      retval = MPI_ERR_ARG;
    } else {
      smpi_op_apply(op, inbuf, inoutbuf, &count, &datatype);
      retval=MPI_SUCCESS;
    }
    smpi_bench_begin();
    return retval;
}

int PMPI_Allreduce(void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm)
{
  int retval = 0;

  smpi_bench_end();

  if (comm == MPI_COMM_NULL) {
    retval = MPI_ERR_COMM;
  } else if (!is_datatype_valid(datatype)) {
    retval = MPI_ERR_TYPE;
  } else if (op == MPI_OP_NULL) {
    retval = MPI_ERR_OP;
  } else {

    char* sendtmpbuf = (char*) sendbuf;
    if( sendbuf == MPI_IN_PLACE ) {
      sendtmpbuf = (char *)xbt_malloc(count*smpi_datatype_get_extent(datatype));
      smpi_datatype_copy(recvbuf, count, datatype,sendtmpbuf, count, datatype);
    }
  int rank = comm != MPI_COMM_NULL ? smpi_process_index() : -1;
  instr_extra_data extra = xbt_new0(s_instr_extra_data_t,1);
  extra->type = TRACING_ALLREDUCE;
  int known=0;
  extra->datatype1 = encode_datatype(datatype, &known);
  int dt_size_send = 1;
  if(!known)
    dt_size_send = smpi_datatype_size(datatype);
  extra->send_size = count*dt_size_send;

  TRACE_smpi_collective_in(rank, -1, __FUNCTION__,extra);

  mpi_coll_allreduce_fun(sendtmpbuf, recvbuf, count, datatype, op, comm);

    if( sendbuf == MPI_IN_PLACE )
      xbt_free(sendtmpbuf);

    retval = MPI_SUCCESS;
  TRACE_smpi_collective_out(rank, -1, __FUNCTION__);
  }

  smpi_bench_begin();
  return retval;
}

int PMPI_Scan(void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm)
{
  int retval = 0;

  smpi_bench_end();

  if (comm == MPI_COMM_NULL) {
    retval = MPI_ERR_COMM;
  } else if (!is_datatype_valid(datatype)) {
    retval = MPI_ERR_TYPE;
  } else if (op == MPI_OP_NULL) {
    retval = MPI_ERR_OP;
  } else {
  int rank = comm != MPI_COMM_NULL ? smpi_process_index() : -1;
  instr_extra_data extra = xbt_new0(s_instr_extra_data_t,1);
  extra->type = TRACING_SCAN;
  int known=0;
  extra->datatype1 = encode_datatype(datatype, &known);
  int dt_size_send = 1;
  if(!known)
    dt_size_send = smpi_datatype_size(datatype);
  extra->send_size = count*dt_size_send;

  TRACE_smpi_collective_in(rank, -1, __FUNCTION__,extra);

  smpi_mpi_scan(sendbuf, recvbuf, count, datatype, op, comm);

  retval = MPI_SUCCESS;
  TRACE_smpi_collective_out(rank, -1, __FUNCTION__);
  }

  smpi_bench_begin();
  return retval;
}

int PMPI_Exscan(void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm){
  int retval = 0;

  smpi_bench_end();

  if (comm == MPI_COMM_NULL) {
    retval = MPI_ERR_COMM;
  } else if (!is_datatype_valid(datatype)) {
    retval = MPI_ERR_TYPE;
  } else if (op == MPI_OP_NULL) {
    retval = MPI_ERR_OP;
  } else {
  int rank = comm != MPI_COMM_NULL ? smpi_process_index() : -1;
  instr_extra_data extra = xbt_new0(s_instr_extra_data_t,1);
  extra->type = TRACING_EXSCAN;
  int known=0;
  extra->datatype1 = encode_datatype(datatype, &known);
  int dt_size_send = 1;
  if(!known)
    dt_size_send = smpi_datatype_size(datatype);
  extra->send_size = count*dt_size_send;
  TRACE_smpi_collective_in(rank, -1, __FUNCTION__,extra);

  smpi_mpi_exscan(sendbuf, recvbuf, count, datatype, op, comm);
    retval = MPI_SUCCESS;
  TRACE_smpi_collective_out(rank, -1, __FUNCTION__);
  }

  smpi_bench_begin();
  return retval;
}

int PMPI_Reduce_scatter(void *sendbuf, void *recvbuf, int *recvcounts, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm)
{
  int retval = 0;
  smpi_bench_end();

  if (comm == MPI_COMM_NULL) {
    retval = MPI_ERR_COMM;
  } else if (!is_datatype_valid(datatype)) {
    retval = MPI_ERR_TYPE;
  } else if (op == MPI_OP_NULL) {
    retval = MPI_ERR_OP;
  } else if (recvcounts == NULL) {
    retval = MPI_ERR_ARG;
  } else {
  int rank = comm != MPI_COMM_NULL ? smpi_process_index() : -1;
  int i=0;
  int size = smpi_comm_size(comm);
  instr_extra_data extra = xbt_new0(s_instr_extra_data_t,1);
  extra->type = TRACING_REDUCE_SCATTER;
  extra->num_processes = size;
  int known=0;
  extra->datatype1 = encode_datatype(datatype, &known);
  int dt_size_send = 1;
  if(!known)
    dt_size_send = smpi_datatype_size(datatype);
  extra->send_size = 0;
  extra->recvcounts= xbt_new(int, size);
  for(i=0; i< size; i++)//copy data to avoid bad free
    extra->recvcounts[i] = recvcounts[i]*dt_size_send;
  TRACE_smpi_collective_in(rank, -1, __FUNCTION__,extra);

  void* sendtmpbuf=sendbuf;
    if(sendbuf==MPI_IN_PLACE)
      sendtmpbuf=recvbuf;

    mpi_coll_reduce_scatter_fun(sendtmpbuf, recvbuf, recvcounts, datatype,  op, comm);
    retval = MPI_SUCCESS;
  TRACE_smpi_collective_out(rank, -1, __FUNCTION__);
  }

  smpi_bench_begin();
  return retval;
}

int PMPI_Reduce_scatter_block(void *sendbuf, void *recvbuf, int recvcount,
                              MPI_Datatype datatype, MPI_Op op, MPI_Comm comm)
{
  int retval,i;
  smpi_bench_end();

  if (comm == MPI_COMM_NULL) {
    retval = MPI_ERR_COMM;
  } else if (!is_datatype_valid(datatype)) {
    retval = MPI_ERR_TYPE;
  } else if (op == MPI_OP_NULL) {
    retval = MPI_ERR_OP;
  } else if (recvcount < 0) {
    retval = MPI_ERR_ARG;
  } else {
    int count=smpi_comm_size(comm);

  int rank = comm != MPI_COMM_NULL ? smpi_process_index() : -1;
  instr_extra_data extra = xbt_new0(s_instr_extra_data_t,1);
  extra->type = TRACING_REDUCE_SCATTER;
  extra->num_processes = count;
  int known=0;
  extra->datatype1 = encode_datatype(datatype, &known);
  int dt_size_send = 1;
  if(!known)
    dt_size_send = smpi_datatype_size(datatype);
  extra->send_size = 0;
  extra->recvcounts= xbt_new(int, count);
  for(i=0; i< count; i++)//copy data to avoid bad free
    extra->recvcounts[i] = recvcount*dt_size_send;

  TRACE_smpi_collective_in(rank, -1, __FUNCTION__,extra);

  int* recvcounts=(int*)xbt_malloc(count);
    for (i=0; i<count;i++)recvcounts[i]=recvcount;
    mpi_coll_reduce_scatter_fun(sendbuf, recvbuf, recvcounts, datatype,  op, comm);
    xbt_free(recvcounts);
    retval = MPI_SUCCESS;

    TRACE_smpi_collective_out(rank, -1, __FUNCTION__);
  }

  smpi_bench_begin();
  return retval;
}

int PMPI_Alltoall(void *sendbuf, int sendcount, MPI_Datatype sendtype,
                 void *recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm)
{
  int retval = 0;

  smpi_bench_end();

  if (comm == MPI_COMM_NULL) {
    retval = MPI_ERR_COMM;
  } else if (sendtype == MPI_DATATYPE_NULL
             || recvtype == MPI_DATATYPE_NULL) {
    retval = MPI_ERR_TYPE;
  } else {
  int rank = comm != MPI_COMM_NULL ? smpi_process_index() : -1;
  instr_extra_data extra = xbt_new0(s_instr_extra_data_t,1);
  extra->type = TRACING_ALLTOALL;
  int known=0;
  extra->datatype1 = encode_datatype(sendtype, &known);
  if(!known)
    extra->send_size = sendcount*smpi_datatype_size(sendtype);
  else
    extra->send_size = sendcount;
  extra->datatype2 = encode_datatype(recvtype, &known);
  if(!known)
    extra->recv_size = recvcount*smpi_datatype_size(recvtype);
  else
    extra->recv_size = recvcount;
  TRACE_smpi_collective_in(rank, -1, __FUNCTION__,extra);

  retval = mpi_coll_alltoall_fun(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, comm);

  TRACE_smpi_collective_out(rank, -1, __FUNCTION__);
  }

  smpi_bench_begin();
  return retval;
}

int PMPI_Alltoallv(void *sendbuf, int *sendcounts, int *senddisps,MPI_Datatype sendtype, void *recvbuf, int *recvcounts,
                  int *recvdisps, MPI_Datatype recvtype, MPI_Comm comm)
{
  int retval = 0;

  smpi_bench_end();

  if (comm == MPI_COMM_NULL) {
    retval = MPI_ERR_COMM;
  } else if (sendtype == MPI_DATATYPE_NULL || recvtype == MPI_DATATYPE_NULL) {
    retval = MPI_ERR_TYPE;
  } else if (sendcounts == NULL || senddisps == NULL || recvcounts == NULL || recvdisps == NULL) {
    retval = MPI_ERR_ARG;
  } else {
  int rank = comm != MPI_COMM_NULL ? smpi_process_index() : -1;
  int i=0;
  int size = smpi_comm_size(comm);
  instr_extra_data extra = xbt_new0(s_instr_extra_data_t,1);
  extra->type = TRACING_ALLTOALLV;
  extra->send_size = 0;
  extra->recv_size = 0;
  extra->recvcounts= xbt_new(int, size);
  extra->sendcounts= xbt_new(int, size);
  int known=0;
  extra->datatype1 = encode_datatype(sendtype, &known);
  int dt_size_send = 1;
  if(!known)
    dt_size_send = smpi_datatype_size(sendtype);
  int dt_size_recv = 1;
  extra->datatype2 = encode_datatype(recvtype, &known);
  if(!known)
    dt_size_recv = smpi_datatype_size(recvtype);
  for(i=0; i< size; i++){//copy data to avoid bad free
    extra->send_size += sendcounts[i]*dt_size_send;
    extra->recv_size += recvcounts[i]*dt_size_recv;

    extra->sendcounts[i] = sendcounts[i]*dt_size_send;
    extra->recvcounts[i] = recvcounts[i]*dt_size_recv;
  }
  extra->num_processes = size;
  TRACE_smpi_collective_in(rank, -1, __FUNCTION__,extra);

  retval = mpi_coll_alltoallv_fun(sendbuf, sendcounts, senddisps, sendtype, recvbuf, recvcounts, recvdisps, recvtype,
                                  comm);
  TRACE_smpi_collective_out(rank, -1, __FUNCTION__);
  }

  smpi_bench_begin();
  return retval;
}


int PMPI_Get_processor_name(char *name, int *resultlen)
{
  int retval = MPI_SUCCESS;

  strncpy(name, sg_host_get_name(SIMIX_host_self()),
          strlen(sg_host_get_name(SIMIX_host_self())) < MPI_MAX_PROCESSOR_NAME - 1 ?
          strlen(sg_host_get_name(SIMIX_host_self())) +1 : MPI_MAX_PROCESSOR_NAME - 1 );
  *resultlen = strlen(name) > MPI_MAX_PROCESSOR_NAME ? MPI_MAX_PROCESSOR_NAME : strlen(name);

  return retval;
}

int PMPI_Get_count(MPI_Status * status, MPI_Datatype datatype, int *count)
{
  int retval = MPI_SUCCESS;
  size_t size;

  if (status == NULL || count == NULL) {
    retval = MPI_ERR_ARG;
  } else if (!is_datatype_valid(datatype)) {
    retval = MPI_ERR_TYPE;
  } else {
    size = smpi_datatype_size(datatype);
    if (size == 0) {
      *count = 0;
    } else if (status->count % size != 0) {
      retval = MPI_UNDEFINED;
    } else {
      *count = smpi_mpi_get_count(status, datatype);
    }
  }
  return retval;
}

int PMPI_Type_contiguous(int count, MPI_Datatype old_type, MPI_Datatype* new_type) {
  int retval = 0;

  if (old_type == MPI_DATATYPE_NULL) {
    retval = MPI_ERR_TYPE;
  } else if (count<0){
    retval = MPI_ERR_COUNT;
  } else {
    retval = smpi_datatype_contiguous(count, old_type, new_type, 0);
  }
  return retval;
}

int PMPI_Type_commit(MPI_Datatype* datatype) {
  int retval = 0;

  if (datatype == NULL || *datatype == MPI_DATATYPE_NULL) {
    retval = MPI_ERR_TYPE;
  } else {
    smpi_datatype_commit(datatype);
    retval = MPI_SUCCESS;
  }
  return retval;
}

int PMPI_Type_vector(int count, int blocklen, int stride, MPI_Datatype old_type, MPI_Datatype* new_type) {
  int retval = 0;

  if (old_type == MPI_DATATYPE_NULL) {
    retval = MPI_ERR_TYPE;
  } else if (count<0 || blocklen<0){
    retval = MPI_ERR_COUNT;
  } else {
    retval = smpi_datatype_vector(count, blocklen, stride, old_type, new_type);
  }
  return retval;
}

int PMPI_Type_hvector(int count, int blocklen, MPI_Aint stride, MPI_Datatype old_type, MPI_Datatype* new_type) {
  int retval = 0;

  if (old_type == MPI_DATATYPE_NULL) {
    retval = MPI_ERR_TYPE;
  } else if (count<0 || blocklen<0){
    retval = MPI_ERR_COUNT;
  } else {
    retval = smpi_datatype_hvector(count, blocklen, stride, old_type, new_type);
  }
  return retval;
}

int PMPI_Type_create_hvector(int count, int blocklen, MPI_Aint stride, MPI_Datatype old_type, MPI_Datatype* new_type) {
  return MPI_Type_hvector(count, blocklen, stride, old_type, new_type);
}

int PMPI_Type_indexed(int count, int* blocklens, int* indices, MPI_Datatype old_type, MPI_Datatype* new_type) {
  int retval = 0;

  if (old_type == MPI_DATATYPE_NULL) {
    retval = MPI_ERR_TYPE;
  } else if (count<0){
    retval = MPI_ERR_COUNT;
  } else {
    retval = smpi_datatype_indexed(count, blocklens, indices, old_type, new_type);
  }
  return retval;
}

int PMPI_Type_create_indexed(int count, int* blocklens, int* indices, MPI_Datatype old_type, MPI_Datatype* new_type) {
  int retval = 0;

  if (old_type == MPI_DATATYPE_NULL) {
    retval = MPI_ERR_TYPE;
  } else if (count<0){
    retval = MPI_ERR_COUNT;
  } else {
    retval = smpi_datatype_indexed(count, blocklens, indices, old_type, new_type);
  }
  return retval;
}

int PMPI_Type_create_indexed_block(int count, int blocklength, int* indices, MPI_Datatype old_type,
                                   MPI_Datatype* new_type) {
  int retval,i;

  if (old_type == MPI_DATATYPE_NULL) {
    retval = MPI_ERR_TYPE;
  } else if (count<0){
    retval = MPI_ERR_COUNT;
  } else {
    int* blocklens=(int*)xbt_malloc(blocklength*count);
    for (i=0; i<count;i++)blocklens[i]=blocklength;
    retval = smpi_datatype_indexed(count, blocklens, indices, old_type, new_type);
    xbt_free(blocklens);
  }
  return retval;
}

int PMPI_Type_hindexed(int count, int* blocklens, MPI_Aint* indices, MPI_Datatype old_type, MPI_Datatype* new_type) {
  int retval = 0;

  if (old_type == MPI_DATATYPE_NULL) {
    retval = MPI_ERR_TYPE;
  } else if (count<0){
    retval = MPI_ERR_COUNT;
  } else {
    retval = smpi_datatype_hindexed(count, blocklens, indices, old_type, new_type);
  }
  return retval;
}

int PMPI_Type_create_hindexed(int count, int* blocklens, MPI_Aint* indices, MPI_Datatype old_type,
                              MPI_Datatype* new_type) {
  return PMPI_Type_hindexed(count, blocklens,indices,old_type,new_type);
}

int PMPI_Type_create_hindexed_block(int count, int blocklength, MPI_Aint* indices, MPI_Datatype old_type,
                                    MPI_Datatype* new_type) {
  int retval,i;

  if (old_type == MPI_DATATYPE_NULL) {
    retval = MPI_ERR_TYPE;
  } else if (count<0){
    retval = MPI_ERR_COUNT;
  } else {
    int* blocklens=(int*)xbt_malloc(blocklength*count);
    for (i=0; i<count;i++)blocklens[i]=blocklength;
    retval = smpi_datatype_hindexed(count, blocklens, indices, old_type, new_type);
    xbt_free(blocklens);
  }
  return retval;
}

int PMPI_Type_struct(int count, int* blocklens, MPI_Aint* indices, MPI_Datatype* old_types, MPI_Datatype* new_type) {
  int retval = 0;

  if (count<0){
    retval = MPI_ERR_COUNT;
  } else {
    retval = smpi_datatype_struct(count, blocklens, indices, old_types, new_type);
  }
  return retval;
}

int PMPI_Type_create_struct(int count, int* blocklens, MPI_Aint* indices, MPI_Datatype* old_types,
                            MPI_Datatype* new_type) {
  return PMPI_Type_struct(count, blocklens, indices, old_types, new_type);
}

int PMPI_Error_class(int errorcode, int* errorclass) {
  // assume smpi uses only standard mpi error codes
  *errorclass=errorcode;
  return MPI_SUCCESS;
}

int PMPI_Initialized(int* flag) {
   *flag=smpi_process_initialized();
   return MPI_SUCCESS;
}

/* The topo part of MPI_COMM_WORLD should always be NULL. When other topologies will be implemented, not only should we
 * check if the topology is NULL, but we should check if it is the good topology type (so we have to add a
 *  MPIR_Topo_Type field, and replace the MPI_Topology field by an union)*/

int PMPI_Cart_create(MPI_Comm comm_old, int ndims, int* dims, int* periodic, int reorder, MPI_Comm* comm_cart) {
  int retval = 0;
  if (comm_old == MPI_COMM_NULL){
    retval =  MPI_ERR_COMM;
  } else if (ndims < 0 || (ndims > 0 && (dims == NULL || periodic == NULL)) || comm_cart == NULL) {
    retval = MPI_ERR_ARG;
  } else{
    retval = smpi_mpi_cart_create(comm_old, ndims, dims, periodic, reorder, comm_cart);
  }
  return retval;
}

int PMPI_Cart_rank(MPI_Comm comm, int* coords, int* rank) {
  if(comm == MPI_COMM_NULL || smpi_comm_topo(comm) == NULL) {
    return MPI_ERR_TOPOLOGY;
  }
  if (coords == NULL) {
    return MPI_ERR_ARG;
  }
  return smpi_mpi_cart_rank(comm, coords, rank);
}

int PMPI_Cart_shift(MPI_Comm comm, int direction, int displ, int* source, int* dest) {
  if(comm == MPI_COMM_NULL || smpi_comm_topo(comm) == NULL) {
    return MPI_ERR_TOPOLOGY;
  }
  if (source == NULL || dest == NULL || direction < 0 ) {
    return MPI_ERR_ARG;
  }
  return smpi_mpi_cart_shift(comm, direction, displ, source, dest);
}

int PMPI_Cart_coords(MPI_Comm comm, int rank, int maxdims, int* coords) {
  if(comm == MPI_COMM_NULL || smpi_comm_topo(comm) == NULL) {
    return MPI_ERR_TOPOLOGY;
  }
  if (rank < 0 || rank >= smpi_comm_size(comm)) {
    return MPI_ERR_RANK;
  }
  if (maxdims <= 0) {
    return MPI_ERR_ARG;
  }
  if(coords == NULL) {
    return MPI_ERR_ARG;
  }
  return smpi_mpi_cart_coords(comm, rank, maxdims, coords);
}

int PMPI_Cart_get(MPI_Comm comm, int maxdims, int* dims, int* periods, int* coords) {
  if(comm == NULL || smpi_comm_topo(comm) == NULL) {
    return MPI_ERR_TOPOLOGY;
  }
  if(maxdims <= 0 || dims == NULL || periods == NULL || coords == NULL) {
    return MPI_ERR_ARG;
  }
  return smpi_mpi_cart_get(comm, maxdims, dims, periods, coords);
}

int PMPI_Cartdim_get(MPI_Comm comm, int* ndims) {
  if (comm == MPI_COMM_NULL || smpi_comm_topo(comm) == NULL) {
    return MPI_ERR_TOPOLOGY;
  }
  if (ndims == NULL) {
    return MPI_ERR_ARG;
  }
  return smpi_mpi_cartdim_get(comm, ndims);
}

int PMPI_Dims_create(int nnodes, int ndims, int* dims) {
  if(dims == NULL) {
    return MPI_ERR_ARG;
  }
  if (ndims < 1 || nnodes < 1) {
    return MPI_ERR_DIMS;
  }

  return smpi_mpi_dims_create(nnodes, ndims, dims);
}

int PMPI_Cart_sub(MPI_Comm comm, int* remain_dims, MPI_Comm* comm_new) {
  if(comm == MPI_COMM_NULL || smpi_comm_topo(comm) == NULL) {
    return MPI_ERR_TOPOLOGY;
  }
  if (comm_new == NULL) {
    return MPI_ERR_ARG;
  }
  return smpi_mpi_cart_sub(comm, remain_dims, comm_new);
}

int PMPI_Type_create_resized(MPI_Datatype oldtype,MPI_Aint lb, MPI_Aint extent, MPI_Datatype *newtype){
    if(oldtype == MPI_DATATYPE_NULL) {
        return MPI_ERR_TYPE;
    }
    int blocks[3] = { 1, 1, 1 };
    MPI_Aint disps[3] = { lb, 0, lb+extent };
    MPI_Datatype types[3] = { MPI_LB, oldtype, MPI_UB };

    s_smpi_mpi_struct_t* subtype = smpi_datatype_struct_create( blocks, disps, 3, types);
    smpi_datatype_create(newtype,oldtype->size, lb, lb + extent, 1 , subtype, DT_FLAG_VECTOR);

    (*newtype)->flags &= ~DT_FLAG_COMMITED;
    return MPI_SUCCESS;
}

int PMPI_Win_create( void *base, MPI_Aint size, int disp_unit, MPI_Info info, MPI_Comm comm, MPI_Win *win){
  int retval = 0;
  smpi_bench_end();
  if (comm == MPI_COMM_NULL) {
    retval= MPI_ERR_COMM;
  }else if ((base == NULL && size != 0) || disp_unit <= 0 || size < 0 ){
    retval= MPI_ERR_OTHER;
  }else{
    *win = smpi_mpi_win_create( base, size, disp_unit, info, comm);
    retval = MPI_SUCCESS;
  }
  smpi_bench_begin();
  return retval;
}

int PMPI_Win_free( MPI_Win* win){
  int retval = 0;
  smpi_bench_end();
  if (win == NULL || *win == MPI_WIN_NULL) {
    retval = MPI_ERR_WIN;
  }else{
    retval=smpi_mpi_win_free(win);
  }
  smpi_bench_begin();
  return retval;
}

int PMPI_Win_set_name(MPI_Win  win, char * name)
{
  int retval = 0;
  if (win == MPI_WIN_NULL)  {
    retval = MPI_ERR_TYPE;
  } else if (name == NULL)  {
    retval = MPI_ERR_ARG;
  } else {
    smpi_mpi_win_set_name(win, name);
    retval = MPI_SUCCESS;
  }
  return retval;
}

int PMPI_Win_get_name(MPI_Win  win, char * name, int* len)
{
  int retval = MPI_SUCCESS;

  if (win == MPI_WIN_NULL)  {
    retval = MPI_ERR_WIN;
  } else if (name == NULL)  {
    retval = MPI_ERR_ARG;
  } else {
    smpi_mpi_win_get_name(win, name, len);
  }
  return retval;
}

int PMPI_Win_get_group(MPI_Win  win, MPI_Group * group){
  int retval = MPI_SUCCESS;
  if (win == MPI_WIN_NULL)  {
    retval = MPI_ERR_WIN;
  }else {
    smpi_mpi_win_get_group(win, group);
  }
  return retval;
}


int PMPI_Win_fence( int assert,  MPI_Win win){
  int retval = 0;
  smpi_bench_end();
  if (win == MPI_WIN_NULL) {
    retval = MPI_ERR_WIN;
  } else {
  int rank = smpi_process_index();
  TRACE_smpi_collective_in(rank, -1, __FUNCTION__, NULL);
  retval = smpi_mpi_win_fence(assert, win);
  TRACE_smpi_collective_out(rank, -1, __FUNCTION__);
  }
  smpi_bench_begin();
  return retval;
}

int PMPI_Get( void *origin_addr, int origin_count, MPI_Datatype origin_datatype, int target_rank,
              MPI_Aint target_disp, int target_count, MPI_Datatype target_datatype, MPI_Win win){
  int retval = 0;
  smpi_bench_end();
  if (win == MPI_WIN_NULL) {
    retval = MPI_ERR_WIN;
  } else if (target_rank == MPI_PROC_NULL) {
    retval = MPI_SUCCESS;
  } else if (target_rank <0){
    retval = MPI_ERR_RANK;
  } else if (target_disp <0){
      retval = MPI_ERR_ARG;
  } else if (origin_count < 0 || target_count < 0) {
    retval = MPI_ERR_COUNT;
  } else if (origin_addr==NULL && origin_count > 0){
    retval = MPI_ERR_COUNT;
  } else if ((!is_datatype_valid(origin_datatype)) || (!is_datatype_valid(target_datatype))) {
    retval = MPI_ERR_TYPE;
  } else {
    int rank = smpi_process_index();
    MPI_Group group;
    smpi_mpi_win_get_group(win, &group);
    int src_traced = smpi_group_index(group, target_rank);
    TRACE_smpi_ptp_in(rank, src_traced, rank, __FUNCTION__, NULL);

    retval = smpi_mpi_get( origin_addr, origin_count, origin_datatype, target_rank, target_disp, target_count,
                           target_datatype, win);

    TRACE_smpi_ptp_out(rank, src_traced, rank, __FUNCTION__);
  }
  smpi_bench_begin();
  return retval;
}

int PMPI_Put( void *origin_addr, int origin_count, MPI_Datatype origin_datatype, int target_rank,
              MPI_Aint target_disp, int target_count, MPI_Datatype target_datatype, MPI_Win win){
  int retval = 0;
  smpi_bench_end();
  if (win == MPI_WIN_NULL) {
    retval = MPI_ERR_WIN;
  } else if (target_rank == MPI_PROC_NULL) {
    retval = MPI_SUCCESS;
  } else if (target_rank <0){
    retval = MPI_ERR_RANK;
  } else if (target_disp <0){
    retval = MPI_ERR_ARG;
  } else if (origin_count < 0 || target_count < 0) {
    retval = MPI_ERR_COUNT;
  } else if (origin_addr==NULL && origin_count > 0){
    retval = MPI_ERR_COUNT;
  } else if ((!is_datatype_valid(origin_datatype)) || (!is_datatype_valid(target_datatype))) {
    retval = MPI_ERR_TYPE;
  } else {
    int rank = smpi_process_index();
    MPI_Group group;
    smpi_mpi_win_get_group(win, &group);
    int dst_traced = smpi_group_index(group, target_rank);
    TRACE_smpi_ptp_in(rank, rank, dst_traced, __FUNCTION__, NULL);
    TRACE_smpi_send(rank, rank, dst_traced, origin_count*smpi_datatype_size(origin_datatype));

    retval = smpi_mpi_put( origin_addr, origin_count, origin_datatype, target_rank, target_disp, target_count,
                           target_datatype, win);

    TRACE_smpi_ptp_out(rank, rank, dst_traced, __FUNCTION__);
  }
  smpi_bench_begin();
  return retval;
}

int PMPI_Accumulate( void *origin_addr, int origin_count, MPI_Datatype origin_datatype, int target_rank,
              MPI_Aint target_disp, int target_count, MPI_Datatype target_datatype, MPI_Op op, MPI_Win win){
  int retval = 0;
  smpi_bench_end();
  if (win == MPI_WIN_NULL) {
    retval = MPI_ERR_WIN;
  } else if (target_rank == MPI_PROC_NULL) {
    retval = MPI_SUCCESS;
  } else if (target_rank <0){
    retval = MPI_ERR_RANK;
  } else if (target_disp <0){
    retval = MPI_ERR_ARG;
  } else if (origin_count < 0 || target_count < 0) {
    retval = MPI_ERR_COUNT;
  } else if (origin_addr==NULL && origin_count > 0){
    retval = MPI_ERR_COUNT;
  } else if ((!is_datatype_valid(origin_datatype)) ||
            (!is_datatype_valid(target_datatype))) {
    retval = MPI_ERR_TYPE;
  } else if (op == MPI_OP_NULL) {
    retval = MPI_ERR_OP;
  } else {
    int rank = smpi_process_index();
    MPI_Group group;
    smpi_mpi_win_get_group(win, &group);
    int src_traced = smpi_group_index(group, target_rank);
    TRACE_smpi_ptp_in(rank, src_traced, rank, __FUNCTION__, NULL);

    retval = smpi_mpi_accumulate( origin_addr, origin_count, origin_datatype, target_rank, target_disp, target_count,
                                  target_datatype, op, win);

    TRACE_smpi_ptp_out(rank, src_traced, rank, __FUNCTION__);
  }
  smpi_bench_begin();
  return retval;
}

int PMPI_Win_post(MPI_Group group, int assert, MPI_Win win){
  int retval = 0;
  smpi_bench_end();
  if (win == MPI_WIN_NULL) {
    retval = MPI_ERR_WIN;
  } else if (group==MPI_GROUP_NULL){
    retval = MPI_ERR_GROUP;
  }
  else {
    int rank = smpi_process_index();
    TRACE_smpi_collective_in(rank, -1, __FUNCTION__, NULL);
    retval = smpi_mpi_win_post(group,assert,win);
    TRACE_smpi_collective_out(rank, -1, __FUNCTION__);
  }
  smpi_bench_begin();
  return retval;
}

int PMPI_Win_start(MPI_Group group, int assert, MPI_Win win){
  int retval = 0;
  smpi_bench_end();
  if (win == MPI_WIN_NULL) {
    retval = MPI_ERR_WIN;
  } else if (group==MPI_GROUP_NULL){
    retval = MPI_ERR_GROUP;
  }
  else {
    int rank = smpi_process_index();
    TRACE_smpi_collective_in(rank, -1, __FUNCTION__, NULL);
    retval = smpi_mpi_win_start(group,assert,win);
    TRACE_smpi_collective_out(rank, -1, __FUNCTION__);
  }
  smpi_bench_begin();
  return retval;
}

int PMPI_Win_complete(MPI_Win win){
  int retval = 0;
  smpi_bench_end();
  if (win == MPI_WIN_NULL) {
    retval = MPI_ERR_WIN;
  }
  else {
    int rank = smpi_process_index();
    TRACE_smpi_collective_in(rank, -1, __FUNCTION__, NULL);

    retval = smpi_mpi_win_complete(win);

    TRACE_smpi_collective_out(rank, -1, __FUNCTION__);
  }
  smpi_bench_begin();
  return retval;
}

int PMPI_Win_wait(MPI_Win win){
  int retval = 0;
  smpi_bench_end();
  if (win == MPI_WIN_NULL) {
    retval = MPI_ERR_WIN;
  }
  else {
    int rank = smpi_process_index();
    TRACE_smpi_collective_in(rank, -1, __FUNCTION__, NULL);

    retval = smpi_mpi_win_wait(win);

    TRACE_smpi_collective_out(rank, -1, __FUNCTION__);
  }
  smpi_bench_begin();
  return retval;
}

int PMPI_Alloc_mem(MPI_Aint size, MPI_Info info, void *baseptr){
  void *ptr = xbt_malloc(size);
  if(!ptr)
    return MPI_ERR_NO_MEM;
  else {
    *(void **)baseptr = ptr;
    return MPI_SUCCESS;
  }
}

int PMPI_Free_mem(void *baseptr){
  xbt_free(baseptr);
  return MPI_SUCCESS;
}

int PMPI_Type_set_name(MPI_Datatype  datatype, char * name)
{
  int retval = 0;
  if (datatype == MPI_DATATYPE_NULL)  {
    retval = MPI_ERR_TYPE;
  } else if (name == NULL)  {
    retval = MPI_ERR_ARG;
  } else {
    smpi_datatype_set_name(datatype, name);
    retval = MPI_SUCCESS;
  }
  return retval;
}

int PMPI_Type_get_name(MPI_Datatype  datatype, char * name, int* len)
{
  int retval = 0;

  if (datatype == MPI_DATATYPE_NULL)  {
    retval = MPI_ERR_TYPE;
  } else if (name == NULL)  {
    retval = MPI_ERR_ARG;
  } else {
    smpi_datatype_get_name(datatype, name, len);
    retval = MPI_SUCCESS;
  }
  return retval;
}

MPI_Datatype PMPI_Type_f2c(MPI_Fint datatype){
  return smpi_type_f2c(datatype);
}

MPI_Fint PMPI_Type_c2f(MPI_Datatype datatype){
  return smpi_type_c2f( datatype);
}

MPI_Group PMPI_Group_f2c(MPI_Fint group){
  return smpi_group_f2c( group);
}

MPI_Fint PMPI_Group_c2f(MPI_Group group){
  return smpi_group_c2f(group);
}

MPI_Request PMPI_Request_f2c(MPI_Fint request){
  return smpi_request_f2c(request);
}

MPI_Fint PMPI_Request_c2f(MPI_Request request) {
  return smpi_request_c2f(request);
}

MPI_Win PMPI_Win_f2c(MPI_Fint win){
  return smpi_win_f2c(win);
}

MPI_Fint PMPI_Win_c2f(MPI_Win win){
  return smpi_win_c2f(win);
}

MPI_Op PMPI_Op_f2c(MPI_Fint op){
  return smpi_op_f2c(op);
}

MPI_Fint PMPI_Op_c2f(MPI_Op op){
  return smpi_op_c2f(op);
}

MPI_Comm PMPI_Comm_f2c(MPI_Fint comm){
  return smpi_comm_f2c(comm);
}

MPI_Fint PMPI_Comm_c2f(MPI_Comm comm){
  return smpi_comm_c2f(comm);
}

MPI_Info PMPI_Info_f2c(MPI_Fint info){
  return smpi_info_f2c(info);
}

MPI_Fint PMPI_Info_c2f(MPI_Info info){
  return smpi_info_c2f(info);
}

int PMPI_Keyval_create(MPI_Copy_function* copy_fn, MPI_Delete_function* delete_fn, int* keyval, void* extra_state) {
  return smpi_comm_keyval_create(copy_fn, delete_fn, keyval, extra_state);
}

int PMPI_Keyval_free(int* keyval) {
  return smpi_comm_keyval_free(keyval);
}

int PMPI_Attr_delete(MPI_Comm comm, int keyval) {
  if(keyval == MPI_TAG_UB||keyval == MPI_HOST||keyval == MPI_IO ||keyval == MPI_WTIME_IS_GLOBAL||keyval == MPI_APPNUM
       ||keyval == MPI_UNIVERSE_SIZE||keyval == MPI_LASTUSEDCODE)
    return MPI_ERR_ARG;
  else if (comm==MPI_COMM_NULL)
    return MPI_ERR_COMM;
  else
    return smpi_comm_attr_delete(comm, keyval);
}

int PMPI_Attr_get(MPI_Comm comm, int keyval, void* attr_value, int* flag) {
  static int one = 1;
  static int zero = 0;
  static int tag_ub = 1000000;
  static int last_used_code = MPI_ERR_LASTCODE;

  if (comm==MPI_COMM_NULL){
    *flag = 0;
    return MPI_ERR_COMM;
  }

  switch (keyval) {
  case MPI_HOST:
  case MPI_IO:
  case MPI_APPNUM:
    *flag = 1;
    *(int**)attr_value = &zero;
    return MPI_SUCCESS;
  case MPI_UNIVERSE_SIZE:
    *flag = 1;
    *(int**)attr_value = &smpi_universe_size;
    return MPI_SUCCESS;
  case MPI_LASTUSEDCODE:
    *flag = 1;
    *(int**)attr_value = &last_used_code;
    return MPI_SUCCESS;
  case MPI_TAG_UB:
    *flag=1;
    *(int**)attr_value = &tag_ub;
    return MPI_SUCCESS;
  case MPI_WTIME_IS_GLOBAL:
    *flag = 1;
    *(int**)attr_value = &one;
    return MPI_SUCCESS;
  default:
    return smpi_comm_attr_get(comm, keyval, attr_value, flag);
  }
}

int PMPI_Attr_put(MPI_Comm comm, int keyval, void* attr_value) {
  if(keyval == MPI_TAG_UB||keyval == MPI_HOST||keyval == MPI_IO ||keyval == MPI_WTIME_IS_GLOBAL||keyval == MPI_APPNUM
       ||keyval == MPI_UNIVERSE_SIZE||keyval == MPI_LASTUSEDCODE)
    return MPI_ERR_ARG;
  else if (comm==MPI_COMM_NULL)
    return MPI_ERR_COMM;
  else
  return smpi_comm_attr_put(comm, keyval, attr_value);
}

int PMPI_Comm_get_attr (MPI_Comm comm, int comm_keyval, void *attribute_val, int *flag)
{
  return PMPI_Attr_get(comm, comm_keyval, attribute_val,flag);
}

int PMPI_Comm_set_attr (MPI_Comm comm, int comm_keyval, void *attribute_val)
{
  return PMPI_Attr_put(comm, comm_keyval, attribute_val);
}

int PMPI_Comm_delete_attr (MPI_Comm comm, int comm_keyval)
{
  return PMPI_Attr_delete(comm, comm_keyval);
}

int PMPI_Comm_create_keyval(MPI_Comm_copy_attr_function* copy_fn, MPI_Comm_delete_attr_function* delete_fn, int* keyval,
                            void* extra_state)
{
  return PMPI_Keyval_create(copy_fn, delete_fn, keyval, extra_state);
}

int PMPI_Comm_free_keyval(int* keyval) {
  return PMPI_Keyval_free(keyval);
}

int PMPI_Type_get_attr (MPI_Datatype type, int type_keyval, void *attribute_val, int* flag)
{
  if (type==MPI_DATATYPE_NULL)
    return MPI_ERR_TYPE;
  else
    return smpi_type_attr_get(type, type_keyval, attribute_val, flag);
}

int PMPI_Type_set_attr (MPI_Datatype type, int type_keyval, void *attribute_val)
{
  if (type==MPI_DATATYPE_NULL)
    return MPI_ERR_TYPE;
  else
    return smpi_type_attr_put(type, type_keyval, attribute_val);
}

int PMPI_Type_delete_attr (MPI_Datatype type, int type_keyval)
{
  if (type==MPI_DATATYPE_NULL)
    return MPI_ERR_TYPE;
  else
    return smpi_type_attr_delete(type, type_keyval);
}

int PMPI_Type_create_keyval(MPI_Type_copy_attr_function* copy_fn, MPI_Type_delete_attr_function* delete_fn, int* keyval,
                            void* extra_state)
{
  return smpi_type_keyval_create(copy_fn, delete_fn, keyval, extra_state);
}

int PMPI_Type_free_keyval(int* keyval) {
  return smpi_type_keyval_free(keyval);
}

int PMPI_Info_create( MPI_Info *info){
  if (info == NULL)
    return MPI_ERR_ARG;
  *info = xbt_new(s_smpi_mpi_info_t, 1);
  (*info)->info_dict= xbt_dict_new_homogeneous(NULL);
  (*info)->refcount=1;
  return MPI_SUCCESS;
}

int PMPI_Info_set( MPI_Info info, char *key, char *value){
  if (info == NULL || key == NULL || value == NULL)
    return MPI_ERR_ARG;

  xbt_dict_set(info->info_dict, key, (void*)value, NULL);
  return MPI_SUCCESS;
}

int PMPI_Info_free( MPI_Info *info){
  if (info == NULL || *info==NULL)
    return MPI_ERR_ARG;
  (*info)->refcount--;
  if((*info)->refcount==0){
    xbt_dict_free(&((*info)->info_dict));
    xbt_free(*info);
  }
  *info=MPI_INFO_NULL;
  return MPI_SUCCESS;
}

int PMPI_Info_get(MPI_Info info,char *key,int valuelen, char *value, int *flag){
  *flag=false;
  if (info == NULL || key == NULL || valuelen <0)
    return MPI_ERR_ARG;
  if (value == NULL)
    return MPI_ERR_INFO_VALUE;
  char* tmpvalue=(char*)xbt_dict_get_or_null(info->info_dict, key);
  if(tmpvalue){
    memcpy(value,tmpvalue, (strlen(tmpvalue) + 1 < static_cast<size_t>(valuelen)) ? strlen(tmpvalue) + 1 : valuelen);
    *flag=true;
  }
  return MPI_SUCCESS;
}

int PMPI_Info_dup(MPI_Info info, MPI_Info *newinfo){
  if (info == NULL || newinfo==NULL)
    return MPI_ERR_ARG;
  *newinfo = xbt_new(s_smpi_mpi_info_t, 1);
  (*newinfo)->info_dict= xbt_dict_new_homogeneous(NULL);
  xbt_dict_cursor_t cursor = NULL;
  int *key;
  void* data;
  xbt_dict_foreach(info->info_dict,cursor,key,data){
    xbt_dict_set((*newinfo)->info_dict, (char*)key, data, NULL);
  }
  return MPI_SUCCESS;
}

int PMPI_Info_delete(MPI_Info info, char *key){
  xbt_ex_t e;
  if (info == NULL || key==NULL)
    return MPI_ERR_ARG;
  TRY {
    xbt_dict_remove(info->info_dict, key);
  }CATCH(e){
    xbt_ex_free(e);
    return MPI_ERR_INFO_NOKEY;
  }
  return MPI_SUCCESS;
}

int PMPI_Info_get_nkeys( MPI_Info info, int *nkeys){
  if (info == NULL || nkeys==NULL)
    return MPI_ERR_ARG;
  *nkeys=xbt_dict_size(info->info_dict);
  return MPI_SUCCESS;
}

int PMPI_Info_get_nthkey( MPI_Info info, int n, char *key){
  if (info == NULL || key==NULL || n<0 || n> MPI_MAX_INFO_KEY)
    return MPI_ERR_ARG;

  xbt_dict_cursor_t cursor = NULL;
  char *keyn;
  void* data;
  int num=0;
  xbt_dict_foreach(info->info_dict,cursor,keyn,data){
    if(num==n){
     strcpy(key,keyn);
      return MPI_SUCCESS;
    }
    num++;
  }
  return MPI_ERR_ARG;
}

int PMPI_Info_get_valuelen( MPI_Info info, char *key, int *valuelen, int *flag){
  *flag=false;
  if (info == NULL || key == NULL || valuelen==NULL || *valuelen <0)
    return MPI_ERR_ARG;
  char* tmpvalue=(char*)xbt_dict_get_or_null(info->info_dict, key);
  if(tmpvalue){
    *valuelen=strlen(tmpvalue);
    *flag=true;
  }
  return MPI_SUCCESS;
}

int PMPI_Unpack(void* inbuf, int incount, int* position, void* outbuf, int outcount, MPI_Datatype type, MPI_Comm comm) {
  if(incount<0 || outcount < 0 || inbuf==NULL || outbuf==NULL)
    return MPI_ERR_ARG;
  if(!is_datatype_valid(type))
    return MPI_ERR_TYPE;
  if(comm==MPI_COMM_NULL)
    return MPI_ERR_COMM;
  return smpi_mpi_unpack(inbuf, incount, position, outbuf,outcount,type, comm);
}

int PMPI_Pack(void* inbuf, int incount, MPI_Datatype type, void* outbuf, int outcount, int* position, MPI_Comm comm) {
  if(incount<0 || outcount < 0|| inbuf==NULL || outbuf==NULL)
    return MPI_ERR_ARG;
  if(!is_datatype_valid(type))
    return MPI_ERR_TYPE;
  if(comm==MPI_COMM_NULL)
    return MPI_ERR_COMM;
  return smpi_mpi_pack(inbuf, incount, type, outbuf,outcount,position, comm);
}

int PMPI_Pack_size(int incount, MPI_Datatype datatype, MPI_Comm comm, int* size) {
  if(incount<0)
    return MPI_ERR_ARG;
  if(!is_datatype_valid(datatype))
    return MPI_ERR_TYPE;
  if(comm==MPI_COMM_NULL)
    return MPI_ERR_COMM;

  *size=incount*smpi_datatype_size(datatype);

  return MPI_SUCCESS;
}


/* The following calls are not yet implemented and will fail at runtime. */
/* Once implemented, please move them above this notice. */

#define NOT_YET_IMPLEMENTED {                                           \
    XBT_WARN("Not yet implemented : %s. Please contact the Simgrid team if support is needed", __FUNCTION__); \
    return MPI_SUCCESS;                                                 \
  }

MPI_Errhandler PMPI_Errhandler_f2c(MPI_Fint errhandler){
  NOT_YET_IMPLEMENTED
}

MPI_Fint PMPI_Errhandler_c2f(MPI_Errhandler errhandler){
  NOT_YET_IMPLEMENTED
}

int PMPI_Cart_map(MPI_Comm comm_old, int ndims, int* dims, int* periods, int* newrank) {
  NOT_YET_IMPLEMENTED
}

int PMPI_Graph_create(MPI_Comm comm_old, int nnodes, int* index, int* edges, int reorder, MPI_Comm* comm_graph) {
  NOT_YET_IMPLEMENTED
}

int PMPI_Graph_get(MPI_Comm comm, int maxindex, int maxedges, int* index, int* edges) {
  NOT_YET_IMPLEMENTED
}

int PMPI_Graph_map(MPI_Comm comm_old, int nnodes, int* index, int* edges, int* newrank) {
  NOT_YET_IMPLEMENTED
}

int PMPI_Graph_neighbors(MPI_Comm comm, int rank, int maxneighbors, int* neighbors) {
  NOT_YET_IMPLEMENTED
}

int PMPI_Graph_neighbors_count(MPI_Comm comm, int rank, int* nneighbors) {
  NOT_YET_IMPLEMENTED
}

int PMPI_Graphdims_get(MPI_Comm comm, int* nnodes, int* nedges) {
  NOT_YET_IMPLEMENTED
}

int PMPI_Topo_test(MPI_Comm comm, int* top_type) {
  NOT_YET_IMPLEMENTED
}

int PMPI_Errhandler_create(MPI_Handler_function* function, MPI_Errhandler* errhandler) {
  NOT_YET_IMPLEMENTED
}

int PMPI_Errhandler_free(MPI_Errhandler* errhandler) {
  NOT_YET_IMPLEMENTED
}

int PMPI_Errhandler_get(MPI_Comm comm, MPI_Errhandler* errhandler) {
  NOT_YET_IMPLEMENTED
}

int PMPI_Error_string(int errorcode, char* string, int* resultlen) {
  NOT_YET_IMPLEMENTED
}

int PMPI_Errhandler_set(MPI_Comm comm, MPI_Errhandler errhandler) {
  NOT_YET_IMPLEMENTED
}

int PMPI_Comm_set_errhandler(MPI_Comm comm, MPI_Errhandler errhandler) {
  NOT_YET_IMPLEMENTED
}

int PMPI_Win_set_errhandler(MPI_Win win, MPI_Errhandler errhandler) {
  NOT_YET_IMPLEMENTED
}

int PMPI_Comm_get_errhandler(MPI_Comm comm, MPI_Errhandler* errhandler) {
  NOT_YET_IMPLEMENTED
}

int PMPI_Cancel(MPI_Request* request) {
  NOT_YET_IMPLEMENTED
}

int PMPI_Buffer_attach(void* buffer, int size) {
  NOT_YET_IMPLEMENTED
}

int PMPI_Buffer_detach(void* buffer, int* size) {
  NOT_YET_IMPLEMENTED
}

int PMPI_Comm_test_inter(MPI_Comm comm, int* flag) {
  NOT_YET_IMPLEMENTED
}

int PMPI_Pcontrol(const int level )
{
  NOT_YET_IMPLEMENTED
}

int PMPI_Intercomm_create(MPI_Comm local_comm, int local_leader, MPI_Comm peer_comm, int remote_leader, int tag,
                          MPI_Comm* comm_out) {
  NOT_YET_IMPLEMENTED
}

int PMPI_Intercomm_merge(MPI_Comm comm, int high, MPI_Comm* comm_out) {
  NOT_YET_IMPLEMENTED
}

int PMPI_Bsend(void* buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm) {
  NOT_YET_IMPLEMENTED
}

int PMPI_Bsend_init(void* buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm,
                    MPI_Request* request) {
  NOT_YET_IMPLEMENTED
}

int PMPI_Ibsend(void* buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm, MPI_Request* request) {
  NOT_YET_IMPLEMENTED
}

int PMPI_Comm_remote_group(MPI_Comm comm, MPI_Group* group) {
  NOT_YET_IMPLEMENTED
}

int PMPI_Comm_remote_size(MPI_Comm comm, int* size) {
  NOT_YET_IMPLEMENTED
}

int PMPI_Rsend(void* buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm) {
  NOT_YET_IMPLEMENTED
}

int PMPI_Rsend_init(void* buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm,
                    MPI_Request* request) {
  NOT_YET_IMPLEMENTED
}

int PMPI_Irsend(void* buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm, MPI_Request* request) {
  NOT_YET_IMPLEMENTED
}

int PMPI_Test_cancelled(MPI_Status* status, int* flag) {
  NOT_YET_IMPLEMENTED
}

int PMPI_Pack_external_size(char *datarep, int incount, MPI_Datatype datatype, MPI_Aint *size){
  NOT_YET_IMPLEMENTED
}

int PMPI_Pack_external(char *datarep, void *inbuf, int incount, MPI_Datatype datatype, void *outbuf, MPI_Aint outcount,
                       MPI_Aint *position){
  NOT_YET_IMPLEMENTED
}

int PMPI_Unpack_external(char *datarep, void *inbuf, MPI_Aint insize, MPI_Aint *position, void *outbuf, int outcount,
                         MPI_Datatype datatype){
  NOT_YET_IMPLEMENTED
}

int PMPI_Get_elements(MPI_Status* status, MPI_Datatype datatype, int* elements) {
  NOT_YET_IMPLEMENTED
}

int PMPI_Type_get_envelope(MPI_Datatype datatype, int *num_integers, int *num_addresses, int *num_datatypes,
                           int *combiner){
  NOT_YET_IMPLEMENTED
}

int PMPI_Type_get_contents(MPI_Datatype datatype, int max_integers, int max_addresses, int max_datatypes,
                           int* array_of_integers, MPI_Aint* array_of_addresses, MPI_Datatype* array_of_datatypes){
  NOT_YET_IMPLEMENTED
}

int PMPI_Type_create_darray(int size, int rank, int ndims, int* array_of_gsizes, int* array_of_distribs,
                            int* array_of_dargs, int* array_of_psizes,int order, MPI_Datatype oldtype,
                            MPI_Datatype *newtype) {
  NOT_YET_IMPLEMENTED
}

int PMPI_Type_create_subarray(int ndims,int *array_of_sizes, int *array_of_subsizes, int *array_of_starts, int order,
                              MPI_Datatype oldtype, MPI_Datatype *newtype){
  NOT_YET_IMPLEMENTED
}

int PMPI_Type_match_size(int typeclass,int size,MPI_Datatype *datatype){
  NOT_YET_IMPLEMENTED
}

int PMPI_Alltoallw( void *sendbuf, int *sendcnts, int *sdispls, MPI_Datatype *sendtypes,
                    void *recvbuf, int *recvcnts, int *rdispls, MPI_Datatype *recvtypes, MPI_Comm comm){
  NOT_YET_IMPLEMENTED
}

int PMPI_Comm_set_name(MPI_Comm comm, char* name){
  NOT_YET_IMPLEMENTED
}

int PMPI_Comm_dup_with_info(MPI_Comm comm, MPI_Info info, MPI_Comm * newcomm){
  NOT_YET_IMPLEMENTED
}

int PMPI_Comm_split_type(MPI_Comm comm, int split_type, int key, MPI_Info info, MPI_Comm *newcomm){
  NOT_YET_IMPLEMENTED
}

int PMPI_Comm_set_info (MPI_Comm comm, MPI_Info info){
  NOT_YET_IMPLEMENTED
}

int PMPI_Comm_get_info (MPI_Comm comm, MPI_Info* info){
  NOT_YET_IMPLEMENTED
}

int PMPI_Comm_create_errhandler( MPI_Comm_errhandler_fn *function, MPI_Errhandler *errhandler){
  NOT_YET_IMPLEMENTED
}

int PMPI_Add_error_class( int *errorclass){
  NOT_YET_IMPLEMENTED
}

int PMPI_Add_error_code(  int errorclass, int *errorcode){
  NOT_YET_IMPLEMENTED
}

int PMPI_Add_error_string( int errorcode, char *string){
  NOT_YET_IMPLEMENTED
}

int PMPI_Comm_call_errhandler(MPI_Comm comm,int errorcode){
  NOT_YET_IMPLEMENTED
}

int PMPI_Request_get_status( MPI_Request request, int *flag, MPI_Status *status){
  NOT_YET_IMPLEMENTED
}

int PMPI_Grequest_start(MPI_Grequest_query_function *query_fn, MPI_Grequest_free_function *free_fn,
                        MPI_Grequest_cancel_function *cancel_fn, void *extra_state, MPI_Request *request){
  NOT_YET_IMPLEMENTED
}

int PMPI_Grequest_complete( MPI_Request request){
  NOT_YET_IMPLEMENTED
}

int PMPI_Status_set_cancelled(MPI_Status *status,int flag){
  NOT_YET_IMPLEMENTED
}

int PMPI_Status_set_elements( MPI_Status *status, MPI_Datatype datatype, int count){
  NOT_YET_IMPLEMENTED
}

int PMPI_Comm_connect( char *port_name, MPI_Info info, int root, MPI_Comm comm, MPI_Comm *newcomm){
  NOT_YET_IMPLEMENTED
}

int PMPI_Publish_name( char *service_name, MPI_Info info, char *port_name){
  NOT_YET_IMPLEMENTED
}

int PMPI_Unpublish_name( char *service_name, MPI_Info info, char *port_name){
  NOT_YET_IMPLEMENTED
}

int PMPI_Lookup_name( char *service_name, MPI_Info info, char *port_name){
  NOT_YET_IMPLEMENTED
}

int PMPI_Comm_join( int fd, MPI_Comm *intercomm){
  NOT_YET_IMPLEMENTED
}

int PMPI_Open_port( MPI_Info info, char *port_name){
  NOT_YET_IMPLEMENTED
}

int PMPI_Close_port(char *port_name){
  NOT_YET_IMPLEMENTED
}

int PMPI_Comm_accept( char *port_name, MPI_Info info, int root, MPI_Comm comm, MPI_Comm *newcomm){
  NOT_YET_IMPLEMENTED
}

int PMPI_Comm_spawn(char *command, char **argv, int maxprocs, MPI_Info info, int root, MPI_Comm comm,
                    MPI_Comm *intercomm, int* array_of_errcodes){
  NOT_YET_IMPLEMENTED
}

int PMPI_Comm_spawn_multiple( int count, char **array_of_commands, char*** array_of_argv,
                              int* array_of_maxprocs, MPI_Info* array_of_info, int root,
                              MPI_Comm comm, MPI_Comm *intercomm, int* array_of_errcodes){
  NOT_YET_IMPLEMENTED
}

int PMPI_Comm_get_parent( MPI_Comm *parent){
  NOT_YET_IMPLEMENTED
}

int PMPI_Win_lock(int lock_type, int rank, int assert, MPI_Win win) {
  NOT_YET_IMPLEMENTED
}

int PMPI_Win_test(MPI_Win win, int *flag){
  NOT_YET_IMPLEMENTED
}

int PMPI_Win_unlock(int rank, MPI_Win win){
  NOT_YET_IMPLEMENTED
}
