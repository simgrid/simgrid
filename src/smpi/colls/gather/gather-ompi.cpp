/* Copyright (c) 2013-2020. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/*
 * Copyright (c) 2004-2005 The Trustees of Indiana University and Indiana
 *                         University Research and Technology
 *                         Corporation.  All rights reserved.
 * Copyright (c) 2004-2009 The University of Tennessee and The University
 *                         of Tennessee Research Foundation.  All rights
 *                         reserved.
 * Copyright (c) 2004-2005 High Performance Computing Center Stuttgart,
 *                         University of Stuttgart.  All rights reserved.
 * Copyright (c) 2004-2005 The Regents of the University of California.
 *                         All rights reserved.
 *
 * Additional copyrights may follow
 */

#include "../coll_tuned_topo.hpp"
#include "../colls_private.hpp"

namespace simgrid {
namespace smpi {

int gather__ompi_binomial(const void* sbuf, int scount, MPI_Datatype sdtype, void* rbuf, int rcount,
                          MPI_Datatype rdtype, int root, MPI_Comm comm)
{
    int line = -1;
    int i;
    int rank;
    int vrank;
    int size;
    int total_recv = 0;
    unsigned char* ptmp    = nullptr;
    unsigned char* tempbuf = nullptr;
    const unsigned char* src_buf;
    int err;
    ompi_coll_tree_t* bmtree;
    MPI_Status status;
    MPI_Aint sextent, slb, strue_lb, strue_extent;
    MPI_Aint rextent, rlb, rtrue_lb, rtrue_extent;


    size = comm->size();
    rank = comm->rank();

    XBT_DEBUG("smpi_coll_tuned_gather_ompi_binomial rank %d", rank);

    /* create the binomial tree */
   // COLL_TUNED_UPDATE_IN_ORDER_BMTREE( comm, tuned_module, root );
    bmtree = ompi_coll_tuned_topo_build_in_order_bmtree(comm, root);
    // data->cached_in_order_bmtree;

    sdtype->extent(&slb, &sextent);
    sdtype->extent(&strue_lb, &strue_extent);

    vrank = (rank - root + size) % size;

    if (rank == root) {
        rdtype->extent(&rlb, &rextent);
        rdtype->extent(&rtrue_lb, &rtrue_extent);
        if (0 == root) {
          /* root on 0, just use the recv buffer */
          ptmp = static_cast<unsigned char*>(rbuf);
          if (sbuf != MPI_IN_PLACE) {
            err = Datatype::copy(sbuf, scount, sdtype, ptmp, rcount, rdtype);
            if (MPI_SUCCESS != err) {
              line = __LINE__;
              goto err_hndl;
            }
          }
        } else {
          /* root is not on 0, allocate temp buffer for recv,
           * rotate data at the end */
          tempbuf = smpi_get_tmp_recvbuffer(rtrue_extent + (rcount * size - 1) * rextent);
          if (nullptr == tempbuf) {
            err  = MPI_ERR_OTHER;
            line = __LINE__;
            goto err_hndl;
          }

          ptmp = tempbuf - rlb;
          if (sbuf != MPI_IN_PLACE) {
            /* copy from sbuf to temp buffer */
            err = Datatype::copy(sbuf, scount, sdtype, ptmp, rcount, rdtype);
            if (MPI_SUCCESS != err) {
              line = __LINE__;
              goto err_hndl;
            }
          } else {
            /* copy from rbuf to temp buffer  */
            err = Datatype::copy((char*)rbuf + rank * rextent * rcount, rcount, rdtype, ptmp, rcount, rdtype);
            if (MPI_SUCCESS != err) {
              line = __LINE__;
              goto err_hndl;
            }
          }
        }
        total_recv = rcount;
        src_buf    = ptmp;
    } else if (!(vrank % 2)) {
      /* other non-leaf nodes, allocate temp buffer for data received from
       * children, the most we need is half of the total data elements due
       * to the property of binomial tree */
      tempbuf = smpi_get_tmp_sendbuffer(strue_extent + (scount * size - 1) * sextent);
      if (nullptr == tempbuf) {
        err  = MPI_ERR_OTHER;
        line = __LINE__;
        goto err_hndl;
      }

      ptmp = tempbuf - slb;
      /* local copy to tempbuf */
      err = Datatype::copy(sbuf, scount, sdtype, ptmp, scount, sdtype);
      if (MPI_SUCCESS != err) {
        line = __LINE__;
        goto err_hndl;
      }

      /* use sdtype,scount as rdtype,rdcount since they are ignored on
       * non-root procs */
      rdtype     = sdtype;
      rcount     = scount;
      rextent    = sextent;
      total_recv = rcount;
      src_buf    = ptmp;
    } else {
      /* leaf nodes, no temp buffer needed, use sdtype,scount as
       * rdtype,rdcount since they are ignored on non-root procs */
      total_recv = scount;
      src_buf    = static_cast<const unsigned char*>(sbuf);
    }

    if (!(vrank % 2)) {
      /* all non-leaf nodes recv from children */
      for (i = 0; i < bmtree->tree_nextsize; i++) {
        int mycount = 0, vkid;
        /* figure out how much data I have to send to this child */
        vkid    = (bmtree->tree_next[i] - root + size) % size;
        mycount = vkid - vrank;
        if (mycount > (size - vkid))
          mycount = size - vkid;
        mycount *= rcount;

        XBT_DEBUG("smpi_coll_tuned_gather_ompi_binomial rank %d recv %d mycount = %d", rank, bmtree->tree_next[i],
                  mycount);

        Request::recv(ptmp + total_recv * rextent, mycount, rdtype, bmtree->tree_next[i], COLL_TAG_GATHER, comm,
                      &status);

        total_recv += mycount;
      }
    }

    if (rank != root) {
      /* all nodes except root send to parents */
      XBT_DEBUG("smpi_coll_tuned_gather_ompi_binomial rank %d send %d count %d\n", rank, bmtree->tree_prev, total_recv);

      Request::send(src_buf, total_recv, sdtype, bmtree->tree_prev, COLL_TAG_GATHER, comm);
  }
    if (rank == root) {
      if (root != 0) {
        /* rotate received data on root if root != 0 */
        err = Datatype::copy(ptmp, rcount * (size - root), rdtype, (char*)rbuf + rextent * root * rcount,
                             rcount * (size - root), rdtype);
        if (MPI_SUCCESS != err) {
          line = __LINE__;
          goto err_hndl;
        }

        err = Datatype::copy(ptmp + rextent * rcount * (size - root), rcount * root, rdtype, (char*)rbuf, rcount * root,
                             rdtype);
        if (MPI_SUCCESS != err) {
          line = __LINE__;
          goto err_hndl;
        }

        smpi_free_tmp_buffer(tempbuf);
      }
    } else if (!(vrank % 2)) {
      /* other non-leaf nodes */
      smpi_free_tmp_buffer(tempbuf);
    }
    ompi_coll_tuned_topo_destroy_tree(&bmtree);
    return MPI_SUCCESS;

 err_hndl:
   if (nullptr != tempbuf)
     smpi_free_tmp_buffer(tempbuf);

   XBT_DEBUG("%s:%4d\tError occurred %d, rank %2d", __FILE__, line, err, rank);
   return err;
}

/*
 *  gather_intra_linear_sync
 *
 *  Function:  - synchronized gather operation with
 *  Accepts:  - same arguments as MPI_Gather(), first segment size
 *  Returns:  - MPI_SUCCESS or error code
 */
int gather__ompi_linear_sync(const void *sbuf, int scount,
                             MPI_Datatype sdtype,
                             void *rbuf, int rcount,
                             MPI_Datatype rdtype,
                             int root,
                             MPI_Comm comm)
{
    int i;
    int ret, line;
    int rank, size;
    int first_segment_count;
    size_t typelng;
    MPI_Aint extent;
    MPI_Aint lb;

    int first_segment_size=0;
    size = comm->size();
    rank = comm->rank();

    size_t dsize, block_size;
    if (rank == root) {
        dsize= rdtype->size();
        block_size = dsize * rcount;
    } else {
        dsize=sdtype->size();
        block_size = dsize * scount;
    }

     if (block_size > 92160){
     first_segment_size = 32768;
     }else{
     first_segment_size = 1024;
     }

     XBT_DEBUG("smpi_coll_tuned_gather_ompi_linear_sync rank %d, segment %d", rank, first_segment_size);

     if (rank != root) {
       /* Non-root processes:
          - receive zero byte message from the root,
          - send the first segment of the data synchronously,
          - send the second segment of the data.
       */

       typelng = sdtype->size();
       sdtype->extent(&lb, &extent);
       first_segment_count = scount;
       COLL_TUNED_COMPUTED_SEGCOUNT((size_t)first_segment_size, typelng, first_segment_count);

       Request::recv(nullptr, 0, MPI_BYTE, root, COLL_TAG_GATHER, comm, MPI_STATUS_IGNORE);

       Request::send(sbuf, first_segment_count, sdtype, root, COLL_TAG_GATHER, comm);

       Request::send((char*)sbuf + extent * first_segment_count, (scount - first_segment_count), sdtype, root,
                     COLL_TAG_GATHER, comm);
    }

    else {
      /* Root process,
         - For every non-root node:
   - post irecv for the first segment of the message
   - send zero byte message to signal node to send the message
   - post irecv for the second segment of the message
   - wait for the first segment to complete
         - Copy local data if necessary
         - Waitall for all the second segments to complete.
*/
      char* ptmp;
      MPI_Request first_segment_req;
      MPI_Request* reqs = new (std::nothrow) MPI_Request[size];
      if (nullptr == reqs) {
        ret  = -1;
        line = __LINE__;
        goto error_hndl;
      }

        typelng=rdtype->size();
        rdtype->extent(&lb, &extent);
        first_segment_count = rcount;
        COLL_TUNED_COMPUTED_SEGCOUNT( (size_t)first_segment_size, typelng,
                                      first_segment_count );

        for (i = 0; i < size; ++i) {
            if (i == rank) {
                /* skip myself */
                reqs[i] = MPI_REQUEST_NULL;
                continue;
            }

            /* irecv for the first segment from i */
            ptmp = (char*)rbuf + i * rcount * extent;
            first_segment_req = Request::irecv(ptmp, first_segment_count, rdtype, i,
                                     COLL_TAG_GATHER, comm
                                     );

            /* send sync message */
            Request::send(rbuf, 0, MPI_BYTE, i,
                                    COLL_TAG_GATHER,
                                     comm);

            /* irecv for the second segment */
            ptmp = (char*)rbuf + (i * rcount + first_segment_count) * extent;
            reqs[i]=Request::irecv(ptmp, (rcount - first_segment_count),
                                     rdtype, i, COLL_TAG_GATHER, comm
                                     );

            /* wait on the first segment to complete */
            Request::wait(&first_segment_req, MPI_STATUS_IGNORE);
        }

        /* copy local data if necessary */
        if (MPI_IN_PLACE != sbuf) {
            ret = Datatype::copy(sbuf, scount, sdtype,
                                  (char*)rbuf + rank * rcount * extent,
                                  rcount, rdtype);
            if (ret != MPI_SUCCESS) { line = __LINE__; goto error_hndl; }
        }

        /* wait all second segments to complete */
        ret = Request::waitall(size, reqs, MPI_STATUSES_IGNORE);
        if (ret != MPI_SUCCESS) { line = __LINE__; goto error_hndl; }

        delete[] reqs;
    }

    /* All done */

    return MPI_SUCCESS;
 error_hndl:
    XBT_DEBUG(
                   "ERROR_HNDL: node %d file %s line %d error %d\n",
                   rank, __FILE__, line, ret );
    return ret;
}

/*
 * Linear functions are copied from the BASIC coll module
 * they do not segment the message and are simple implementations
 * but for some small number of nodes and/or small data sizes they
 * are just as fast as tuned/tree based segmenting operations
 * and as such may be selected by the decision functions
 * These are copied into this module due to the way we select modules
 * in V1. i.e. in V2 we will handle this differently and so will not
 * have to duplicate code.
 * JPG following the examples from other coll_tuned implementations. Dec06.
 */

/* copied function (with appropriate renaming) starts here */
/*
 *  gather_intra
 *
 *  Function:  - basic gather operation
 *  Accepts:  - same arguments as MPI_Gather()
 *  Returns:  - MPI_SUCCESS or error code
 */
int gather__ompi_basic_linear(const void* sbuf, int scount, MPI_Datatype sdtype, void* rbuf, int rcount,
                              MPI_Datatype rdtype, int root, MPI_Comm comm)
{
    int i;
    int err;
    int rank;
    int size;
    char *ptmp;
    MPI_Aint incr;
    MPI_Aint extent;
    MPI_Aint lb;

    size = comm->size();
    rank = comm->rank();

    /* Everyone but root sends data and returns. */
    XBT_DEBUG("ompi_coll_tuned_gather_intra_basic_linear rank %d", rank);

    if (rank != root) {
        Request::send(sbuf, scount, sdtype, root,
                                 COLL_TAG_GATHER,
                                  comm);
        return MPI_SUCCESS;
    }

    /* I am the root, loop receiving the data. */

    rdtype->extent(&lb, &extent);
    incr = extent * rcount;
    for (i = 0, ptmp = (char *) rbuf; i < size; ++i, ptmp += incr) {
        if (i == rank) {
            if (MPI_IN_PLACE != sbuf) {
                err = Datatype::copy(sbuf, scount, sdtype,
                                      ptmp, rcount, rdtype);
            } else {
                err = MPI_SUCCESS;
            }
        } else {
            Request::recv(ptmp, rcount, rdtype, i,
                                    COLL_TAG_GATHER,
                                    comm, MPI_STATUS_IGNORE);
            err = MPI_SUCCESS;
        }
        if (MPI_SUCCESS != err) {
            return err;
        }
    }

    /* All done */

    return MPI_SUCCESS;
}

}
}
