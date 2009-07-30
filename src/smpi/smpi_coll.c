/* $Id$tag */

/* smpi_coll.c -- various optimized routing for collectives                   */

/* Copyright (c) 2009 Stephane Genaud.                                        */
/* All rights reserved.                                                       */

/* This program is free software; you can redistribute it and/or modify it
 *  * under the terms of the license (GNU LGPL) which comes with this package. */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "private.h"
#include "smpi_coll_private.h"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(smpi_coll, smpi,
                                "Logging specific to SMPI (coll)");

/* proc_tree taken and translated from P2P-MPI */

struct proc_tree {
        int PROCTREE_A;
        int numChildren;
        int * child;
        int parent;
        int me;
        int root;
        int isRoot;
};
typedef struct proc_tree * proc_tree_t;



/* prototypes */
void build_tree( int index, int extent, proc_tree_t *tree);
proc_tree_t alloc_tree( int arity );
void free_tree( proc_tree_t tree);
void print_tree(proc_tree_t tree);




/**
 * alloc and init
 **/
proc_tree_t alloc_tree( int arity ) {
        proc_tree_t tree = malloc(1*sizeof(struct proc_tree));
        int i;

        tree->PROCTREE_A = arity;
        tree->isRoot = 0; 
        tree->numChildren = 0;
        tree->child = malloc(arity*sizeof(int));
        for (i=0; i < arity; i++) {
                tree->child[i] = -1;
        }
        tree->root = -1;
        tree->parent = -1;
        return( tree );
}

/**
 * free
 **/
void free_tree( proc_tree_t tree) {
        free (tree->child );
        free(tree);
}



/**
 * Build the tree depending on a process rank (index) and the group size (extent)
 * @param index the rank of the calling process
 * @param extent the total number of processes
 **/
void build_tree( int index, int extent, proc_tree_t *tree) {
        int places = (*tree)->PROCTREE_A * index;
        int i;
        int ch;
        int pr;

        (*tree)->me = index;
        (*tree)->root = 0 ;

        for (i = 1; i <= (*tree)->PROCTREE_A; i++) {
                ++places;
                ch = (*tree)->PROCTREE_A * index + i + (*tree)->root;
                //printf("places %d\n",places);
                ch %= extent;
                if (places < extent) {
                         //printf("ch <%d> = <%d>\n",i,ch);
                         //printf("adding to the tree at index <%d>\n\n",i-1);
                        (*tree)->child[i - 1] = ch;
                        (*tree)->numChildren++;
                }
                else {
                       //fprintf(stderr,"not adding to the tree\n");
                }
        }
        //fprintf(stderr,"procTree.numChildren <%d>\n",(*tree)->numChildren);

        if (index == (*tree)->root) {
                (*tree)->isRoot = 1;
        }
        else {
                (*tree)->isRoot = 0;
                pr = (index - 1) / (*tree)->PROCTREE_A;
                (*tree)->parent = pr;
        }
}

/**
 * prints the tree as a graphical representation
 **/
void print_tree(proc_tree_t tree) {
      int i;
      char *spacer;
      if (-1 != tree->parent ) {
            printf("[%d]\n   +---",tree->parent);
            spacer= strdup("     ");
      }
      else {
            spacer=strdup("");
      }
      printf("<%d>\n",tree->me);
      for (i=0;i < tree->numChildren; i++) {
              printf("%s   +--- %d\n", spacer,tree->child[i]);
      }
      free(spacer);
}
      
/**
 * bcast
 **/
