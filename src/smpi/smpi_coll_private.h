/**
 * $Id$tag 
 *
 * smpi_coll_private.h -- functions of smpi_coll.c that are exported to other SMPI modules.
 *
 *
 *
 **/
#include "private.h"

int nary_tree_bcast(void *buf, int count, MPI_Datatype datatype, int root, MPI_Comm comm,int arity);
int nary_tree_barrier( MPI_Comm comm, int arity );

int smpi_coll_tuned_alltoall_bruck(void *sbuf, int scount, MPI_Datatype sdtype, 
		                       void* rbuf, int rcount, MPI_Datatype rdtype,
				           MPI_Comm comm);

int smpi_coll_tuned_alltoall_pairwise (void *sendbuf, int sendcount, MPI_Datatype datatype,
		                           void* recvbuf, int recvcount, MPI_Datatype recvdatatype,
						   MPI_Comm comm);

