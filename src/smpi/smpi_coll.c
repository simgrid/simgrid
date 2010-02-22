/* $Id$tag */

/* smpi_coll.c -- various optimized routing for collectives                   */

/* Copyright (c) 2009 Stephane Genaud.                                        */
/* All rights reserved.                                                       */

/* This program is free software; you can redistribute it and/or modify it
 *  * under the terms of the license (GNU LGPL) which comes with this package. */

#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "private.h"
#include "smpi_coll_private.h"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(smpi_coll, smpi,
                                "Logging specific to SMPI (coll)");

struct s_proc_tree {
  int PROCTREE_A;
  int numChildren;
  int * child;
  int parent;
  int me;
  int root;
  int isRoot;
};
typedef struct s_proc_tree * proc_tree_t;

/**
 * alloc and init
 **/
static proc_tree_t alloc_tree(int arity) {
  proc_tree_t tree;
  int i;

  tree = xbt_new(struct s_proc_tree, 1);
  tree->PROCTREE_A = arity;
  tree->isRoot = 0; 
  tree->numChildren = 0;
  tree->child = xbt_new(int, arity);
  for(i = 0; i < arity; i++) {
    tree->child[i] = -1;
  }
  tree->root = -1;
  tree->parent = -1;
  return tree;
}

/**
 * free
 **/
static void free_tree(proc_tree_t tree) {
  xbt_free(tree->child );
  xbt_free(tree);
}

/**
 * Build the tree depending on a process rank (index) and the group size (extent)
 * @param index the rank of the calling process
 * @param extent the total number of processes
 **/
static void build_tree(int index, int extent, proc_tree_t* tree) {
  int places = (*tree)->PROCTREE_A * index;
  int i, ch, pr;

  (*tree)->me = index;
  (*tree)->root = 0 ;
  for(i = 1; i <= (*tree)->PROCTREE_A; i++) {
    ++places;
    ch = (*tree)->PROCTREE_A * index + i + (*tree)->root;
    ch %= extent;
    if(places < extent) {
      (*tree)->child[i - 1] = ch;
      (*tree)->numChildren++;
    }
  }
  if(index == (*tree)->root) {
    (*tree)->isRoot = 1;
  } else {
    (*tree)->isRoot = 0;
    pr = (index - 1) / (*tree)->PROCTREE_A;
    (*tree)->parent = pr;
  }
}

/**
 * bcast
 **/
static void tree_bcast(void* buf, int count, MPI_Datatype datatype, int root, MPI_Comm comm, proc_tree_t tree) {
  int system_tag = 999;  // used negative int but smpi_create_request() declares this illegal (to be checked)
  int rank, i;
  MPI_Request* requests;

  rank = smpi_comm_rank(comm);
  /* wait for data from my parent in the tree */
  if(!tree->isRoot) {
    DEBUG3("<%d> tree_bcast(): i am not root: recv from %d, tag=%d)",
           rank, tree->parent, system_tag + rank);
    smpi_mpi_recv(buf, count, datatype, tree->parent, system_tag + rank, comm, MPI_STATUS_IGNORE);
  }
  requests = xbt_new(MPI_Request, tree->numChildren);
  DEBUG2("<%d> creates %d requests (1 per child)\n", rank, tree->numChildren);
  /* iniates sends to ranks lower in the tree */
  for(i = 0; i < tree->numChildren; i++) {
    if(tree->child[i] == -1) {
      requests[i] = MPI_REQUEST_NULL;
    } else {
      DEBUG3("<%d> send to <%d>, tag=%d", rank, tree->child[i], system_tag + tree->child[i]);
      requests[i] = smpi_mpi_isend(buf, count, datatype, tree->child[i], system_tag + tree->child[i], comm);
    }
  }
  smpi_mpi_waitall(tree->numChildren, requests, MPI_STATUS_IGNORE);
  xbt_free(requests);
}

/**
 * anti-bcast
 **/
