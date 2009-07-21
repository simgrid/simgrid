#ifndef SMPI_H
#define SMPI_H

#include <stddef.h>
#include <sys/time.h>
#include <xbt/misc.h>
#include <xbt/function_types.h>

SG_BEGIN_DECL()
#define SMPI_RAND_SEED 5
#define MPI_ANY_SOURCE -1
#define MPI_ANY_TAG -1
#define MPI_UNDEFINED -1
// errorcodes
#define MPI_SUCCESS     0
#define MPI_ERR_COMM    1
#define MPI_ERR_ARG     2
#define MPI_ERR_TYPE    3
#define MPI_ERR_REQUEST 4
#define MPI_ERR_INTERN  5
#define MPI_ERR_COUNT   6
#define MPI_ERR_RANK    7
#define MPI_ERR_TAG     8
#define MPI_ERR_TRUNCATE 9
// MPI_Comm
     typedef struct smpi_mpi_communicator_t *smpi_mpi_communicator_t;
     typedef smpi_mpi_communicator_t MPI_Comm;

// MPI_Datatype
     typedef struct smpi_mpi_datatype_t *smpi_mpi_datatype_t;
     typedef smpi_mpi_datatype_t MPI_Datatype;

// MPI_Request
     typedef struct smpi_mpi_request_t *smpi_mpi_request_t;
     typedef smpi_mpi_request_t MPI_Request;

// MPI_Op
     typedef struct smpi_mpi_op_t *smpi_mpi_op_t;
     typedef smpi_mpi_op_t MPI_Op;

// MPI_Status
     struct smpi_mpi_status_t {
       int MPI_SOURCE;
       int MPI_TAG;
       int MPI_ERROR;
     };
     typedef struct smpi_mpi_status_t smpi_mpi_status_t;
     typedef smpi_mpi_status_t MPI_Status;

// global SMPI data structure
     typedef struct smpi_mpi_global_t {

       smpi_mpi_communicator_t mpi_comm_world;

       smpi_mpi_datatype_t mpi_byte;
       smpi_mpi_datatype_t mpi_char;
       smpi_mpi_datatype_t mpi_int;
       smpi_mpi_datatype_t mpi_float;
       smpi_mpi_datatype_t mpi_double;

       smpi_mpi_op_t mpi_land;
       smpi_mpi_op_t mpi_sum;
       smpi_mpi_op_t mpi_prod;
       smpi_mpi_op_t mpi_min;
       smpi_mpi_op_t mpi_max;

     } s_smpi_mpi_global_t;
     typedef struct smpi_mpi_global_t *smpi_mpi_global_t;
     extern smpi_mpi_global_t smpi_mpi_global;

#define MPI_COMM_WORLD    (smpi_mpi_global->mpi_comm_world)
#define MPI_COMM_NULL     NULL

#define MPI_STATUS_IGNORE NULL
#define MPI_Aint          ptrdiff_t

#define MPI_BYTE          (smpi_mpi_global->mpi_byte)
#define MPI_CHAR          (smpi_mpi_global->mpi_char)
#define MPI_INT           (smpi_mpi_global->mpi_int)
#define MPI_FLOAT         (smpi_mpi_global->mpi_float)
#define MPI_DOUBLE        (smpi_mpi_global->mpi_double)

#define MPI_LAND          (smpi_mpi_global->mpi_land)
#define MPI_SUM           (smpi_mpi_global->mpi_sum)
#define MPI_PROD          (smpi_mpi_global->mpi_prod)
#define MPI_MIN           (smpi_mpi_global->mpi_min)
#define MPI_MAX           (smpi_mpi_global->mpi_max)

// MPI macros
#define MPI_Init(a, b) SMPI_MPI_Init(a, b)
#define MPI_Finalize() SMPI_MPI_Finalize()
#define MPI_Abort(a, b) SMPI_MPI_Abort(a, b)
#define MPI_Comm_size(a, b) SMPI_MPI_Comm_size(a, b)
#define MPI_Comm_split(a, b, c, d) SMPI_MPI_Comm_split(a, b, c, d)
#define MPI_Comm_rank(a, b) SMPI_MPI_Comm_rank(a, b)
#define MPI_Type_size(a, b) SMPI_MPI_Type_size(a, b)
#define MPI_Type_get_extent(a, b, c) SMPI_MPI_Type_get_extent(a, b, c)
#define MPI_Type_lb(a, b) SMPI_MPI_Type_lb(a, b)
#define MPI_Type_ub(a, b) SMPI_MPI_Type_ub(a, b)

#define MPI_Barrier(a) SMPI_MPI_Barrier(a)
#define MPI_Irecv(a, b, c, d, e, f, g) SMPI_MPI_Irecv(a, b, c, d, e, f, g)
#define MPI_Recv(a, b, c, d, e, f, g) SMPI_MPI_Recv(a, b, c, d, e, f, g)
#define MPI_Isend(a, b, c, d, e, f, g) SMPI_MPI_Isend(a, b, c, d, e, f, g)
#define MPI_Send(a, b, c, d, e, f) SMPI_MPI_Send(a, b, c, d, e, f)
#define MPI_Sendrecv( a, b, c, d, e, f, g, h, i, j, k, l) SMPI_MPI_Sendrecv(a, b, c, d, e, f, g, h, i, j, k, l) 
#define MPI_Bcast(a, b, c, d, e) SMPI_MPI_Bcast(a, b, c, d, e)
#define MPI_Wait(a, b) SMPI_MPI_Wait(a, b)
#define MPI_Waitall(a, b, c) SMPI_MPI_Waitall(a, b, c)
#define MPI_Waitany(a, b, c, d) SMPI_MPI_Waitany(a, b, c, d)
#define MPI_Wtime() SMPI_MPI_Wtime()
#define MPI_Reduce( a, b, c, d, e, f, g) SMPI_MPI_Reduce( a, b, c, d, e, f, g)
#define MPI_Allreduce( a, b, c, d, e, f) SMPI_MPI_Allreduce( a, b, c, d, e, f)
#define MPI_Scatter( a, b, c, d, e, f, g, h )  SMPI_MPI_Scatter( a, b, c, d, e, f, g, h)
#define MPI_Alltoall( a, b, c, d, e, f, g )  SMPI_MPI_Alltoall( a, b, c, d, e, f, g)

