/* smpi_coll.c -- various optimized routing for collectives                   */

/* Copyright (c) 2009, 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "private.h"
#include "colls/colls.h"

s_mpi_coll_description_t mpi_coll_allgather_description[] = {
  {"default",
   "allgather default collective",
   smpi_mpi_allgather},
COLL_ALLGATHERS(COLL_DESCRIPTION, COLL_COMMA),
  {NULL, NULL, NULL}      /* this array must be NULL terminated */
};

s_mpi_coll_description_t mpi_coll_allreduce_description[] = {
  {"default",
   "allreduce default collective",
   smpi_mpi_allreduce},
COLL_ALLREDUCES(COLL_DESCRIPTION, COLL_COMMA),
  {NULL, NULL, NULL}      /* this array must be NULL terminated */
};

s_mpi_coll_description_t mpi_coll_alltoall_description[] = {
  {"ompi",
   "Ompi alltoall default collective",
   smpi_coll_tuned_alltoall_ompi},
COLL_ALLTOALLS(COLL_DESCRIPTION, COLL_COMMA),
  {"bruck",
   "Alltoall Bruck (SG) collective",
   smpi_coll_tuned_alltoall_bruck},
  {"basic_linear",
   "Alltoall basic linear (SG) collective",
   smpi_coll_tuned_alltoall_basic_linear},
  {"pairwise",
   "Alltoall pairwise (SG) collective",
   smpi_coll_tuned_alltoall_pairwise},
  {NULL, NULL, NULL}      /* this array must be NULL terminated */
};

s_mpi_coll_description_t mpi_coll_alltoallv_description[] = {
  {"default",
   "Ompi alltoallv default collective",
   smpi_coll_basic_alltoallv},
COLL_ALLTOALLVS(COLL_DESCRIPTION, COLL_COMMA),
  {NULL, NULL, NULL}      /* this array must be NULL terminated */
};

s_mpi_coll_description_t mpi_coll_bcast_description[] = {
  {"default",
   "allgather default collective",
   smpi_mpi_bcast},
COLL_BCASTS(COLL_DESCRIPTION, COLL_COMMA),
  {NULL, NULL, NULL}      /* this array must be NULL terminated */
};

s_mpi_coll_description_t mpi_coll_reduce_description[] = {
  {"default",
   "allgather default collective",
   smpi_mpi_reduce},
COLL_REDUCES(COLL_DESCRIPTION, COLL_COMMA),
  {NULL, NULL, NULL}      /* this array must be NULL terminated */
};



/** Displays the long description of all registered models, and quit */
void coll_help(const char *category, s_mpi_coll_description_t * table)
{
  int i;
  printf("Long description of the %s models accepted by this simulator:\n",
         category);
  for (i = 0; table[i].name; i++)
    printf("  %s: %s\n", table[i].name, table[i].description);
}

int find_coll_description(s_mpi_coll_description_t * table,
                           const char *name)
{
  int i;
  char *name_list = NULL;

  for (i = 0; table[i].name; i++)
    if (!strcmp(name, table[i].name)) {
      return i;
    }
  name_list = strdup(table[0].name);
  for (i = 1; table[i].name; i++) {
    name_list =
        xbt_realloc(name_list,
                    strlen(name_list) + strlen(table[i].name) + 3);
    strcat(name_list, ", ");
    strcat(name_list, table[i].name);
  }
  xbt_die("Model '%s' is invalid! Valid models are: %s.", name, name_list);
  return -1;
}



XBT_LOG_NEW_DEFAULT_SUBCATEGORY(smpi_coll, smpi,
                                "Logging specific to SMPI (coll)");

int (*mpi_coll_allgather_fun)(void *, int, MPI_Datatype, void*, int, MPI_Datatype, MPI_Comm);
int (*mpi_coll_allreduce_fun)(void *sbuf, void *rbuf, int rcount, MPI_Datatype dtype, MPI_Op op, MPI_Comm comm);
int (*mpi_coll_alltoall_fun)(void *, int, MPI_Datatype, void*, int, MPI_Datatype, MPI_Comm);
int (*mpi_coll_alltoallv_fun)(void *, int*, int*, MPI_Datatype, void*, int*, int*, MPI_Datatype, MPI_Comm);
int (*mpi_coll_bcast_fun)(void *buf, int count, MPI_Datatype datatype, int root, MPI_Comm com);
int (*mpi_coll_reduce_fun)(void *buf, void *rbuf, int count, MPI_Datatype datatype, MPI_Op op, int root, MPI_Comm comm);

