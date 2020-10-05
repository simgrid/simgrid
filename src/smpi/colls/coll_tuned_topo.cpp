/* Copyright (c) 2013-2020. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/*
 * Copyright (c) 2004-2005 The Trustees of Indiana University and Indiana
 *                         University Research and Technology
 *                         Corporation.  All rights reserved.
 * Copyright (c) 2004-2005 The University of Tennessee and The University
 *                         of Tennessee Research Foundation.  All rights
 *                         reserved.
 * Copyright (c) 2004-2005 High Performance Computing Center Stuttgart,
 *                         University of Stuttgart.  All rights reserved.
 * Copyright (c) 2004-2005 The Regents of the University of California.
 *                         All rights reserved.
 *
 * Additional copyrights may follow
 */

#include "coll_tuned_topo.hpp"
#include "colls_private.hpp"
/*
 * Some static helpers.
 */
static int pown( int fanout, int num )
{
    int p = 1;
    if( num < 0 ) return 0;
    if (1==num) return fanout;
    if (2==fanout) {
        return p<<num;
    }
    else {
      for (int j = 0; j < num; j++) {
        p *= fanout;
      }
    }
    return p;
}

static int calculate_level( int fanout, int rank )
{
    int level, num;
    if( rank < 0 ) return -1;
    for( level = 0, num = 0; num <= rank; level++ ) {
        num += pown(fanout, level);
    }
    return level-1;
}

static int calculate_num_nodes_up_to_level( int fanout, int level )
{
    /* just use geometric progression formula for sum:
       a^0+a^1+...a^(n-1) = (a^n-1)/(a-1) */
    if (fanout > 1)
      return ((pown(fanout, level) - 1) / (fanout - 1));
    else
      return 0; // is this right ?
}

/*
 * And now the building functions.
 *
 * An example for fanout = 2, comm_size = 7
 *
 *              0           <-- delta = 1 (fanout^0)
 *            /   \
 *           1     2        <-- delta = 2 (fanout^1)
 *          / \   / \
 *         3   5 4   6      <-- delta = 4 (fanout^2)
 */

ompi_coll_tree_t*
ompi_coll_tuned_topo_build_tree( int fanout,
                                 MPI_Comm comm,
                                 int root )
{
    int rank, size;
    int sparent;
    int level; /* location of my rank in the tree structure of size */
    int delta; /* number of nodes on my level */
    int slimit; /* total number of nodes on levels above me */
    int shiftedrank;
    ompi_coll_tree_t* tree;

    XBT_DEBUG("coll:tuned:topo_build_tree Building fo %d rt %d", fanout, root);

    if (fanout<1) {
        XBT_DEBUG("coll:tuned:topo_build_tree invalid fanout %d", fanout);
        return nullptr;
    }
    if (fanout>MAXTREEFANOUT) {
        XBT_DEBUG("coll:tuned:topo_build_tree invalid fanout %d bigger than max %d", fanout, MAXTREEFANOUT);
        return nullptr;
    }

    /*
     * Get size and rank of the process in this communicator
     */
    size = comm->size();
    rank = comm->rank();

    tree = new ompi_coll_tree_t;
    if (not tree) {
      XBT_DEBUG("coll:tuned:topo_build_tree PANIC::out of memory");
      return nullptr;
    }

    /*
     * Initialize tree
     */
    tree->tree_fanout   = fanout;
    tree->tree_bmtree   = 0;
    tree->tree_root     = root;
    tree->tree_prev     = -1;
    tree->tree_nextsize = 0;
    for (int i = 0; i < fanout; i++) {
      tree->tree_next[i] = -1;
    }

    /* return if we have less than 2 processes */
    if( size < 2 ) {
        return tree;
    }

    /*
     * Shift all ranks by root, so that the algorithm can be
     * designed as if root would be always 0
     * shiftedrank should be used in calculating distances
     * and position in tree
     */
    shiftedrank = rank - root;
    if( shiftedrank < 0 ) {
        shiftedrank += size;
    }

    /* calculate my level */
    level = calculate_level( fanout, shiftedrank );
    delta = pown( fanout, level );

    /* find my children */
    for (int i = 0; i < fanout; i++) {
        int schild = shiftedrank + delta * (i+1);
        if( schild < size ) {
            tree->tree_next[i] = (schild+root)%size;
            tree->tree_nextsize = tree->tree_nextsize + 1;
        } else {
            break;
        }
    }

    /* find my parent */
    slimit = calculate_num_nodes_up_to_level( fanout, level );
    sparent = shiftedrank;
    if( sparent < fanout ) {
        sparent = 0;
    } else {
        while( sparent >= slimit ) {
            sparent -= delta/fanout;
        }
    }
    tree->tree_prev = (sparent+root)%size;

    return tree;
}

