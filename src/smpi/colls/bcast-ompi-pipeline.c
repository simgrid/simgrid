 #include "colls_private.h"


#define MAXTREEFANOUT 32

#define COLL_TUNED_COMPUTED_SEGCOUNT(SEGSIZE, TYPELNG, SEGCOUNT)        \
    if( ((SEGSIZE) >= (TYPELNG)) &&                                     \
        ((SEGSIZE) < ((TYPELNG) * (SEGCOUNT))) ) {                      \
        size_t residual;                                                \
        (SEGCOUNT) = (int)((SEGSIZE) / (TYPELNG));                      \
        residual = (SEGSIZE) - (SEGCOUNT) * (TYPELNG);                  \
        if( residual > ((TYPELNG) >> 1) )                               \
            (SEGCOUNT)++;                                               \
    }                                                                   \

 typedef struct ompi_coll_tree_t {
        int32_t tree_root;
        int32_t tree_fanout;
        int32_t tree_bmtree;
        int32_t tree_prev;
        int32_t tree_next[MAXTREEFANOUT];
        int32_t tree_nextsize;
    } ompi_coll_tree_t;

    ompi_coll_tree_t*
    ompi_coll_tuned_topo_build_chain( int fanout,
                                     MPI_Comm com,
                                     int root );

ompi_coll_tree_t*
ompi_coll_tuned_topo_build_chain( int fanout,
                                  MPI_Comm comm,
                                  int root )
{
    int rank, size;
    int srank; /* shifted rank */
    int i,maxchainlen;
    int mark,head,len;
    ompi_coll_tree_t *chain;

    XBT_DEBUG("coll:tuned:topo:build_chain fo %d rt %d", fanout, root);

    /* 
     * Get size and rank of the process in this communicator 
     */
    size = smpi_comm_size(comm);
    rank = smpi_comm_rank(comm);

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
    chain = (ompi_coll_tree_t*)malloc( sizeof(ompi_coll_tree_t) );
    if (!chain) {
        XBT_DEBUG("coll:tuned:topo:build_chain PANIC out of memory");
        fflush(stdout);
        return NULL;
    }
    chain->tree_root     = MPI_UNDEFINED;
    chain->tree_nextsize = -1;
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
        for( i = 1; i < fanout; i++ ) {
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

smpi_coll_tuned_bcast_ompi_pipeline( void* buffer,
                                      int original_count, 
                                      MPI_Datatype datatype, 
                                      int root,
                                      MPI_Comm comm)
{
    int count_by_segment = original_count;
    size_t type_size;
    int segsize;
    //mca_coll_tuned_module_t *tuned_module = (mca_coll_tuned_module_t*) module;
    //mca_coll_tuned_comm_t *data = tuned_module->tuned_data;
    
//    return ompi_coll_tuned_bcast_intra_generic( buffer, count, datatype, root, comm, module,
//                                                count_by_segment, data->cached_pipeline );
    ompi_coll_tree_t * tree = ompi_coll_tuned_topo_build_chain( 1, comm, root );
    int err = 0, line, i;
    int rank, size;
    int segindex;
    int num_segments; /* Number of segments */
    int sendcount;    /* number of elements sent in this segment */ 
    size_t realsegsize;
    char *tmpbuf;
    ptrdiff_t extent;
    MPI_Request recv_reqs[2] = {MPI_REQUEST_NULL, MPI_REQUEST_NULL};
    MPI_Request *send_reqs = NULL;
    int req_index;
    
    /**
     * Determine number of elements sent per operation.
     */
    type_size = smpi_datatype_size(datatype);

    size = smpi_comm_size(comm);
    rank = smpi_comm_rank(comm);
    xbt_assert( size > 1 );


    const double a_p16  = 3.2118e-6; /* [1 / byte] */
    const double b_p16  = 8.7936;   
    const double a_p64  = 2.3679e-6; /* [1 / byte] */
    const double b_p64  = 1.1787;     
    const double a_p128 = 1.6134e-6; /* [1 / byte] */
    const double b_p128 = 2.1102;
    size_t message_size;

    /* else we need data size for decision function */
    message_size = type_size * (unsigned long)original_count;   /* needed for decision */

    if (size < (a_p128 * message_size + b_p128)) {
            //Pipeline with 128KB segments 
            segsize = 1024  << 7;
    }else if (size < (a_p64 * message_size + b_p64)) {
            // Pipeline with 64KB segments 
            segsize = 1024 << 6;
    }else if (size < (a_p16 * message_size + b_p16)) {
            //Pipeline with 16KB segments 
            segsize = 1024 << 4;
    }

    COLL_TUNED_COMPUTED_SEGCOUNT( segsize, type_size, count_by_segment );

    XBT_DEBUG("coll:tuned:bcast_intra_pipeline rank %d ss %5d type_size %lu count_by_segment %d",
                 smpi_comm_rank(comm), segsize, (unsigned long)type_size, count_by_segment);



    extent = smpi_datatype_get_extent (datatype);
    num_segments = (original_count + count_by_segment - 1) / count_by_segment;
    realsegsize = count_by_segment * extent;
    
    /* Set the buffer pointers */
    tmpbuf = (char *) buffer;

    if( tree->tree_nextsize != 0 ) {
        send_reqs = xbt_new(MPI_Request, tree->tree_nextsize  );
    }

    /* Root code */
    if( rank == root ) {
        /* 
           For each segment:
           - send segment to all children.
             The last segment may have less elements than other segments.
        */
        sendcount = count_by_segment;
        for( segindex = 0; segindex < num_segments; segindex++ ) {
            if( segindex == (num_segments - 1) ) {
                sendcount = original_count - segindex * count_by_segment;
            }
            for( i = 0; i < tree->tree_nextsize; i++ ) { 
                send_reqs[i] = smpi_mpi_isend(tmpbuf, sendcount, datatype,
                                         tree->tree_next[i], 
                                         777, comm);
           } 

            /* complete the sends before starting the next sends */
            smpi_mpi_waitall( tree->tree_nextsize, send_reqs, 
                                         MPI_STATUSES_IGNORE );

            /* update tmp buffer */
            tmpbuf += realsegsize;

        }
    } 
    
    /* Intermediate nodes code */
    else if( tree->tree_nextsize > 0 ) { 
        /* 
           Create the pipeline. 
           1) Post the first receive
           2) For segments 1 .. num_segments
              - post new receive
              - wait on the previous receive to complete
              - send this data to children
           3) Wait on the last segment
           4) Compute number of elements in last segment.
           5) Send the last segment to children
         */
        req_index = 0;
        recv_reqs[req_index]=smpi_mpi_irecv(tmpbuf, count_by_segment, datatype,
                           tree->tree_prev, 777,
                           comm);
        
        for( segindex = 1; segindex < num_segments; segindex++ ) {
            
            req_index = req_index ^ 0x1;
            
            /* post new irecv */
            recv_reqs[req_index]= smpi_mpi_irecv( tmpbuf + realsegsize, count_by_segment,
                                datatype, tree->tree_prev, 
                                777, 
                                comm);
            
            /* wait for and forward the previous segment to children */
            smpi_mpi_wait( &recv_reqs[req_index ^ 0x1], 
                                     MPI_STATUSES_IGNORE );
            
            for( i = 0; i < tree->tree_nextsize; i++ ) { 
                send_reqs[i]=smpi_mpi_isend(tmpbuf, count_by_segment, datatype,
                                         tree->tree_next[i], 
                                         777, comm );
            } 
            
            /* complete the sends before starting the next iteration */
            smpi_mpi_waitall( tree->tree_nextsize, send_reqs, 
                                         MPI_STATUSES_IGNORE );
            
            /* Update the receive buffer */
            tmpbuf += realsegsize;
        }

        /* Process the last segment */
        smpi_mpi_wait( &recv_reqs[req_index], MPI_STATUSES_IGNORE );
        sendcount = original_count - (num_segments - 1) * count_by_segment;
        for( i = 0; i < tree->tree_nextsize; i++ ) {
            send_reqs[i] = smpi_mpi_isend(tmpbuf, sendcount, datatype,
                                     tree->tree_next[i], 
                                     777, comm);
        }
        
        smpi_mpi_waitall( tree->tree_nextsize, send_reqs, 
                                     MPI_STATUSES_IGNORE );
    }
  
    /* Leaf nodes */
    else {
        /* 
           Receive all segments from parent in a loop:
           1) post irecv for the first segment
           2) for segments 1 .. num_segments
              - post irecv for the next segment
              - wait on the previous segment to arrive
           3) wait for the last segment
        */
        req_index = 0;
        recv_reqs[req_index] = smpi_mpi_irecv(tmpbuf, count_by_segment, datatype,
                                 tree->tree_prev, 777,
                                 comm);

        for( segindex = 1; segindex < num_segments; segindex++ ) {
            req_index = req_index ^ 0x1;
            tmpbuf += realsegsize;
            /* post receive for the next segment */
            recv_reqs[req_index] = smpi_mpi_irecv(tmpbuf, count_by_segment, datatype, 
                                     tree->tree_prev, 777, 
                                     comm);
            /* wait on the previous segment */
            smpi_mpi_wait( &recv_reqs[req_index ^ 0x1], 
                                     MPI_STATUS_IGNORE );
        }

        smpi_mpi_wait( &recv_reqs[req_index], MPI_STATUS_IGNORE );
    }

    if( NULL != send_reqs ) free(send_reqs);

    return (MPI_SUCCESS);
}