static void tree_antibcast(void* buf, int count, MPI_Datatype datatype, int root, MPI_Comm comm, proc_tree_t tree) {
  int system_tag = 999;  // used negative int but smpi_create_request() declares this illegal (to be checked)
  int rank, i;
  MPI_Request* requests;

  rank = smpi_comm_rank(comm);
  // everyone sends to its parent, except root.
  if(!tree->isRoot) {
    DEBUG3("<%d> tree_antibcast(): i am not root: send to %d, tag=%d)",
           rank, tree->parent, system_tag + rank);
    smpi_mpi_send(buf, count, datatype, tree->parent, system_tag + rank, comm);
  }
  //every one receives as many messages as it has children
  requests = xbt_new(MPI_Request, tree->numChildren);
  DEBUG2("<%d> creates %d requests (1 per child)\n", rank, tree->numChildren);
  for(i = 0; i < tree->numChildren; i++) {
    if(tree->child[i] == -1) {
      requests[i] = MPI_REQUEST_NULL;
    } else {
      DEBUG3("<%d> recv from <%d>, tag=%d", rank, tree->child[i], system_tag + tree->child[i]);
      requests[i] = smpi_mpi_irecv(buf, count, datatype, tree->child[i], system_tag + tree->child[i], comm);
    }
  }
  smpi_mpi_waitall(tree->numChildren, requests, MPI_STATUS_IGNORE);
  xbt_free(requests);
} 

/**
 * bcast with a binary, ternary, or whatever tree .. 
 **/
void nary_tree_bcast(void* buf, int count, MPI_Datatype datatype, int root, MPI_Comm comm, int arity) {
  proc_tree_t tree = alloc_tree(arity); 
  int rank, size;

  rank = smpi_comm_rank(comm);
  size = smpi_comm_size(comm);
  build_tree(rank, size, &tree);
  tree_bcast(buf, count, datatype, root, comm, tree);
  free_tree(tree);
}

/**
 * barrier with a binary, ternary, or whatever tree .. 
 **/
void nary_tree_barrier(MPI_Comm comm, int arity) {
  proc_tree_t tree = alloc_tree( arity ); 
  int rank, size;
  char dummy='$';

  rank = smpi_comm_rank(comm);
  size = smpi_comm_size(comm);
  build_tree(rank, size, &tree);
  tree_antibcast(&dummy, 1, MPI_CHAR, 0, comm, tree);
  tree_bcast(&dummy, 1, MPI_CHAR, 0, comm, tree);
  free_tree(tree);
}

/**
 * Alltoall Bruck 
 *
 * Openmpi calls this routine when the message size sent to each rank < 2000 bytes and size < 12
 **/
int smpi_coll_tuned_alltoall_bruck(void* sendbuf, int sendcount, MPI_Datatype sendtype, void* recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm) {
  DEBUG0("coll:tuned:alltoall_intra_bruck ** NOT IMPLEMENTED YET**");
  return MPI_SUCCESS;
}

/**
 * Alltoall basic_linear
 **/
