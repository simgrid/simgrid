#include <stdio.h>

#include <signal.h>
#include <sys/time.h>
#include "xbt/xbt_portability.h"
#include "simix/simix.h"
#include "simix/private.h"
#include "smpi.h"

xbt_fifo_t *smpi_pending_send_requests      = NULL;
xbt_fifo_t *smpi_pending_recv_requests      = NULL;
xbt_fifo_t *smpi_received_messages          = NULL;

int smpi_running_hosts = 0;

smpi_mpi_communicator_t smpi_mpi_comm_world;

smpi_mpi_status_t smpi_mpi_status_ignore;

smpi_mpi_datatype_t smpi_mpi_byte;
smpi_mpi_datatype_t smpi_mpi_int;
smpi_mpi_datatype_t smpi_mpi_double;

smpi_mpi_op_t smpi_mpi_land;
smpi_mpi_op_t smpi_mpi_sum;

static xbt_os_timer_t smpi_timer;
static int smpi_benchmarking;
static double smpi_reference_speed;

// mutexes
smx_mutex_t smpi_running_hosts_mutex = NULL;
smx_mutex_t smpi_benchmarking_mutex  = NULL;
smx_mutex_t init_mutex = NULL;
smx_cond_t init_cond  = NULL;

int rootready = 0;
int readycount = 0;

XBT_LOG_NEW_DEFAULT_CATEGORY(smpi, "SMPI");

int smpi_sender(int argc, char **argv)
{
	return 0;
}

int smpi_receiver(int argc, char **argv)
{
	return 0;
}

int smpi_run_simulation(int argc, char **argv)
{
	smx_cond_t   cond           = NULL;
	smx_action_t action         = NULL;

	xbt_fifo_t   actions_failed = xbt_fifo_new();
	xbt_fifo_t   actions_done   = xbt_fifo_new();

	srand(SMPI_RAND_SEED);

	SIMIX_global_init(&argc, argv);

	init_mutex = SIMIX_mutex_init();
	init_cond  = SIMIX_cond_init();

	SIMIX_function_register("smpi_simulated_main", smpi_simulated_main);
	SIMIX_create_environment(argv[1]);
	SIMIX_launch_application(argv[2]);

	/* Prepare to display some more info when dying on Ctrl-C pressing */
	//signal(SIGINT, inthandler);

	/* Clean IO before the run */
	fflush(stdout);
	fflush(stderr);

	while (SIMIX_solve(actions_done, actions_failed) != -1.0) {
		while (action = xbt_fifo_pop(actions_failed)) {
			DEBUG1("** %s failed **", action->name);
			while (cond = xbt_fifo_pop(action->cond_list)) {
				SIMIX_cond_broadcast(cond);
			}
			SIMIX_action_destroy(action);
		}
		while (action = xbt_fifo_pop(actions_done)) {
			DEBUG1("** %s done **",action->name);
			while (cond = xbt_fifo_pop(action->cond_list)) {
				SIMIX_cond_broadcast(cond);
			}
			SIMIX_action_destroy(action);
		}
	}
	xbt_fifo_free(actions_failed);
	xbt_fifo_free(actions_done);
	INFO1("simulation time %g", SIMIX_get_clock());
	SIMIX_clean();
	return 0;
}

void smpi_mpi_land_func(void *x, void *y, void *z)
{
	*(int *)z = *(int *)x && *(int *)y;
}

void smpi_mpi_sum_func(void *x, void *y, void *z)
{
	*(int *)z = *(int *)x + *(int *)y;
}

int smpi_mpi_rank(smpi_mpi_communicator_t *comm, smx_host_t host)
{
	int i;

	for(i = comm->size - 1; i > 0 && host != comm->hosts[i]; i--);

	return i;
}

int inline smpi_mpi_rank_self(smpi_mpi_communicator_t *comm)
{
	return smpi_mpi_rank(comm, SIMIX_host_self());
}