int tree_bcast(void *buf, int count, MPI_Datatype datatype, int root, MPI_Comm comm, proc_tree_t tree);
int tree_bcast( void *buf, int count, MPI_Datatype datatype, int root,
                MPI_Comm comm, proc_tree_t tree) 
{
        int system_tag = 999;  // used negative int but smpi_create_request() declares this illegal (to be checked)
        int rank;
        int retval = MPI_SUCCESS;
        int i;
        smpi_mpi_request_t request;
        smpi_mpi_request_t * requests;

        rank = smpi_mpi_comm_rank(comm);

        /* wait for data from my parent in the tree */
        if (!tree->isRoot) {
                DEBUG3("<%d> tree_bcast(): i am not root: recv from %d, tag=%d)",rank,tree->parent,system_tag+rank);
                retval = smpi_create_request(buf, count, datatype, 
                                tree->parent, rank, 
                                system_tag + rank, 
                                comm, &request);
                if (MPI_SUCCESS != retval) {
                        printf("** internal error: smpi_create_request() rank=%d returned retval=%d, %s:%d\n",
                                        rank,retval,__FILE__,__LINE__);
                }
                smpi_mpi_irecv(request);
                DEBUG2("<%d> tree_bcast(): waiting on irecv from %d",rank, tree->parent);
                smpi_mpi_wait(request, MPI_STATUS_IGNORE);
                xbt_mallocator_release(smpi_global->request_mallocator, request);
        }

        requests = xbt_malloc( tree->numChildren * sizeof(smpi_mpi_request_t));
        DEBUG2("<%d> creates %d requests (1 per child)\n",rank,tree->numChildren);

        /* iniates sends to ranks lower in the tree */
        for (i=0; i < tree->numChildren; i++) {
                if (tree->child[i] != -1) {
                        DEBUG3("<%d> send to <%d>,tag=%d",rank,tree->child[i], system_tag+tree->child[i]);
                        retval = smpi_create_request(buf, count, datatype, 
                                        rank, tree->child[i], 
                                        system_tag + tree->child[i], 
                                        comm, &(requests[i]));
                        if (MPI_SUCCESS != retval) {
                              printf("** internal error: smpi_create_request() rank=%d returned retval=%d, %s:%d\n",
                                              rank,retval,__FILE__,__LINE__);
                        }
                        smpi_mpi_isend(requests[i]);
                        /* FIXME : we should not wait immediately here. See next FIXME. */
                        smpi_mpi_wait( requests[i], MPI_STATUS_IGNORE);
                        xbt_mallocator_release(smpi_global->request_mallocator, requests[i]);
                }
        }
        /* FIXME : normally, we sould wait only once all isend have been issued:
         * this is the following commented code. It deadlocks, probably because
         * of a bug in the sender process */
        
        /* wait for completion of sends */
        /* printf("[%d] wait for %d send completions\n",rank,tree->numChildren);
          smpi_mpi_waitall( tree->numChildren, requests, MPI_STATUS_IGNORE);
         printf("[%d] reqs completed\n)",rank);
           */
        
        xbt_free(requests);
        return(retval);
        /* checked ok with valgrind --leak-check=full*/
}


/**
 * anti-bcast
 **/
int tree_antibcast(void *buf, int count, MPI_Datatype datatype, int root, MPI_Comm comm, proc_tree_t tree);
int tree_antibcast( void *buf, int count, MPI_Datatype datatype, int root,
                MPI_Comm comm, proc_tree_t tree) 
{
        int system_tag = 999;  // used negative int but smpi_create_request() declares this illegal (to be checked)
        int rank;
        int retval = MPI_SUCCESS;
        int i;
        smpi_mpi_request_t request;
        smpi_mpi_request_t * requests;

        rank = smpi_mpi_comm_rank(comm);

     	  //------------------anti-bcast-------------------
        
        // everyone sends to its parent, except root.
        if (!tree->isRoot) {
                retval = smpi_create_request(buf, count, datatype,
                                rank,tree->parent,  
                                system_tag + rank, 
                                comm, &request);
                if (MPI_SUCCESS != retval) {
                        printf("** internal error: smpi_create_request() rank=%d returned retval=%d, %s:%d\n",
                                        rank,retval,__FILE__,__LINE__);
                }
                smpi_mpi_isend(request);
                smpi_mpi_wait(request, MPI_STATUS_IGNORE);
                xbt_mallocator_release(smpi_global->request_mallocator, request);
        }

        //every one receives as many messages as it has children
        requests = xbt_malloc( tree->numChildren * sizeof(smpi_mpi_request_t));
        for (i=0; i < tree->numChildren; i++) {
                if (tree->child[i] != -1) {
                        retval = smpi_create_request(buf, count, datatype, 
                                        tree->child[i], rank, 
                                        system_tag + tree->child[i], 
                                        comm, &(requests[i]));
                        if (MPI_SUCCESS != retval) {
                                printf("** internal error: smpi_create_request() rank=%d returned retval=%d, %s:%d\n",
                                                rank,retval,__FILE__,__LINE__);
                        }
                        smpi_mpi_irecv(requests[i]);
                        /* FIXME : we should not wait immediately here. See next FIXME. */
                        smpi_mpi_wait( requests[i], MPI_STATUS_IGNORE);
                        xbt_mallocator_release(smpi_global->request_mallocator, requests[i]);
                }
        }
        xbt_free(requests);
        return(retval);

        /* checked ok with valgrind --leak-check=full*/
} 