struct s_proc_tree {
  int PROCTREE_A;
  int numChildren;
  int *child;
  int parent;
  int me;
  int root;
  int isRoot;
};
typedef struct s_proc_tree *proc_tree_t;

/**
 * alloc and init
 **/
static proc_tree_t alloc_tree(int arity)
{
  proc_tree_t tree;
  int i;

  tree = xbt_new(struct s_proc_tree, 1);
  tree->PROCTREE_A = arity;
  tree->isRoot = 0;
  tree->numChildren = 0;
  tree->child = xbt_new(int, arity);
  for (i = 0; i < arity; i++) {
    tree->child[i] = -1;
  }
  tree->root = -1;
  tree->parent = -1;
  return tree;
}

/**
 * free
 **/
static void free_tree(proc_tree_t tree)
{
  xbt_free(tree->child);
  xbt_free(tree);
}

/**
 * Build the tree depending on a process rank (index) and the group size (extent)
 * @param root the rank of the tree root
 * @param rank the rank of the calling process
 * @param size the total number of processes
 **/
static void build_tree(int root, int rank, int size, proc_tree_t * tree)
{
  int index = (rank - root + size) % size;
  int firstChildIdx = index * (*tree)->PROCTREE_A + 1;
  int i;

  (*tree)->me = rank;
  (*tree)->root = root;

  for (i = 0; i < (*tree)->PROCTREE_A && firstChildIdx + i < size; i++) {
    (*tree)->child[i] = (firstChildIdx + i + root) % size;
    (*tree)->numChildren++;
  }
  if (rank == root) {
    (*tree)->isRoot = 1;
  } else {
    (*tree)->isRoot = 0;
    (*tree)->parent = (((index - 1) / (*tree)->PROCTREE_A) + root) % size;
  }
}

/**
 * bcast
 **/
static void tree_bcast(void *buf, int count, MPI_Datatype datatype,
                       MPI_Comm comm, proc_tree_t tree)
{
  int system_tag = 999;         // used negative int but smpi_create_request() declares this illegal (to be checked)
  int rank, i;
  MPI_Request *requests;

  rank = smpi_comm_rank(comm);
  /* wait for data from my parent in the tree */
  if (!tree->isRoot) {
    XBT_DEBUG("<%d> tree_bcast(): i am not root: recv from %d, tag=%d)",
           rank, tree->parent, system_tag + rank);
    smpi_mpi_recv(buf, count, datatype, tree->parent, system_tag + rank,
                  comm, MPI_STATUS_IGNORE);
  }
  requests = xbt_new(MPI_Request, tree->numChildren);
  XBT_DEBUG("<%d> creates %d requests (1 per child)", rank,
         tree->numChildren);
  /* iniates sends to ranks lower in the tree */
  for (i = 0; i < tree->numChildren; i++) {
    if (tree->child[i] == -1) {
      requests[i] = MPI_REQUEST_NULL;
    } else {
      XBT_DEBUG("<%d> send to <%d>, tag=%d", rank, tree->child[i],
             system_tag + tree->child[i]);
      requests[i] =
          smpi_isend_init(buf, count, datatype, tree->child[i],
                          system_tag + tree->child[i], comm);
    }
  }
  smpi_mpi_startall(tree->numChildren, requests);
  smpi_mpi_waitall(tree->numChildren, requests, MPI_STATUS_IGNORE);
  xbt_free(requests);
}

/**
 * anti-bcast
 **/
