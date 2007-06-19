#define DEFAULT_POWER 100

#define MPI_ANY_SOURCE -1

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

#include <stdlib.h>
#include <msg/msg.h>

typedef enum { MPI_PORT = 0, SEND_SYNC_PORT, RECV_SYNC_PORT, MAX_CHANNEL } channel_t;

// MPI_Comm
struct smpi_mpi_communicator_t {
  int id;
  int size;
  int barrier;
  smx_host_t *hosts;
  smx_process_t *processes;
};
typedef struct smpi_mpi_communicator_t smpi_mpi_communicator_t;
typedef smpi_mpi_communicator_t *MPI_Comm;
extern smpi_mpi_communicator_t smpi_mpi_comm_world;
#define MPI_COMM_WORLD (&smpi_mpi_comm_world)

// MPI_Status
struct smpi_mpi_status_t {
  int MPI_SOURCE;
};
typedef struct smpi_mpi_status_t smpi_mpi_status_t;
typedef smpi_mpi_status_t MPI_Status;
extern smpi_mpi_status_t smpi_mpi_status_ignore;
#define MPI_STATUS_IGNORE (&smpi_mpi_status_ignore)

// MPI_Datatype
struct smpi_mpi_datatype_t {
//  int type;
  size_t size;
};
typedef struct smpi_mpi_datatype_t smpi_mpi_datatype_t;
typedef smpi_mpi_datatype_t *MPI_Datatype;
// FIXME: add missing datatypes
extern smpi_mpi_datatype_t smpi_mpi_byte;
#define MPI_BYTE (&smpi_mpi_byte)
extern smpi_mpi_datatype_t smpi_mpi_int;
#define MPI_INT (&smpi_mpi_int)
extern smpi_mpi_datatype_t smpi_mpi_double;
#define MPI_DOUBLE (&smpi_mpi_double)

struct smpi_waitlist_node_t {
  smx_process_t process;
  struct smpi_waitlist_node_t *next;
};
typedef struct smpi_waitlist_node_t smpi_waitlist_node_t;

// FIXME: maybe it isn't appropriate to have the next pointer inside
// MPI_Request
struct smpi_mpi_request_t {
  void *buf;
  int count;
  smpi_mpi_datatype_t *datatype;
  int src;
  int dst;
  int tag;
  smpi_mpi_communicator_t *comm;
  short int completed;
  smpi_waitlist_node_t *waitlist;
  struct smpi_mpi_request_t *next;
  int fwdthrough;
};
typedef struct smpi_mpi_request_t smpi_mpi_request_t;
typedef smpi_mpi_request_t *MPI_Request;

// MPI_Op
struct smpi_mpi_op_t {
  void (*func)(void *x, void *y, void *z);
};
typedef struct smpi_mpi_op_t smpi_mpi_op_t;
typedef smpi_mpi_op_t *MPI_Op;
extern smpi_mpi_op_t smpi_mpi_land;
#define MPI_LAND (&smpi_mpi_land)
extern smpi_mpi_op_t smpi_mpi_sum;
#define MPI_SUM (&smpi_mpi_sum)

// smpi_received_t
struct smpi_received_t {
  int commid;
  int src;
  int dst;
  int tag;
  int fwdthrough;
  void *data;
  struct smpi_received_t *next;
};
typedef struct smpi_received_t smpi_received_t;

// sender/receiver (called by main routine)
int smpi_sender(int argc, char *argv[]);
int smpi_receiver(int argc, char *argv[]);

// smpi functions
int smpi_comm_rank(smpi_mpi_communicator_t *comm, smx_host_t host);
void smpi_isend(smpi_mpi_request_t*);
void smpi_irecv(smpi_mpi_request_t*);
void smpi_barrier(smpi_mpi_communicator_t *comm);
void smpi_wait(smpi_mpi_request_t *request, smpi_mpi_status_t *status);
void smpi_wait_all(int count, smpi_mpi_request_t **requests, smpi_mpi_status_t *statuses);
void smpi_wait_all_nostatus(int count, smpi_mpi_request_t **requests);
void smpi_bench_begin();
void smpi_bench_end();
int smpi_create_request(void *buf, int count, smpi_mpi_datatype_t *datatype, int src, int dst, int tag, smpi_mpi_communicator_t *comm, smpi_mpi_request_t **request);
unsigned int smpi_sleep(unsigned int);
void smpi_exit(int);