/*
 * Constructs in-order binary tree which can be used for non-commutative reduce
 * operations.
 * Root of this tree is always rank (size-1) and fanout is 2.
 * Here are some of the examples of this tree:
 * size == 2     size == 3     size == 4                size == 9
 *      1             2             3                        8
 *     /             / \          /   \                    /   \
 *    0             1  0         2     1                  7     3
 *                                    /                 /  \   / \
 *                                   0                 6    5 2   1
 *                                                         /     /
 *                                                        4     0
 */
ompi_coll_tree_t*
ompi_coll_tuned_topo_build_in_order_bintree( MPI_Comm comm )
{
    int rank, size;
    int myrank, delta;
    int parent;
    ompi_coll_tree_t* tree;

    /*
     * Get size and rank of the process in this communicator
     */
    size = comm->size();
    rank = comm->rank();

    tree = new ompi_coll_tree_t;
    if (not tree) {
      XBT_DEBUG("coll:tuned:topo_build_tree PANIC::out of memory");
      return nullptr;
    }

    /*
     * Initialize tree
     */
    tree->tree_fanout   = 2;
    tree->tree_bmtree   = 0;
    tree->tree_root     = size - 1;
    tree->tree_prev     = -1;
    tree->tree_nextsize = 0;
    tree->tree_next[0]  = -1;
    tree->tree_next[1]  = -1;
    XBT_DEBUG(
                 "coll:tuned:topo_build_in_order_tree Building fo %d rt %d",
                 tree->tree_fanout, tree->tree_root);

    /*
     * Build the tree
     */
    myrank = rank;
    parent = size - 1;
    delta = 0;

    while (true) {
      /* Compute the size of the right subtree */
      int rightsize = size >> 1;

      /* Determine the left and right child of this parent  */
      int lchild = -1;
      int rchild = -1;
      if (size - 1 > 0) {
        lchild = parent - 1;
        if (lchild > 0) {
          rchild = rightsize - 1;
        }
      }

      /* The following cases are possible: myrank can be
         - a parent,
         - belong to the left subtree, or
         - belong to the right subtee
         Each of the cases need to be handled differently.
      */

      if (myrank == parent) {
        /* I am the parent:
           - compute real ranks of my children, and exit the loop. */
        if (lchild >= 0)
          tree->tree_next[0] = lchild + delta;
        if (rchild >= 0)
          tree->tree_next[1] = rchild + delta;
        break;
      }
      if (myrank > rchild) {
        /* I belong to the left subtree:
           - If I am the left child, compute real rank of my parent
           - Iterate down through tree:
           compute new size, shift ranks down, and update delta.
        */
        if (myrank == lchild) {
          tree->tree_prev = parent + delta;
        }
        size   = size - rightsize - 1;
        delta  = delta + rightsize;
        myrank = myrank - rightsize;
        parent = size - 1;

      } else {
        /* I belong to the right subtree:
           - If I am the right child, compute real rank of my parent
           - Iterate down through tree:
           compute new size and parent,
           but the delta and rank do not need to change.
        */
        if (myrank == rchild) {
          tree->tree_prev = parent + delta;
        }
        size   = rightsize;
        parent = rchild;
      }
    }

    if (tree->tree_next[0] >= 0) { tree->tree_nextsize = 1; }
    if (tree->tree_next[1] >= 0) { tree->tree_nextsize += 1; }

    return tree;
}

int ompi_coll_tuned_topo_destroy_tree( ompi_coll_tree_t** tree )
{
    ompi_coll_tree_t *ptr;

    if ((tree == nullptr) || (*tree == nullptr)) {
      return MPI_SUCCESS;
    }

    ptr = *tree;

    delete ptr;
    *tree = nullptr; /* mark tree as gone */

    return MPI_SUCCESS;
}

/*
 *
 * Here are some of the examples of this tree:
 * size == 2                   size = 4                 size = 8
 *      0                           0                        0
 *     /                            | \                    / | \
 *    1                             2  1                  4  2  1
 *                                     |                     |  |\
 *                                     3                     6  5 3
 *                                                                |
 *                                                                7
 */
