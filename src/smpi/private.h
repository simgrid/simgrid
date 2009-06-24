#ifndef SMPI_PRIVATE_H
#define SMPI_PRIVATE_H

#include "xbt/mallocator.h"
#include "xbt/xbt_os_time.h"

#include "simix/simix.h"

#include "smpi/smpi.h"

#define SMPI_DEFAULT_SPEED 100
#define SMPI_REQUEST_MALLOCATOR_SIZE 100
#define SMPI_MESSAGE_MALLOCATOR_SIZE 100

// smpi mpi communicator
typedef struct smpi_mpi_communicator_t {
  int size;
  int barrier_count;
  smx_mutex_t barrier_mutex;
  smx_cond_t barrier_cond;

  int *rank_to_index_map;
  int *index_to_rank_map;

} s_smpi_mpi_communicator_t;

// smpi mpi datatype
typedef struct smpi_mpi_datatype_t {
  size_t size;
} s_smpi_mpi_datatype_t;

// smpi mpi request
typedef struct smpi_mpi_request_t {
  smpi_mpi_communicator_t comm;
  int src;
  int dst;
  int tag;

  void *buf;
  int count;
  smpi_mpi_datatype_t datatype;

  short int completed:1;

  smx_mutex_t mutex;
  smx_cond_t cond;

  void *data;
  int forward;

} s_smpi_mpi_request_t;

// smpi mpi op
typedef struct smpi_mpi_op_t {
  void (*func) (void *a, void *b, int *length, MPI_Datatype * datatype);
} s_smpi_mpi_op_t;

// smpi received message
typedef struct smpi_received_message_t {
  smpi_mpi_communicator_t comm;
  int src;
  int tag;

  void *buf;

  void *data;
  int forward;

} s_smpi_received_message_t;
typedef struct smpi_received_message_t *smpi_received_message_t;

typedef struct smpi_do_once_duration_node_t {
  char *file;
  int line;
  double duration;
  struct smpi_do_once_duration_node_t *next;
} s_smpi_do_once_duration_node_t;
typedef struct smpi_do_once_duration_node_t *smpi_do_once_duration_node_t;

typedef struct smpi_global_t {

  // config vars
  double reference_speed;

  // state vars

  smx_host_t *hosts;
  int host_count;
  xbt_mallocator_t request_mallocator;
  xbt_mallocator_t message_mallocator;

  // FIXME: request queues should be moved to host data...
  xbt_fifo_t *pending_send_request_queues;
  xbt_fifo_t *received_message_queues;

  smx_process_t *main_processes;

  xbt_os_timer_t timer;
  smx_mutex_t timer_mutex;
  smx_cond_t timer_cond;

  // keeps track of previous times
  smpi_do_once_duration_node_t do_once_duration_nodes;
  smx_mutex_t do_once_mutex;
  double *do_once_duration;

} s_smpi_global_t;
typedef struct smpi_global_t *smpi_global_t;
extern smpi_global_t smpi_global;

typedef struct smpi_host_data_t {
  int index;
  smx_mutex_t mutex;
  smx_cond_t cond;

  smx_process_t main;
  smx_process_t sender;
  smx_process_t receiver;

  int finalize; /* for main stopping sender&receiver */

  xbt_fifo_t pending_recv_request_queue;
} s_smpi_host_data_t;
typedef struct smpi_host_data_t *smpi_host_data_t;

// function prototypes
void smpi_process_init(void);
void smpi_process_finalize(void);
int smpi_mpi_comm_rank(smpi_mpi_communicator_t comm);

int smpi_mpi_barrier(smpi_mpi_communicator_t comm);
int smpi_mpi_isend(smpi_mpi_request_t request);
int smpi_mpi_irecv(smpi_mpi_request_t request);
int smpi_mpi_wait(smpi_mpi_request_t request, smpi_mpi_status_t * status);

void smpi_execute(double duration);
void smpi_start_timer(void);
double smpi_stop_timer(void);
void smpi_bench_begin(void);
void smpi_bench_end(void);
void smpi_bench_skip(void);

void smpi_global_init(void);
void smpi_global_destroy(void);
int smpi_host_index(void);
smx_mutex_t smpi_host_mutex(void);
smx_cond_t smpi_host_cond(void);
int smpi_run_simulation(int *argc, char **argv);
int smpi_create_request(void *buf, int count, smpi_mpi_datatype_t datatype,
                        int src, int dst, int tag,
                        smpi_mpi_communicator_t comm,
                        smpi_mpi_request_t * request);

int smpi_sender(int argc,char*argv[]);
int smpi_receiver(int argc, char*argv[]);

#endif