void smpi_mpi_init()
{
	int i;
	int size;
	smx_process_t process;
	smx_host_t *hosts;
	smx_host_t host;
	double duration;

	// initialize some local variables
	host  = SIMIX_host_self();
	hosts = SIMIX_host_get_table();
	size  = SIMIX_host_get_number();

	// node 0 sets the globals
	if (host == hosts[0]) {

		// running hosts
		smpi_running_hosts_mutex          = SIMIX_mutex_init();
	        smpi_running_hosts                = size;

		// global communicator
		smpi_mpi_comm_world.size          = size;
		smpi_mpi_comm_world.barrier       = 0;
		smpi_mpi_comm_world.barrier_mutex = SIMIX_mutex_init();
		smpi_mpi_comm_world.barrier_cond  = SIMIX_cond_init();
		smpi_mpi_comm_world.hosts         = hosts;
		smpi_mpi_comm_world.processes     = xbt_new0(smx_process_t, size);
		smpi_mpi_comm_world.processes[0]  = SIMIX_process_self();

		// mpi datatypes
		smpi_mpi_byte.size                = (size_t)1;
		smpi_mpi_int.size                 = sizeof(int);
		smpi_mpi_double.size              = sizeof(double);

		// mpi operations
		smpi_mpi_land.func                = &smpi_mpi_land_func;
		smpi_mpi_sum.func                 = &smpi_mpi_sum_func;

		// smpi globals
		smpi_pending_send_requests        = xbt_new0(xbt_fifo_t, size);
		smpi_pending_recv_requests        = xbt_new0(xbt_fifo_t, size);
		smpi_received_messages            = xbt_new0(xbt_fifo_t, size);

		for(i = 0; i < size; i++) {
			smpi_pending_send_requests[i] = xbt_fifo_new();
			smpi_pending_recv_requests[i] = xbt_fifo_new();
			smpi_received_messages[i]     = xbt_fifo_new();
		}

		smpi_timer                      = xbt_os_timer_new();
		smpi_reference_speed            = SMPI_DEFAULT_SPEED;
		smpi_benchmarking               = 0;
		smpi_benchmarking_mutex         = SIMIX_mutex_init();

		// signal all nodes to perform initialization
		SIMIX_mutex_lock(init_mutex);
		rootready = 1;
		SIMIX_cond_broadcast(init_cond);
		SIMIX_mutex_unlock(init_mutex);

	} else {

		// make sure root is done before own initialization
		SIMIX_mutex_lock(init_mutex);
		if (!rootready) {
			SIMIX_cond_wait(init_cond, init_mutex);
		}
		SIMIX_mutex_unlock(init_mutex);

    		smpi_mpi_comm_world.processes[smpi_mpi_rank_self(&smpi_mpi_comm_world)] = SIMIX_process_self();

	}

	// wait for all nodes to signal initializatin complete
	SIMIX_mutex_lock(init_mutex);
	readycount++;
	if (readycount < size) {
		SIMIX_cond_wait(init_cond, init_mutex);
	} else {
		SIMIX_cond_broadcast(init_cond);
	}
	SIMIX_mutex_unlock(init_mutex);

}

void smpi_mpi_finalize()
{
	int i;

	SIMIX_mutex_lock(smpi_running_hosts_mutex);
	i = --smpi_running_hosts;
	SIMIX_mutex_unlock(smpi_running_hosts_mutex);

	if (0 >= i) {

		SIMIX_mutex_destroy(smpi_running_hosts_mutex);

		for (i = 0 ; i < smpi_mpi_comm_world.size; i++) {
			xbt_fifo_free(smpi_pending_send_requests[i]);
			xbt_fifo_free(smpi_pending_recv_requests[i]);
			xbt_fifo_free(smpi_received_messages[i]);
		}

		xbt_free(smpi_pending_send_requests);
		xbt_free(smpi_pending_recv_requests);
		xbt_free(smpi_received_messages);

		SIMIX_mutex_destroy(smpi_mpi_comm_world.barrier_mutex);
		SIMIX_cond_destroy(smpi_mpi_comm_world.barrier_cond);
		xbt_free(smpi_mpi_comm_world.processes);

		xbt_os_timer_free(smpi_timer);
	}

}