ompi_coll_tree_t*
ompi_coll_tuned_topo_build_bmtree( MPI_Comm comm,
                                   int root )
{
    int children = 0;
    int rank;
    int size;
    int mask = 1;
    int index;
    int remote;
    ompi_coll_tree_t *bmtree;
    int i;

    XBT_DEBUG("coll:tuned:topo:build_bmtree rt %d", root);

    /*
     * Get size and rank of the process in this communicator
     */
    size = comm->size();
    rank = comm->rank();

    index = rank -root;

    bmtree = new ompi_coll_tree_t;
    if (not bmtree) {
      XBT_DEBUG("coll:tuned:topo:build_bmtree PANIC out of memory");
      return nullptr;
    }

    bmtree->tree_bmtree   = 1;

    bmtree->tree_root     = MPI_UNDEFINED;
    bmtree->tree_nextsize = MPI_UNDEFINED;
    for(i=0;i<MAXTREEFANOUT;i++) {
        bmtree->tree_next[i] = -1;
    }

    if( index < 0 ) index += size;

    while( mask <= index ) mask <<= 1;

    /* Now I can compute my father rank */
    if( root == rank ) {
        bmtree->tree_prev = root;
    } else {
        remote = (index ^ (mask >> 1)) + root;
        if( remote >= size ) remote -= size;
        bmtree->tree_prev = remote;
    }
    /* And now let's fill my children */
    while( mask < size ) {
        remote = (index ^ mask);
        if( remote >= size ) break;
        remote += root;
        if( remote >= size ) remote -= size;
        if (children==MAXTREEFANOUT) {
            XBT_DEBUG("coll:tuned:topo:build_bmtree max fanout incorrect %d needed %d", MAXTREEFANOUT, children);
            delete bmtree;
            return nullptr;
        }
        bmtree->tree_next[children] = remote;
        mask <<= 1;
        children++;
    }
    bmtree->tree_nextsize = children;
    bmtree->tree_root     = root;
    return bmtree;
}

/*
 * Constructs in-order binomial tree which can be used for gather/scatter
 * operations.
 *
 * Here are some of the examples of this tree:
 * size == 2                   size = 4                 size = 8
 *      0                           0                        0
 *     /                          / |                      / | \
 *    1                          1  2                     1  2  4
 *                                  |                        |  | \
 *                                  3                        3  5  6
 *                                                                 |
 *                                                                 7
 */
ompi_coll_tree_t* ompi_coll_tuned_topo_build_in_order_bmtree(MPI_Comm comm, int root)
{
    int children = 0;
    int rank, vrank;
    int size;
    int mask = 1;
    ompi_coll_tree_t *bmtree;
    int i;

    XBT_DEBUG("coll:tuned:topo:build_in_order_bmtree rt %d", root);

    /*
     * Get size and rank of the process in this communicator
     */
    size = comm->size();
    rank = comm->rank();

    vrank = (rank - root + size) % size;

    bmtree = new ompi_coll_tree_t;
    if (not bmtree) {
      XBT_DEBUG("coll:tuned:topo:build_bmtree PANIC out of memory");
      delete bmtree;
      return nullptr;
    }

    bmtree->tree_bmtree   = 1;
    bmtree->tree_root     = MPI_UNDEFINED;
    bmtree->tree_nextsize = MPI_UNDEFINED;
    for(i=0;i<MAXTREEFANOUT;i++) {
        bmtree->tree_next[i] = -1;
    }

    if (root == rank) {
      bmtree->tree_prev = root;
    }

    while (mask < size) {
      int remote = vrank ^ mask;
      if (remote < vrank) {
        bmtree->tree_prev = (remote + root) % size;
        break;
      } else if (remote < size) {
        bmtree->tree_next[children] = (remote + root) % size;
        children++;
        if (children == MAXTREEFANOUT) {
          XBT_DEBUG("coll:tuned:topo:build_bmtree max fanout incorrect %d needed %d", MAXTREEFANOUT, children);
          delete bmtree;
          return nullptr;
        }
      }
      mask <<= 1;
    }
    bmtree->tree_nextsize = children;
    bmtree->tree_root     = root;

    return bmtree;
}


