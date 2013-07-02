
#include "colls_private.h"
/*  
 * Linear functions are copied from the basic coll module.  For
 * some small number of nodes and/or small data sizes they are just as
 * fast as tuned/tree based segmenting operations and as such may be
 * selected by the decision functions.  These are copied into this module
 * due to the way we select modules in V1. i.e. in V2 we will handle this
 * differently and so will not have to duplicate code.  
 * GEF Oct05 after asking Jeff.  
 */
int
smpi_coll_tuned_alltoallv_ompi_basic_linear(void *sbuf, int *scounts, int *sdisps,
                                            MPI_Datatype sdtype,
                                            void *rbuf, int *rcounts, int *rdisps,
                                            MPI_Datatype rdtype,
                                            MPI_Comm comm)
{
    int i, size, rank;
    char *psnd, *prcv;
    int nreqs;
    ptrdiff_t sext, rext;
    MPI_Request *preq;
    size = smpi_comm_size(comm);
    rank = smpi_comm_rank(comm);
    MPI_Request *ireqs= xbt_malloc(sizeof(MPI_Request) * size * 2);
    XBT_DEBUG(
                 "coll:tuned:alltoallv_intra_basic_linear rank %d", rank);

    sext=smpi_datatype_get_extent(sdtype);
    rext=smpi_datatype_get_extent(rdtype);

    /* Simple optimization - handle send to self first */
    psnd = ((char *) sbuf) + (sdisps[rank] * sext);
    prcv = ((char *) rbuf) + (rdisps[rank] * rext);
    if (0 != scounts[rank]) {
        smpi_datatype_copy(psnd, scounts[rank], sdtype,
                              prcv, rcounts[rank], rdtype);
    }

    /* If only one process, we're done. */
    if (1 == size) {
        return MPI_SUCCESS;
    }

    /* Now, initiate all send/recv to/from others. */
    nreqs = 0;
    preq = ireqs;

    /* Post all receives first */
    for (i = 0; i < size; ++i) {
        if (i == rank || 0 == rcounts[i]) {
            continue;
        }

        prcv = ((char *) rbuf) + (rdisps[i] * rext);

        *preq = smpi_irecv_init(prcv, rcounts[i], rdtype,
                                      i, COLL_TAG_ALLTOALLV, comm
                                      );
        preq++;
        ++nreqs;
        
    }

    /* Now post all sends */
    for (i = 0; i < size; ++i) {
        if (i == rank || 0 == scounts[i]) {
            continue;
        }

        psnd = ((char *) sbuf) + (sdisps[i] * sext);
        *preq=smpi_isend_init(psnd, scounts[i], sdtype,
                                      i, COLL_TAG_ALLTOALLV, comm
                                      );
        preq++;
        ++nreqs;
    }

    /* Start your engines.  This will never return an error. */
    smpi_mpi_startall(nreqs, ireqs);

    /* Wait for them all.  If there's an error, note that we don't care
     * what the error was -- just that there *was* an error.  The PML
     * will finish all requests, even if one or more of them fail.
     * i.e., by the end of this call, all the requests are free-able.
     * So free them anyway -- even if there was an error, and return the
     * error after we free everything. */
    smpi_mpi_waitall(nreqs, ireqs,
                                MPI_STATUSES_IGNORE);

    /* Free the requests. */
    for (i = 0; i < nreqs; ++i) {
      if(ireqs[i]!=MPI_REQUEST_NULL)smpi_mpi_request_free(&ireqs[i]);
    }

    return MPI_SUCCESS;
}