static void tree_antibcast(void *buf, int count, MPI_Datatype datatype,
                           MPI_Comm comm, proc_tree_t tree)
{
  int system_tag = 999;         // used negative int but smpi_create_request() declares this illegal (to be checked)
  int rank, i;
  MPI_Request *requests;

  rank = smpi_comm_rank(comm);
  // everyone sends to its parent, except root.
  if (!tree->isRoot) {
    XBT_DEBUG("<%d> tree_antibcast(): i am not root: send to %d, tag=%d)",
           rank, tree->parent, system_tag + rank);
    smpi_mpi_send(buf, count, datatype, tree->parent, system_tag + rank,
                  comm);
  }
  //every one receives as many messages as it has children
  requests = xbt_new(MPI_Request, tree->numChildren);
  XBT_DEBUG("<%d> creates %d requests (1 per child)", rank,
         tree->numChildren);
  for (i = 0; i < tree->numChildren; i++) {
    if (tree->child[i] == -1) {
      requests[i] = MPI_REQUEST_NULL;
    } else {
      XBT_DEBUG("<%d> recv from <%d>, tag=%d", rank, tree->child[i],
             system_tag + tree->child[i]);
      requests[i] =
          smpi_irecv_init(buf, count, datatype, tree->child[i],
                          system_tag + tree->child[i], comm);
    }
  }
  smpi_mpi_startall(tree->numChildren, requests);
  smpi_mpi_waitall(tree->numChildren, requests, MPI_STATUS_IGNORE);
  xbt_free(requests);
}

/**
 * bcast with a binary, ternary, or whatever tree ..
 **/
void nary_tree_bcast(void *buf, int count, MPI_Datatype datatype, int root,
                     MPI_Comm comm, int arity)
{
  proc_tree_t tree = alloc_tree(arity);
  int rank, size;

  rank = smpi_comm_rank(comm);
  size = smpi_comm_size(comm);
  build_tree(root, rank, size, &tree);
  tree_bcast(buf, count, datatype, comm, tree);
  free_tree(tree);
}

/**
 * barrier with a binary, ternary, or whatever tree ..
 **/
void nary_tree_barrier(MPI_Comm comm, int arity)
{
  proc_tree_t tree = alloc_tree(arity);
  int rank, size;
  char dummy = '$';

  rank = smpi_comm_rank(comm);
  size = smpi_comm_size(comm);
  build_tree(0, rank, size, &tree);
  tree_antibcast(&dummy, 1, MPI_CHAR, comm, tree);
  tree_bcast(&dummy, 1, MPI_CHAR, comm, tree);
  free_tree(tree);
}

int smpi_coll_tuned_alltoall_ompi(void *sendbuf, int sendcount,
                                   MPI_Datatype sendtype, void *recvbuf,
                                   int recvcount, MPI_Datatype recvtype,
                                   MPI_Comm comm)
{
  int size, sendsize;	
  size = smpi_comm_size(comm);	
  sendsize = smpi_datatype_size(sendtype) * sendcount;	
  if (sendsize < 200 && size > 12) {
    return
        smpi_coll_tuned_alltoall_bruck(sendbuf, sendcount, sendtype,
                                       recvbuf, recvcount, recvtype,
                                       comm);
  } else if (sendsize < 3000) {
    return
        smpi_coll_tuned_alltoall_basic_linear(sendbuf, sendcount,
                                              sendtype, recvbuf,
                                              recvcount, recvtype, comm);
  } else {
    return
        smpi_coll_tuned_alltoall_pairwise(sendbuf, sendcount, sendtype,
                                          recvbuf, recvcount, recvtype,
                                          comm);
  }
}

/**
 * Alltoall Bruck
 *
 * Openmpi calls this routine when the message size sent to each rank < 2000 bytes and size < 12
 * FIXME: uh, check smpi_pmpi again, but this routine is called for > 12, not
 * less...
 **/