ompi_coll_tree_t*
ompi_coll_tuned_topo_build_chain( int fanout,
                                  MPI_Comm comm,
                                  int root )
{
    int rank, size;
    int srank; /* shifted rank */
    int i,maxchainlen;
    int mark;
    ompi_coll_tree_t *chain;

    XBT_DEBUG("coll:tuned:topo:build_chain fo %d rt %d", fanout, root);

    /*
     * Get size and rank of the process in this communicator
     */
    size = comm->size();
    rank = comm->rank();

    if( fanout < 1 ) {
        XBT_DEBUG("coll:tuned:topo:build_chain WARNING invalid fanout of ZERO, forcing to 1 (pipeline)!");
        fanout = 1;
    }
    if (fanout>MAXTREEFANOUT) {
        XBT_DEBUG("coll:tuned:topo:build_chain WARNING invalid fanout %d bigger than max %d, forcing to max!", fanout, MAXTREEFANOUT);
        fanout = MAXTREEFANOUT;
    }

    /*
     * Allocate space for topology arrays if needed
     */
    chain = new ompi_coll_tree_t;
    if (not chain) {
      XBT_DEBUG("coll:tuned:topo:build_chain PANIC out of memory");
      fflush(stdout);
      return nullptr;
    }
    for(i=0;i<fanout;i++) chain->tree_next[i] = -1;

    /*
     * Set root & numchain
     */
    chain->tree_root = root;
    if( (size - 1) < fanout ) {
        chain->tree_nextsize = size-1;
        fanout = size-1;
    } else {
        chain->tree_nextsize = fanout;
    }

    /*
     * Shift ranks
     */
    srank = rank - root;
    if (srank < 0) srank += size;

    /*
     * Special case - fanout == 1
     */
    if( fanout == 1 ) {
        if( srank == 0 ) chain->tree_prev = -1;
        else chain->tree_prev = (srank-1+root)%size;

        if( (srank + 1) >= size) {
            chain->tree_next[0] = -1;
            chain->tree_nextsize = 0;
        } else {
            chain->tree_next[0] = (srank+1+root)%size;
            chain->tree_nextsize = 1;
        }
        return chain;
    }

    /* Let's handle the case where there is just one node in the communicator */
    if( size == 1 ) {
        chain->tree_next[0] = -1;
        chain->tree_nextsize = 0;
        chain->tree_prev = -1;
        return chain;
    }
    /*
     * Calculate maximum chain length
     */
    maxchainlen = (size-1) / fanout;
    if( (size-1) % fanout != 0 ) {
        maxchainlen++;
        mark = (size-1)%fanout;
    } else {
        mark = fanout+1;
    }

    /*
     * Find your own place in the list of shifted ranks
     */
    if( srank != 0 ) {
        int column;
        int head;
        int len;
        if( srank-1 < (mark * maxchainlen) ) {
            column = (srank-1)/maxchainlen;
            head = 1+column*maxchainlen;
            len = maxchainlen;
        } else {
            column = mark + (srank-1-mark*maxchainlen)/(maxchainlen-1);
            head = mark*maxchainlen+1+(column-mark)*(maxchainlen-1);
            len = maxchainlen-1;
        }

        if( srank == head ) {
            chain->tree_prev = 0; /*root*/
        } else {
            chain->tree_prev = srank-1; /* rank -1 */
        }
        if( srank == (head + len - 1) ) {
            chain->tree_next[0] = -1;
            chain->tree_nextsize = 0;
        } else {
            if( (srank + 1) < size ) {
                chain->tree_next[0] = srank+1;
                chain->tree_nextsize = 1;
            } else {
                chain->tree_next[0] = -1;
                chain->tree_nextsize = 0;
            }
        }
    }

    /*
     * Unshift values
     */
    if( rank == root ) {
        chain->tree_prev = -1;
        chain->tree_next[0] = (root+1)%size;
        for (int i = 1; i < fanout; i++) {
            chain->tree_next[i] = chain->tree_next[i-1] + maxchainlen;
            if( i > mark ) {
                chain->tree_next[i]--;
            }
            chain->tree_next[i] %= size;
        }
        chain->tree_nextsize = fanout;
    } else {
        chain->tree_prev = (chain->tree_prev+root)%size;
        if( chain->tree_next[0] != -1 ) {
            chain->tree_next[0] = (chain->tree_next[0]+root)%size;
        }
    }

    return chain;
}

int ompi_coll_tuned_topo_dump_tree (ompi_coll_tree_t* tree, int rank)
{
    XBT_DEBUG("coll:tuned:topo:topo_dump_tree %1d tree root %d"
                 " fanout %d BM %1d nextsize %d prev %d",
                 rank, tree->tree_root, tree->tree_bmtree, tree->tree_fanout,
                 tree->tree_nextsize, tree->tree_prev);
    if( tree->tree_nextsize ) {
      for (int i = 0; i < tree->tree_nextsize; i++)
        XBT_DEBUG("[%1d] %d", i, tree->tree_next[i]);
    }
    return (0);
}
