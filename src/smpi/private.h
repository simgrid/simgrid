#ifndef SMPI_PRIVATE_H
#define SMPI_PRIVATE_H

#include "simix/simix.h"
#include "xbt/mallocator.h"
#include "xbt/xbt_os_time.h"
#include "smpi.h"

#define SMPI_DEFAULT_SPEED 100
#define SMPI_REQUEST_MALLOCATOR_SIZE 100
#define SMPI_MESSAGE_MALLOCATOR_SIZE 100

typedef struct smpi_mpi_communicator_simdata {
	smx_host_t    *hosts;
	smx_process_t *processes;
	int            barrier_count;
	smx_mutex_t    barrier_mutex;
	smx_cond_t     barrier_cond;
} s_smpi_mpi_communicator_simdata_t;

typedef struct smpi_mpi_request_simdata {
	smx_mutex_t mutex;
	smx_cond_t  cond;
} s_smpi_mpi_request_simdata_t;

typedef struct SMPI_Global {

	// config vars
	double            reference_speed;

	// state vars
	int               root_ready:1;
	int               ready_process_count;
	smx_mutex_t       start_stop_mutex;
	smx_cond_t        start_stop_cond;

	xbt_mallocator_t  request_mallocator;
	xbt_mallocator_t  message_mallocator;

	xbt_fifo_t       *pending_send_request_queues;
	smx_mutex_t      *pending_send_request_queues_mutexes;

	xbt_fifo_t       *pending_recv_request_queues;
	smx_mutex_t      *pending_recv_request_queues_mutexes;

	xbt_fifo_t       *received_message_queues;
	smx_mutex_t      *received_message_queues_mutexes;

	smx_process_t    *sender_processes;
	smx_process_t    *receiver_processes;

	int               running_hosts_count;
	smx_mutex_t       running_hosts_count_mutex;

	xbt_os_timer_t   *timers;
	smx_mutex_t      *timers_mutexes;

} s_SMPI_Global_t, *SMPI_Global_t;
extern SMPI_Global_t smpi_global;

struct smpi_received_message_t {
	smpi_mpi_communicator_t *comm;
	int src;
	int dst;
	int tag;
	void *buf;
};
typedef struct smpi_received_message_t smpi_received_message_t;

// function prototypes
int smpi_mpi_comm_size(smpi_mpi_communicator_t *comm);
int smpi_mpi_comm_rank(smpi_mpi_communicator_t *comm, smx_host_t host);
int smpi_mpi_comm_rank_self(smpi_mpi_communicator_t *comm);
int smpi_mpi_comm_world_rank_self(void);
int smpi_sender(int argc, char **argv);
int smpi_receiver(int argc, char **argv);
void *smpi_request_new(void);
void smpi_request_free(void *pointer);
void smpi_request_reset(void *pointer);
void *smpi_message_new(void);
void smpi_message_free(void *pointer);
void smpi_message_reset(void *pointer);
void smpi_global_init(void);
void smpi_global_destroy(void);
int smpi_run_simulation(int argc, char **argv);
void smpi_mpi_land_func(void *x, void *y, void *z);
void smpi_mpi_sum_func(void *x, void *y, void *z);
void smpi_mpi_init(void);
void smpi_mpi_finalize(void);
void smpi_bench_begin(void);
void smpi_bench_end(void);
void smpi_barrier(smpi_mpi_communicator_t *comm);
int smpi_comm_rank(smpi_mpi_communicator_t *comm, smx_host_t host);
int smpi_create_request(void *buf, int count, smpi_mpi_datatype_t *datatype,
	int src, int dst, int tag, smpi_mpi_communicator_t *comm, smpi_mpi_request_t **request);
int smpi_isend(smpi_mpi_request_t *request);
int smpi_irecv(smpi_mpi_request_t *request);
void smpi_wait(smpi_mpi_request_t *request, smpi_mpi_status_t *status);

#endif