int smpi_coll_tuned_alltoall_basic_linear(void *sendbuf, int sendcount, MPI_Datatype sendtype, void* recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm) {
  int system_tag = 888;
  int i, rank, size, err, count;
  MPI_Aint lb, sendinc, recvinc;
  MPI_Request *requests;

  /* Initialize. */
  rank = smpi_comm_rank(comm);
  size = smpi_comm_size(comm);
  DEBUG1("<%d> algorithm alltoall_basic_linear() called.", rank);
  err = smpi_datatype_extent(sendtype, &lb, &sendinc);
  err = smpi_datatype_extent(recvtype, &lb, &recvinc);
  sendinc *= sendcount;
  recvinc *= recvcount;
  /* simple optimization */
  err = smpi_datatype_copy(&((char*)sendbuf)[rank * sendinc], sendcount, sendtype, &((char*)recvbuf)[rank * recvinc], recvcount, recvtype);
  if(err == MPI_SUCCESS && size > 1) {
    /* Initiate all send/recv to/from others. */
    requests = xbt_new(MPI_Request, 2 * (size - 1));
    /* Post all receives first -- a simple optimization */
    count = 0;
    for(i = (rank + 1) % size; i != rank; i = (i + 1) % size) {
      requests[count] = smpi_mpi_irecv(&((char*)recvbuf)[i * recvinc], recvcount, recvtype, i, system_tag, comm);
      count++;
    }
    /* Now post all sends in reverse order 
     *   - We would like to minimize the search time through message queue
     *     when messages actually arrive in the order in which they were posted.
     * TODO: check the previous assertion
     */
    for(i = (rank + size - 1) % size; i != rank; i = (i + size - 1) % size ) {
      requests[count] = smpi_mpi_isend(&((char*)sendbuf)[i * sendinc], sendcount, sendtype, i, system_tag, comm);
      count++;
    }
    /* Wait for them all.  If there's an error, note that we don't
     * care what the error was -- just that there *was* an error.  The
     * PML will finish all requests, even if one or more of them fail.
     * i.e., by the end of this call, all the requests are free-able.
     * So free them anyway -- even if there was an error, and return
     * the error after we free everything.
     */
    DEBUG2("<%d> wait for %d requests", rank, count);
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
int smpi_coll_tuned_alltoall_pairwise(void* sendbuf, int sendcount, MPI_Datatype sendtype, void* recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm) {
  int system_tag = 999;
  int rank, size, step, sendto, recvfrom, sendsize, recvsize;

  rank = smpi_comm_rank(comm);
  size = smpi_comm_size(comm);
  DEBUG1("<%d> algorithm alltoall_pairwise() called.", rank);
  sendsize = smpi_datatype_size(sendtype);
  recvsize = smpi_datatype_size(recvtype);
  /* Perform pairwise exchange - starting from 1 so the local copy is last */
  for(step = 1; step < size + 1; step++) {
    /* who do we talk to in this step? */
    sendto  = (rank+step)%size;
    recvfrom = (rank+size-step)%size;
    /* send and receive */
    smpi_mpi_sendrecv(&((char*)sendbuf)[sendto * sendsize * sendcount], sendcount, sendtype, sendto, system_tag, &((char*)recvbuf)[recvfrom * recvsize * recvcount], recvcount, recvtype, recvfrom, system_tag, comm, MPI_STATUS_IGNORE);
  }
  return MPI_SUCCESS;
}

int smpi_coll_basic_alltoallv(void* sendbuf, int* sendcounts, int* senddisps, MPI_Datatype sendtype, void* recvbuf, int *recvcounts, int* recvdisps, MPI_Datatype recvtype, MPI_Comm comm) {
  int system_tag = 889;
  int i, rank, size, err, count;
  MPI_Aint lb, sendextent, recvextent;
  MPI_Request* requests;

  /* Initialize. */
  rank = smpi_comm_rank(comm);
  size = smpi_comm_size(comm);
  DEBUG1("<%d> algorithm basic_alltoallv() called.", rank);
  err = smpi_datatype_extent(sendtype, &lb, &sendextent);
  err = smpi_datatype_extent(recvtype, &lb, &recvextent);
  /* Local copy from self */
  err = smpi_datatype_copy(&((char*)sendbuf)[senddisps[rank] * sendextent], sendcounts[rank], sendtype, &((char*)recvbuf)[recvdisps[rank] * recvextent], recvcounts[rank], recvtype);
  if(err == MPI_SUCCESS && size > 1) {
    /* Initiate all send/recv to/from others. */
    requests = xbt_new(MPI_Request, 2 * (size - 1));
    count = 0;
    /* Create all receives that will be posted first */
    for(i = 0; i < size; ++i) {
      if(i == rank || recvcounts[i] == 0) {
        DEBUG3("<%d> skip request creation [src = %d, recvcounts[src] = %d]", rank, i, recvcounts[i]);
        continue;
      }
      requests[count] = smpi_mpi_irecv(&((char*)recvbuf)[recvdisps[i] * recvextent], recvcounts[i], recvtype, i, system_tag, comm);
      count++;
    }
    /* Now create all sends  */
    for(i = 0; i < size; ++i) {
      if(i == rank || sendcounts[i] == 0) {
        DEBUG3("<%d> skip request creation [dst = %d, sendcounts[dst] = %d]", rank, i, sendcounts[i]);
        continue;
      }
      requests[count] = smpi_mpi_isend(&((char*)sendbuf)[senddisps[i] * sendextent], sendcounts[i], sendtype, i, system_tag, comm);
      count++;
    }
    /* Wait for them all.  If there's an error, note that we don't
     * care what the error was -- just that there *was* an error.  The
     * PML will finish all requests, even if one or more of them fail.
     * i.e., by the end of this call, all the requests are free-able.
     * So free them anyway -- even if there was an error, and return
     * the error after we free everything.
     */
    DEBUG2("<%d> wait for %d requests", rank, count);
    smpi_mpi_waitall(count, requests, MPI_STATUS_IGNORE);
    xbt_free(requests);
  }
  return err;
}