int smpi_coll_tuned_alltoall_bruck(void *sendbuf, int sendcount,
                                   MPI_Datatype sendtype, void *recvbuf,
                                   int recvcount, MPI_Datatype recvtype,
                                   MPI_Comm comm)
{
  int system_tag = 777;
  int i, rank, size, err, count;
  MPI_Aint lb;
  MPI_Aint sendext = 0;
  MPI_Aint recvext = 0;
  MPI_Request *requests;

  // FIXME: check implementation
  rank = smpi_comm_rank(comm);
  size = smpi_comm_size(comm);
  XBT_DEBUG("<%d> algorithm alltoall_bruck() called.", rank);
  err = smpi_datatype_extent(sendtype, &lb, &sendext);
  err = smpi_datatype_extent(recvtype, &lb, &recvext);
  /* Local copy from self */
  err =
      smpi_datatype_copy((char *)sendbuf + rank * sendcount * sendext, 
                         sendcount, sendtype, 
                         (char *)recvbuf + rank * recvcount * recvext,
                         recvcount, recvtype);
  if (err == MPI_SUCCESS && size > 1) {
    /* Initiate all send/recv to/from others. */
    requests = xbt_new(MPI_Request, 2 * (size - 1));
    count = 0;
    /* Create all receives that will be posted first */
    for (i = 0; i < size; ++i) {
      if (i == rank) {
        XBT_DEBUG("<%d> skip request creation [src = %d, recvcount = %d]",
               rank, i, recvcount);
        continue;
      }
      requests[count] =
          smpi_irecv_init((char *)recvbuf + i * recvcount * recvext, recvcount,
                          recvtype, i, system_tag, comm);
      count++;
    }
    /* Now create all sends  */
    for (i = 0; i < size; ++i) {
      if (i == rank) {
        XBT_DEBUG("<%d> skip request creation [dst = %d, sendcount = %d]",
               rank, i, sendcount);
        continue;
      }
      requests[count] =
          smpi_isend_init((char *)sendbuf + i * sendcount * sendext, sendcount,
                          sendtype, i, system_tag, comm);
      count++;
    }
    /* Wait for them all. */
    smpi_mpi_startall(count, requests);
    XBT_DEBUG("<%d> wait for %d requests", rank, count);
    smpi_mpi_waitall(count, requests, MPI_STATUS_IGNORE);
    xbt_free(requests);
  }
  return MPI_SUCCESS;
}

/**
 * Alltoall basic_linear (STARMPI:alltoall-simple)
 **/
int smpi_coll_tuned_alltoall_basic_linear(void *sendbuf, int sendcount,
                                          MPI_Datatype sendtype,
                                          void *recvbuf, int recvcount,
                                          MPI_Datatype recvtype,
                                          MPI_Comm comm)
{
  int system_tag = 888;
  int i, rank, size, err, count;
  MPI_Aint lb = 0, sendext = 0, recvext = 0;
  MPI_Request *requests;

  /* Initialize. */
  rank = smpi_comm_rank(comm);
  size = smpi_comm_size(comm);
  XBT_DEBUG("<%d> algorithm alltoall_basic_linear() called.", rank);
  err = smpi_datatype_extent(sendtype, &lb, &sendext);
  err = smpi_datatype_extent(recvtype, &lb, &recvext);
  /* simple optimization */
  err = smpi_datatype_copy((char *)sendbuf + rank * sendcount * sendext, 
                           sendcount, sendtype, 
                           (char *)recvbuf + rank * recvcount * recvext, 
                           recvcount, recvtype);
  if (err == MPI_SUCCESS && size > 1) {
    /* Initiate all send/recv to/from others. */
    requests = xbt_new(MPI_Request, 2 * (size - 1));
    /* Post all receives first -- a simple optimization */
    count = 0;
    for (i = (rank + 1) % size; i != rank; i = (i + 1) % size) {
      requests[count] =
          smpi_irecv_init((char *)recvbuf + i * recvcount * recvext, recvcount, 
                          recvtype, i, system_tag, comm);
      count++;
    }
    /* Now post all sends in reverse order
     *   - We would like to minimize the search time through message queue
     *     when messages actually arrive in the order in which they were posted.
     * TODO: check the previous assertion
     */
    for (i = (rank + size - 1) % size; i != rank; i = (i + size - 1) % size) {
      requests[count] =
          smpi_isend_init((char *)sendbuf + i * sendcount * sendext, sendcount,
                          sendtype, i, system_tag, comm);
      count++;
    }
    /* Wait for them all. */
    smpi_mpi_startall(count, requests);
    XBT_DEBUG("<%d> wait for %d requests", rank, count);
    smpi_mpi_waitall(count, requests, MPI_STATUS_IGNORE);
    xbt_free(requests);
  }
  return err;
}

