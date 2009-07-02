/* $Id$tag */

/* smpi_coll.c -- various optimized routing for collectives                   */

/* Copyright (c) 2009 Stephane Genaud.                                        */
/* All rights reserved.                                                       */

/* This program is free software; you can redistribute it and/or modify it
 *  * under the terms of the license (GNU LGPL) which comes with this package. */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "private.h"


/* proctree taken and translated from P2P-MPI */

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
int binomial_tree_bcast(void *buf, int count, MPI_Datatype datatype, int root, MPI_Comm comm);



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
int binomial_tree_bcast(void *buf, int count, MPI_Datatype datatype, int root,
                MPI_Comm comm)
{
        int system_tag = 999;  // used negative int but smpi_create_request() declares this illegal (to be checked)
        int rank;
        int retval = MPI_SUCCESS;
        int i;
        smpi_mpi_request_t request;
        smpi_mpi_request_t * requests;
        void **tmpbufs; 

        rank = smpi_mpi_comm_rank(comm);
        proc_tree_t tree = alloc_tree( 2 ); // arity=2: a binomial tree

        build_tree( rank, comm->size, &tree );
        /* wait for data from my parent in the tree */
        if (!tree->isRoot) {
#ifdef DEBUG_STEPH
                printf("[%d] recv(%d  from %d, tag=%d)\n",rank,rank, tree->parent, system_tag+rank);
#endif
                retval = smpi_create_request(buf, count, datatype, 
                                tree->parent, rank, 
                                system_tag + rank, 
                                comm, &request);
                if (MPI_SUCCESS != retval) {
                        printf("** internal error: smpi_create_request() rank=%d returned retval=%d, %s:%d\n",
                                        rank,retval,__FILE__,__LINE__);
                }
                smpi_mpi_irecv(request);
#ifdef DEBUG_STEPH
                printf("[%d] waiting on irecv from %d\n",rank , tree->parent);
#endif
                smpi_mpi_wait(request, MPI_STATUS_IGNORE);
        }

        tmpbufs  = xbt_malloc( tree->numChildren * sizeof(void *)); 
        requests = xbt_malloc( tree->numChildren * sizeof(smpi_mpi_request_t));
#ifdef DEBUG_STEPH
        printf("[%d] creates %d requests\n",rank,tree->numChildren);
#endif

        /* iniates sends to ranks lower in the tree */
        for (i=0; i < tree->numChildren; i++) {
                if (tree->child[i] != -1) {
#ifdef DEBUG_STEPH
                        printf("[%d] send(%d->%d, tag=%d)\n",rank,rank, tree->child[i], system_tag+tree->child[i]);
#endif
                        tmpbufs[i] =  xbt_malloc( count * datatype->size);
                        memcpy( tmpbufs[i], buf, count * datatype->size * sizeof(char));
                        retval = smpi_create_request(tmpbufs[i], count, datatype, 
                                        rank, tree->child[i], 
                                        system_tag + tree->child[i], 
                                        comm, &(requests[i]));
#ifdef DEBUG_STEPH
                        printf("[%d] after create req[%d]=%p req->(src=%d,dst=%d)\n",rank , i, requests[i],requests[i]->src,requests[i]->dst );
#endif
                        if (MPI_SUCCESS != retval) {
                              printf("** internal error: smpi_create_request() rank=%d returned retval=%d, %s:%d\n",
                                              rank,retval,__FILE__,__LINE__);
                        }
                        smpi_mpi_isend(requests[i]);
                        /* FIXME : we should not wait immediately here. See next FIXME. */
                        smpi_mpi_wait( requests[i], MPI_STATUS_IGNORE);
                        xbt_free(tmpbufs[i]);
                        xbt_free(requests[i]);
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
        
        xbt_free(tmpbufs);
        xbt_free(requests);
        return(retval);
}

/**
 * example usage
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

                

