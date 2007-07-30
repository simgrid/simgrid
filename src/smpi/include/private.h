#include "xbt/mallocator.h"
#include "xbt/xbt_os_time.h"

#define SMPI_DEFAULT_SPEED 100
#define SMPI_REQUEST_MALLOCATOR_SIZE 100
#define SMPI_MESSAGE_MALLOCATOR_SIZE 100

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

	int              *benchmarking_flags;
	smx_mutex_t      *benchmarking_flags_mutexes;

} s_SMPI_Global_t, *SMPI_Global_t;

extern SMPI_Global_t smpi_global;

// smpi_received_message_t
struct smpi_received_message_t {
	smpi_mpi_communicator_t *comm;
	int src;
	int dst;
	int tag;
	void *buf;
};
typedef struct smpi_received_message_t smpi_received_message_t;