/**
 * bcast with a binary, ternary, or whatever tree .. 
 **/
int nary_tree_bcast(void *buf, int count, MPI_Datatype datatype, int root,
                MPI_Comm comm, int arity)
{
int rank;
int retval;

        rank = smpi_mpi_comm_rank( comm );
        DEBUG2("<%d> entered nary_tree_bcast(), arity=%d",rank,arity);
        // arity=2: a binary tree, arity=4 seem to be a good setting (see P2P-MPI))
        proc_tree_t tree = alloc_tree( arity ); 
        build_tree( rank, comm->size, &tree );

        retval = tree_bcast( buf, count, datatype, root, comm, tree );

        free_tree( tree );
        return( retval );
}


/**
 * Barrier
 **/
int nary_tree_barrier( MPI_Comm comm , int arity)
{
        int rank;
        int retval = MPI_SUCCESS;
        char dummy='$';

        rank = smpi_mpi_comm_rank(comm);
        // arity=2: a binary tree, arity=4 seem to be a good setting (see P2P-MPI))
        proc_tree_t tree = alloc_tree( arity ); 

        build_tree( rank, comm->size, &tree );

        retval = tree_antibcast( &dummy, 1, MPI_CHAR, 0, comm, tree);
        if (MPI_SUCCESS != retval) {
                printf("[%s:%d] ** Error: tree_antibcast() returned retval=%d\n",__FILE__,__LINE__,retval);
        }
        else {
            retval = tree_bcast(&dummy,  1, MPI_CHAR, 0, comm, tree);
        }

        free_tree( tree );
        return(retval);

        /* checked ok with valgrind --leak-check=full*/
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

int smpi_coll_tuned_alltoall_pairwise (void *sendbuf, int sendcount, MPI_Datatype datatype,
		    void* recvbuf, int recvcount, MPI_Datatype recvdatatype, MPI_Comm comm)
{
        int retval = MPI_SUCCESS;
	  int rank;
	  int size = comm->size;
	  int step;
	  int sendto, recvfrom;
	  int tag_alltoall=999;
	  void * tmpsend, *tmprecv;

	  rank = smpi_mpi_comm_rank(comm);
        INFO1("<%d> algorithm alltoall_pairwise() called.\n",rank);


	  /* Perform pairwise exchange - starting from 1 so the local copy is last */
	  for (step = 1; step < size+1; step++) {

		    /* who do we talk to in this step? */
		    sendto  = (rank+step)%size;
		    recvfrom = (rank+size-step)%size;

		    /* where from are we sending and where from are we receiving actual data ? */
		    tmpsend = (char*)sendbuf+sendto*datatype->size*sendcount;
		    tmprecv = (char*)recvbuf+recvfrom*recvdatatype->size*recvcount;

		    /* send and receive */
		    /* in OpenMPI, they use :
			 err = ompi_coll_tuned_sendrecv( tmpsend, scount, sdtype, sendto, MCA_COLL_BASE_TAG_ALLTOALL,
			 tmprecv, rcount, rdtype, recvfrom, MCA_COLL_BASE_TAG_ALLTOALL,
			 comm, MPI_STATUS_IGNORE, rank);
		     */
		    retval = MPI_Sendrecv(tmpsend, sendcount, datatype, sendto, tag_alltoall,
					        tmprecv, recvcount, recvdatatype, recvfrom, tag_alltoall,
						  comm, MPI_STATUS_IGNORE);
	  }
	  return(retval);
}

/**
 * helper: copy a datatype into another (in the simple case dt1=dt2) 
**/
int copy_dt( void *sbuf, int scount, const MPI_Datatype sdtype, void *rbuf, int rcount, const MPI_Datatype rdtype);
int copy_dt( void *sbuf, int scount, const MPI_Datatype sdtype,
             void *rbuf, int rcount, const MPI_Datatype rdtype)
{
    /* First check if we really have something to do */
    if (0 == rcount) {
        return ((0 == scount) ? MPI_SUCCESS : MPI_ERR_TRUNCATE);
    }
   /* If same datatypes used, just copy. */
   if (sdtype == rdtype) {
      int count = ( scount < rcount ? scount : rcount );
      memcpy( rbuf, sbuf, sdtype->size*count);
      return ((scount > rcount) ? MPI_ERR_TRUNCATE : MPI_SUCCESS);
   }
   /* FIXME:  cases 
    * - If receive packed. 
    * - If send packed
    * to be treated once we have the MPI_Pack things ...
    **/
     return( MPI_SUCCESS );
}

/**
 *
 **/
int smpi_coll_tuned_alltoall_basic_linear(void *sbuf, int scount, MPI_Datatype sdtype,
		    void* rbuf, int rcount, MPI_Datatype rdtype, MPI_Comm comm)
{
	  int i;
        int system_alltoall_tag = 888;
	  int rank;
	  int size = comm->size;
	  int err;
	  char *psnd;
	  char *prcv;
        int nreq = 0;
	  MPI_Aint lb;
	  MPI_Aint sndinc;
	  MPI_Aint rcvinc;
	  MPI_Request *reqs;

	  /* Initialize. */
	  rank = smpi_mpi_comm_rank(comm);
        DEBUG1("<%d> algorithm alltoall_basic_linear() called.",rank);

        err = smpi_mpi_type_get_extent(sdtype, &lb, &sndinc);
        err = smpi_mpi_type_get_extent(rdtype, &lb, &rcvinc);
        sndinc *= scount;
        rcvinc *= rcount;
	  /* simple optimization */
	  psnd = ((char *) sbuf) + (rank * sndinc);
	  prcv = ((char *) rbuf) + (rank * rcvinc);

	  err = copy_dt( psnd, scount, sdtype, prcv, rcount, rdtype );
	  if (MPI_SUCCESS != err) {
		    return err;
	  }

	  /* If only one process, we're done. */
	  if (1 == size) {
		    return MPI_SUCCESS;
	  }

	  /* Initiate all send/recv to/from others. */
	  reqs =  xbt_malloc(2*(size-1) * sizeof(smpi_mpi_request_t));

	  prcv = (char *) rbuf;
	  psnd = (char *) sbuf;

	  /* Post all receives first -- a simple optimization */
	  for (i = (rank + 1) % size; i != rank; i = (i + 1) % size) {
		    err = smpi_create_request( prcv + (i * rcvinc), rcount, rdtype,
                                        i, rank,
                                        system_alltoall_tag,
                                        comm, &(reqs[nreq]));
                if (MPI_SUCCESS != err) {
                        DEBUG2("<%d> failed to create request for rank %d",rank,i);
                        for (i=0;i< nreq;i++) 
                                xbt_mallocator_release(smpi_global->request_mallocator, reqs[i]);
                        return err;
                }
                nreq++;
	  }
	  /* Now post all sends in reverse order 
	   *        - We would like to minimize the search time through message queue
	   *                 when messages actually arrive in the order in which they were posted.
	   *                      */
	  for (i = (rank + size - 1) % size; i != rank; i = (i + size - 1) % size ) {
		    err = smpi_create_request (psnd + (i * sndinc), scount, sdtype, 
                                         rank, i,
						     system_alltoall_tag, 
                                         comm, &(reqs[nreq]));
                if (MPI_SUCCESS != err) {
                        DEBUG2("<%d> failed to create request for rank %d\n",rank,i);
                        for (i=0;i< nreq;i++) 
                                xbt_mallocator_release(smpi_global->request_mallocator, reqs[i]);
                        return err;
                }
                nreq++;
        }

        /* Start your engines.  This will never return an error. */
        for ( i=0; i< nreq/2; i++ ) {
            DEBUG3("<%d> issued irecv request reqs[%d]=%p",rank,i,reqs[i]);
            smpi_mpi_irecv( reqs[i] );
        }
        for ( i= nreq/2; i<nreq; i++ ) {
            DEBUG3("<%d> issued isend request reqs[%d]=%p",rank,i,reqs[i]);
            smpi_mpi_isend( reqs[i] );
        }


        /* Wait for them all.  If there's an error, note that we don't
         * care what the error was -- just that there *was* an error.  The
         * PML will finish all requests, even if one or more of them fail.
         * i.e., by the end of this call, all the requests are free-able.
         * So free them anyway -- even if there was an error, and return
         * the error after we free everything. */

        DEBUG2("<%d> wait for %d requests",rank,nreq);
        // waitall is buggy: use a loop instead for the moment
        // err = smpi_mpi_waitall(nreq, reqs, MPI_STATUS_IGNORE);
        for (i=0;i<nreq;i++) {
                err = smpi_mpi_wait( reqs[i], MPI_STATUS_IGNORE);
        }

	  /* Free the reqs */
        assert( nreq == 2*(size-1) );
        for (i=0;i< nreq;i++) {
            xbt_mallocator_release(smpi_global->request_mallocator, reqs[i]);
        }
        xbt_free( reqs );
	  return err;
}


/**
 * Alltoall Bruck 
 *
 * Openmpi calls this routine when the message size sent to each rank < 2000 bytes and size < 12
 **/


int smpi_coll_tuned_alltoall_bruck(void *sendbuf, int sendcount, MPI_Datatype sdtype,
		    void* recvbuf, int recvcount, MPI_Datatype rdtype, MPI_Comm comm)
{
/*	  int size = comm->size;
	  int i, k, line = -1;
	  int sendto, recvfrom, distance, *displs=NULL, *blen=NULL;
	  int maxpacksize, packsize, position;
	  char * tmpbuf=NULL, *packbuf=NULL;
	  ptrdiff_t lb, sext, rext;
	  int err = 0;
	  int weallocated = 0;
	  MPI_Datatype iddt;

	  rank = smpi_mpi_comm_rank(comm);
*/
	  INFO0("coll:tuned:alltoall_intra_bruck ** NOT IMPLEMENTED YET**");
/*
	  displs = xbt_malloc(size*sizeof(int));
	  blen =   xbt_malloc(size*sizeof(int));

	  weallocated = 1;
*/
	  /* Prepare for packing data */
/*
	  err = MPI_Pack_size( scount*size, sdtype, comm, &maxpacksize );
	  if (err != MPI_SUCCESS) {  }
*/
	  /* pack buffer allocation */
/*	  packbuf = (char*) malloc((unsigned) maxpacksize);
	  if (packbuf == NULL) { }
*/
	  /* tmp buffer allocation for message data */
/*	  tmpbuf = xbt_malloc(scount*size*sext);
	  if (tmpbuf == NULL) {  }
*/

	  /* Step 1 - local rotation - shift up by rank */
/*	  err = ompi_ddt_copy_content_same_ddt (sdtype, (int32_t) ((size-rank)*scount),
				tmpbuf, ((char*)sbuf)+rank*scount*sext);
	  if (err<0) {
		    line = __LINE__; err = -1; goto err_hndl;
	  }

	  if (rank != 0) {
		    err = ompi_ddt_copy_content_same_ddt (sdtype, (int32_t) (rank*scount),
					  tmpbuf+(size-rank)*scount*sext, (char*)sbuf);
		    if (err<0) {
				line = __LINE__; err = -1; goto err_hndl;
		    }
	  }
*/
	  /* perform communication step */
/*	  for (distance = 1; distance < size; distance<<=1) {
*/
		    /* send data to "sendto" */
/*		    sendto = (rank+distance)%size;
		    recvfrom = (rank-distance+size)%size;
		    packsize = 0;
		    k = 0;
*/
		    /* create indexed datatype */
//		    for (i = 1; i < size; i++) {
//				if ((i&distance) == distance) {
//					  displs[k] = i*scount; blen[k] = scount;
//					  k++;
//				}
//		    }
		    /* Set indexes and displacements */
//		    err = MPI_Type_indexed(k, blen, displs, sdtype, &iddt);
//		    if (err != MPI_SUCCESS) { line = __LINE__; goto err_hndl;  }
//		    /* Commit the new datatype */
///		    err = MPI_Type_commit(&iddt);
//		    if (err != MPI_SUCCESS) { line = __LINE__; goto err_hndl;  }

		    /* have the new distribution ddt, pack and exchange data */
//		    err = MPI_Pack(tmpbuf, 1, iddt, packbuf, maxpacksize, &packsize, comm);
//		    if (err != MPI_SUCCESS) { line = __LINE__; goto err_hndl;  }

		    /* Sendreceive */
//		    err = ompi_coll_tuned_sendrecv ( packbuf, packsize, MPI_PACKED, sendto,
//					  MCA_COLL_BASE_TAG_ALLTOALL,
//					  rbuf, packsize, MPI_PACKED, recvfrom,
//					  MCA_COLL_BASE_TAG_ALLTOALL,
//					  comm, MPI_STATUS_IGNORE, rank);
//		    if (err != MPI_SUCCESS) { line = __LINE__; goto err_hndl; }

		    /* Unpack data from rbuf to tmpbuf */
//		    position = 0;
//	    err = MPI_Unpack(rbuf, packsize, &position,
//					  tmpbuf, 1, iddt, comm);
//		    if (err != MPI_SUCCESS) { line = __LINE__; goto err_hndl; }

		    /* free ddt */
//		    err = MPI_Type_free(&iddt);
//		    if (err != MPI_SUCCESS) { line = __LINE__; goto err_hndl;  }
//	  } /* end of for (distance = 1... */

	  /* Step 3 - local rotation - */
//	  for (i = 0; i < size; i++) {

//		    err = ompi_ddt_copy_content_same_ddt (rdtype, (int32_t) rcount,
//					  ((char*)rbuf)+(((rank-i+size)%size)*rcount*rext),
//					  tmpbuf+i*rcount*rext);
//
//	  if (err<0) {
//				line = __LINE__; err = -1; goto err_hndl;
//		    }
//	  }

	  /* Step 4 - clean up */
/*	  if (tmpbuf != NULL) free(tmpbuf);
	  if (packbuf != NULL) free(packbuf);
	  if (weallocated) {
		    if (displs != NULL) free(displs);
		    if (blen != NULL) free(blen);
	  }
	  return OMPI_SUCCESS;

err_hndl:
	  OPAL_OUTPUT((ompi_coll_tuned_stream,"%s:%4d\tError occurred %d, rank %2d", __FILE__,line,err,rank));
	  if (tmpbuf != NULL) free(tmpbuf);
	  if (packbuf != NULL) free(packbuf);
	  if (weallocated) {
		    if (displs != NULL) free(displs);
		    if (blen != NULL) free(blen);
	  }
	  return err;
	  */
	  return -1; /* FIXME: to be changed*/
}


/**
 * -----------------------------------------------------------------------------------------------------
 * example usage
 * -----------------------------------------------------------------------------------------------------
 **/
/*
 * int main() {

      int rank; 
      int size=12;

      proc_tree_t tree;
      for (rank=0;rank<size;rank++) {
            printf("--------------tree for rank %d ----------\n",rank);
            tree = alloc_tree( 2 );
            build_tree( rank, size, &tree );
            print_tree( tree );
            free_tree( tree );
   
      }
      printf("-------------- bcast ----------\n");
      for (rank=0;rank<size;rank++) {
              bcast( rank, size );
      }


}
*/

                

