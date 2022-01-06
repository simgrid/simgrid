/* Copyright (c) 2013-2022. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "../colls_private.hpp"
/*
 * Linear functions are copied from the basic coll module.  For
 * some small number of nodes and/or small data sizes they are just as
 * fast as tuned/tree based segmenting operations and as such may be
 * selected by the decision functions.  These are copied into this module
 * due to the way we select modules in V1. i.e. in V2 we will handle this
 * differently and so will not have to duplicate code.
 * GEF Oct05 after asking Jeff.
 */
namespace simgrid{
namespace smpi{
int alltoallv__ompi_basic_linear(const void *sbuf, const int *scounts, const int *sdisps,
                                 MPI_Datatype sdtype,
                                 void *rbuf, const int *rcounts, const int *rdisps,
                                 MPI_Datatype rdtype,
                                 MPI_Comm comm)
{
  int size = comm->size();
  int rank = comm->rank();
  XBT_DEBUG("coll:tuned:alltoallv_intra_basic_linear rank %d", rank);

  ptrdiff_t sext = sdtype->get_extent();
  ptrdiff_t rext = rdtype->get_extent();

  /* Simple optimization - handle send to self first */
  char* psnd = ((char*)sbuf) + (sdisps[rank] * sext);
  char* prcv = ((char*)rbuf) + (rdisps[rank] * rext);
  if (0 != scounts[rank]) {
    Datatype::copy(psnd, scounts[rank], sdtype, prcv, rcounts[rank], rdtype);
  }

  /* If only one process, we're done. */
  if (1 == size) {
    return MPI_SUCCESS;
  }

  /* Now, initiate all send/recv to/from others. */
  auto* ireqs        = new MPI_Request[size * 2];
  int nreqs          = 0;
  MPI_Request* preq  = ireqs;

  /* Post all receives first */
  for (int i = 0; i < size; ++i) {
    if (i == rank) {
      continue;
    }

    prcv = ((char*)rbuf) + (rdisps[i] * rext);

    *preq = Request::irecv_init(prcv, rcounts[i], rdtype, i, COLL_TAG_ALLTOALLV, comm);
    preq++;
    ++nreqs;
  }

  /* Now post all sends */
  for (int i = 0; i < size; ++i) {
    if (i == rank) {
      continue;
    }

    psnd  = ((char*)sbuf) + (sdisps[i] * sext);
    *preq = Request::isend_init(psnd, scounts[i], sdtype, i, COLL_TAG_ALLTOALLV, comm);
    preq++;
    ++nreqs;
  }

  /* Start your engines.  This will never return an error. */
  Request::startall(nreqs, ireqs);

  /* Wait for them all.  If there's an error, note that we don't care
   * what the error was -- just that there *was* an error.  The PML
   * will finish all requests, even if one or more of them fail.
   * i.e., by the end of this call, all the requests are free-able.
   * So free them anyway -- even if there was an error, and return the
   * error after we free everything. */
  Request::waitall(nreqs, ireqs, MPI_STATUSES_IGNORE);

  /* Free the requests. */
  for (int i = 0; i < nreqs; ++i) {
    if (ireqs[i] != MPI_REQUEST_NULL)
      Request::unref(&ireqs[i]);
  }
  delete[] ireqs;

  return MPI_SUCCESS;
}
}
}