// SMPI Functions
XBT_PUBLIC(int) SMPI_MPI_Init(int *argc, char ***argv);
XBT_PUBLIC(int) SMPI_MPI_Finalize(void);
XBT_PUBLIC(int) SMPI_MPI_Abort(MPI_Comm comm, int errorcode);
XBT_PUBLIC(int) SMPI_MPI_Comm_size(MPI_Comm comm, int *size);
XBT_PUBLIC(int) SMPI_MPI_Comm_rank(MPI_Comm comm, int *rank);
XBT_PUBLIC(int) SMPI_MPI_Type_size(MPI_Datatype datatype, size_t * size);
XBT_PUBLIC(int) SMPI_MPI_Type_get_extent(MPI_Datatype datatype, MPI_Aint* lb, MPI_Aint *extent);
XBT_PUBLIC(int) SMPI_MPI_Type_lb(MPI_Datatype datatype, MPI_Aint* disp);
XBT_PUBLIC(int) SMPI_MPI_Type_ub(MPI_Datatype datatype, MPI_Aint* disp);


XBT_PUBLIC(int) SMPI_MPI_Barrier(MPI_Comm comm);
XBT_PUBLIC(int) SMPI_MPI_Irecv(void *buf, int count, MPI_Datatype datatype,
                               int src, int tag, MPI_Comm comm,
                               MPI_Request * request);
XBT_PUBLIC(int) SMPI_MPI_Recv(void *buf, int count, MPI_Datatype datatype,
                              int src, int tag, MPI_Comm comm,
                              MPI_Status * status);
XBT_PUBLIC(int) SMPI_MPI_Isend(void *buf, int count, MPI_Datatype datatype,
                               int dst, int tag, MPI_Comm comm,
                               MPI_Request * request);
XBT_PUBLIC(int) SMPI_MPI_Send(void *buf, int count, MPI_Datatype datatype,
                              int dst, int tag, MPI_Comm comm);

XBT_PUBLIC(int) SMPI_MPI_Sendrecv(void *sendbuf, int sendcount, MPI_Datatype sendtype, int dest, int sendtag, 
		                  void *recvbuf, int recvcount, MPI_Datatype recvtype, int source, int recvtag, 
					MPI_Comm comm, MPI_Status *status);

XBT_PUBLIC(int) SMPI_MPI_Bcast(void *buf, int count, MPI_Datatype datatype,
                               int root, MPI_Comm comm);
XBT_PUBLIC(int) SMPI_MPI_Wait(MPI_Request * request, MPI_Status * status);
XBT_PUBLIC(int) SMPI_MPI_Waitall(int count, MPI_Request requests[],
                                 MPI_Status status[]);
XBT_PUBLIC(int) SMPI_MPI_Waitany(int count, MPI_Request requests[],
                                 int *index, MPI_Status status[]);
XBT_PUBLIC(int) SMPI_MPI_Comm_split(MPI_Comm comm, int color, int key,
                                    MPI_Comm * comm_out);
XBT_PUBLIC(double) SMPI_MPI_Wtime(void);

XBT_PUBLIC(int) SMPI_MPI_Reduce(void *sendbuf, void *recvbuf, int count,
                                MPI_Datatype datatype, MPI_Op op, int root,
                                MPI_Comm comm);
XBT_PUBLIC(int) SMPI_MPI_Allreduce(void *sendbuf, void *recvbuf, int count,
		                    MPI_Datatype datatype, MPI_Op op, MPI_Comm comm);

XBT_PUBLIC(int) SMPI_MPI_Scatter(void *sendbuf, int sendcount, MPI_Datatype datatype,
		                     void *recvbuf, int recvcount, MPI_Datatype recvtype,int root, MPI_Comm comm);

XBT_PUBLIC(int) SMPI_MPI_Alltoall(void *sendbuf, int sendcount, MPI_Datatype datatype,
		                     void *recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm);

// smpi functions
XBT_IMPORT_NO_EXPORT(int) smpi_simulated_main(int argc, char **argv);
XBT_PUBLIC(unsigned int) smpi_sleep(unsigned int);
XBT_PUBLIC(void) smpi_exit(int);
XBT_PUBLIC(int) smpi_gettimeofday(struct timeval *tv, struct timezone *tz);

XBT_PUBLIC(void) smpi_do_once_1(const char *file, int line);
XBT_PUBLIC(int) smpi_do_once_2(void);
XBT_PUBLIC(void) smpi_do_once_3(void);

#define SMPI_DO_ONCE for (smpi_do_once_1(__FILE__, __LINE__); smpi_do_once_2(); smpi_do_once_3())

SG_END_DECL()
#endif