void smpi_bench_begin()
{
	xbt_assert0(!smpi_benchmarking, "Already benchmarking");
	smpi_benchmarking = 1;
	xbt_os_timer_start(smpi_timer);
	return;
}

void smpi_bench_end()
{
	double duration;
	smx_host_t host;
	smx_action_t compute_action;
	smx_mutex_t mutex;
	smx_cond_t cond;

	xbt_assert0(smpi_benchmarking, "Not benchmarking yet");
	smpi_benchmarking = 0;
	xbt_os_timer_stop(smpi_timer);
	duration = xbt_os_timer_elapsed(smpi_timer);
	host           = SIMIX_host_self();
	compute_action = SIMIX_action_sleep(host, "computation", duration * SMPI_DEFAULT_SPEED);
	mutex          = SIMIX_mutex_init();
	cond           = SIMIX_cond_init();
	SIMIX_mutex_lock(mutex);
	SIMIX_register_condition_to_action(compute_action, cond);
	SIMIX_register_action_to_condition(compute_action, cond);
	SIMIX_cond_wait(cond, mutex);
	SIMIX_mutex_unlock(mutex);
	SIMIX_mutex_destroy(mutex);
	SIMIX_cond_destroy(cond);
	// FIXME: check for success/failure?
	return;
}

void smpi_barrier(smpi_mpi_communicator_t *comm) {
	int i;
	SIMIX_mutex_lock(comm->barrier_mutex);
	comm->barrier++;
	if(i < comm->size) {
		SIMIX_cond_wait(comm->barrier_cond, comm->barrier_mutex);
	} else {
		comm->barrier = 0;
		SIMIX_cond_broadcast(comm->barrier_cond);
	}
	SIMIX_mutex_unlock(comm->barrier_mutex);
}

int smpi_comm_rank(smpi_mpi_communicator_t *comm, smx_host_t host)
{
	int i;
	for(i = 0; i < comm->size && host != comm->hosts[i]; i++);
	if (i >= comm->size) i = -1;
	return i;
}

// FIXME: move into own file
int smpi_gettimeofday(struct timeval *tv, struct timezone *tz)
{
	double now;
	int retval = 0;
	smpi_bench_end();
	if (NULL == tv) {
		retval = -1;
	} else {
		now = SIMIX_get_clock();
		tv->tv_sec  = now;
		tv->tv_usec = ((now - (double)tv->tv_sec) * 1000000.0);
	}
	smpi_bench_begin();
	return retval;
}

unsigned int smpi_sleep(unsigned int seconds)
{
	smx_mutex_t mutex;
	smx_cond_t cond;
	smx_host_t host;
	smx_action_t sleep_action;

	smpi_bench_end();
	host         = SIMIX_host_self();
	sleep_action = SIMIX_action_sleep(host, seconds);
	mutex        = SIMIX_mutex_init();
	cond         = SIMIX_cond_init();
	SIMIX_mutex_lock(mutex);
	SIMIX_register_condition_to_action(sleep_action, cond);
	SIMIX_register_action_to_condition(sleep_action, cond);
	SIMIX_cond_wait(cond, mutex);
	SIMIX_mutex_unlock(mutex);
	SIMIX_mutex_destroy(mutex);
	SIMIX_cond_destroy(cond);
	// FIXME: check for success/failure?
	smpi_bench_begin();
	return 0;
}

void smpi_exit(int status)
{
	smpi_bench_end();
	SIMIX_mutex_lock(smpi_running_hosts_mutex);
	smpi_running_hosts--;
	SIMIX_mutex_unlock(smpi_running_hosts_mutex);
	SIMIX_process_kill(SIMIX_process_self());
	return;
}