/**
 * Alltoall pairwise
 *
 * this algorithm performs size steps (1<=s<=size) and
 * at each step s, a process p sends iand receive to.from a unique distinct remote process
 * size=5 : s=1:  4->0->1, 0->1->2, 1->2->3, ...
 *          s=2:  3->0->2, 4->1->3, 0->2->4, 1->3->0 , 2->4->1
 *          ....
 * Openmpi calls this routine when the message size sent to each rank is greater than 3000 bytes
 **/
int smpi_coll_tuned_alltoall_pairwise(void *sendbuf, int sendcount,
                                      MPI_Datatype sendtype, void *recvbuf,
                                      int recvcount, MPI_Datatype recvtype,
                                      MPI_Comm comm)
{
  int system_tag = 999;
  int rank, size, step, sendto, recvfrom, sendsize, recvsize;

  rank = smpi_comm_rank(comm);
  size = smpi_comm_size(comm);
  XBT_DEBUG("<%d> algorithm alltoall_pairwise() called.", rank);
  sendsize = smpi_datatype_size(sendtype);
  recvsize = smpi_datatype_size(recvtype);
  /* Perform pairwise exchange - starting from 1 so the local copy is last */
  for (step = 1; step < size + 1; step++) {
    /* who do we talk to in this step? */
    sendto = (rank + step) % size;
    recvfrom = (rank + size - step) % size;
    /* send and receive */
    smpi_mpi_sendrecv(&((char *) sendbuf)[sendto * sendsize * sendcount],
                      sendcount, sendtype, sendto, system_tag,
                      &((char *) recvbuf)[recvfrom * recvsize * recvcount],
                      recvcount, recvtype, recvfrom, system_tag, comm,
                      MPI_STATUS_IGNORE);
  }
  return MPI_SUCCESS;
}

int smpi_coll_basic_alltoallv(void *sendbuf, int *sendcounts,
                              int *senddisps, MPI_Datatype sendtype,
                              void *recvbuf, int *recvcounts,
                              int *recvdisps, MPI_Datatype recvtype,
                              MPI_Comm comm)
{
  int system_tag = 889;
  int i, rank, size, err, count;
  MPI_Aint lb = 0, sendext = 0, recvext = 0;
  MPI_Request *requests;

  /* Initialize. */
  rank = smpi_comm_rank(comm);
  size = smpi_comm_size(comm);
  XBT_DEBUG("<%d> algorithm basic_alltoallv() called.", rank);
  err = smpi_datatype_extent(sendtype, &lb, &sendext);
  err = smpi_datatype_extent(recvtype, &lb, &recvext);
  /* Local copy from self */
  err =
      smpi_datatype_copy((char *)sendbuf + senddisps[rank] * sendext, 
                         sendcounts[rank], sendtype,
                         (char *)recvbuf + recvdisps[rank] * recvext, 
                         recvcounts[rank], recvtype);
  if (err == MPI_SUCCESS && size > 1) {
    /* Initiate all send/recv to/from others. */
    requests = xbt_new(MPI_Request, 2 * (size - 1));
    count = 0;
    /* Create all receives that will be posted first */
    for (i = 0; i < size; ++i) {
      if (i == rank || recvcounts[i] == 0) {
        XBT_DEBUG
            ("<%d> skip request creation [src = %d, recvcounts[src] = %d]",
             rank, i, recvcounts[i]);
        continue;
      }
      requests[count] =
          smpi_irecv_init((char *)recvbuf + recvdisps[i] * recvext, 
                          recvcounts[i], recvtype, i, system_tag, comm);
      count++;
    }
    /* Now create all sends  */
    for (i = 0; i < size; ++i) {
      if (i == rank || sendcounts[i] == 0) {
        XBT_DEBUG
            ("<%d> skip request creation [dst = %d, sendcounts[dst] = %d]",
             rank, i, sendcounts[i]);
        continue;
      }
      requests[count] =
          smpi_isend_init((char *)sendbuf + senddisps[i] * sendext, 
                          sendcounts[i], sendtype, i, system_tag, comm);
      count++;
    }
    /* Wait for them all. */
    smpi_mpi_startall(count, requests);
    XBT_DEBUG("<%d> wait for %d requests", rank, count);
    smpi_mpi_waitall(count, requests, MPI_STATUS_IGNORE);
    xbt_free(requests);
  }
  return err;
}
