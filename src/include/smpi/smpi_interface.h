#ifndef _SMPI_INTERFACE_H
#define _SMPI_INTERFACE_H
#include "smpi/smpi.h"

/********** Tracing **********/
/* from smpi_instr.c */
void TRACE_smpi_alloc(void);
void TRACE_smpi_release(void);
void TRACE_smpi_ptp_in(int rank, int src, int dst, const char *operation);
void TRACE_smpi_ptp_out(int rank, int src, int dst, const char *operation);
void TRACE_smpi_send(int rank, int src, int dst);
void TRACE_smpi_recv(int rank, int src, int dst);
void TRACE_smpi_init(int rank);
void TRACE_smpi_finalize(int rank);

/** \brief MPI collective description
 */

typedef struct mpi_coll_description {
  const char *name;
  const char *description;
  void *coll;
} s_mpi_coll_description_t, *mpi_coll_description_t;

/** \ingroup MPI allgather
 *  \brief The list of all available allgather collectives
 */
XBT_PUBLIC_DATA(s_mpi_coll_description_t) mpi_coll_allgather_description[];
XBT_PUBLIC_DATA(int (*mpi_coll_allgather_fun)
                (void *, int, MPI_Datatype, void *, int, MPI_Datatype,
                 MPI_Comm));


/** \ingroup MPI allreduce
 *  \brief The list of all available allgather collectives
 */
XBT_PUBLIC_DATA(s_mpi_coll_description_t) mpi_coll_allreduce_description[];
XBT_PUBLIC_DATA(int (*mpi_coll_allreduce_fun)
                (void *sbuf, void *rbuf, int rcount, MPI_Datatype dtype,
                 MPI_Op op, MPI_Comm comm));


/** \ingroup MPI alltoall
 *  \brief The list of all available alltoall collectives
 */
XBT_PUBLIC_DATA(s_mpi_coll_description_t) mpi_coll_alltoall_description[];
XBT_PUBLIC_DATA(int (*mpi_coll_alltoall_fun)
                (void *, int, MPI_Datatype, void *, int, MPI_Datatype,
                 MPI_Comm));

/** \ingroup MPI alltoallv
 *  \brief The list of all available alltoallv collectives
 */
XBT_PUBLIC_DATA(s_mpi_coll_description_t) mpi_coll_alltoallv_description[];
XBT_PUBLIC_DATA(int (*mpi_coll_alltoallv_fun)
                (void *, int*, int*, MPI_Datatype, void *, int*, int*, MPI_Datatype,
                 MPI_Comm));


/** \ingroup MPI bcast
 *  \brief The list of all available bcast collectives
 */
XBT_PUBLIC_DATA(s_mpi_coll_description_t) mpi_coll_bcast_description[];
XBT_PUBLIC_DATA(int (*mpi_coll_bcast_fun)
                (void *buf, int count, MPI_Datatype datatype, int root,
                 MPI_Comm com));


/** \ingroup MPI reduce
 *  \brief The list of all available reduce collectives
 */
XBT_PUBLIC_DATA(s_mpi_coll_description_t) mpi_coll_reduce_description[];
XBT_PUBLIC_DATA(int (*mpi_coll_reduce_fun)
                (void *buf, void *rbuf, int count, MPI_Datatype datatype,
                 MPI_Op op, int root, MPI_Comm comm));


XBT_PUBLIC(void) coll_help(const char *category,
                           s_mpi_coll_description_t * table);
XBT_PUBLIC(int) find_coll_description(s_mpi_coll_description_t * table,
                                      const char *name);


#endif                          /* _SMPI_INTERFAC_H */
