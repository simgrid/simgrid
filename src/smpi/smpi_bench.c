#include "private.h"

// FIXME: could cause trouble with multithreaded procs on same host...
void smpi_bench_begin()
{
	int rank = smpi_mpi_comm_rank_self(smpi_mpi_global->mpi_comm_world);
	SIMIX_mutex_lock(smpi_global->timers_mutexes[rank]);
	xbt_os_timer_start(smpi_global->timers[rank]);
	return;
}

void smpi_bench_end()
{
	int rank = smpi_mpi_comm_rank_self(smpi_mpi_global->mpi_comm_world);
	double duration;
	smx_host_t host;
	smx_action_t compute_action;
	smx_mutex_t mutex;
	smx_cond_t cond;

	xbt_os_timer_stop(smpi_global->timers[rank]);

	duration       = xbt_os_timer_elapsed(smpi_global->timers[rank]);
	SIMIX_mutex_unlock(smpi_global->timers_mutexes[rank]);

	host           = smpi_mpi_global->mpi_comm_world->simdata->hosts[rank];
	compute_action = SIMIX_action_execute(host, NULL, duration * SMPI_DEFAULT_SPEED);
	mutex          = SIMIX_mutex_init();
	cond           = SIMIX_cond_init();

	SIMIX_mutex_lock(mutex);
	SIMIX_register_action_to_condition(compute_action, cond);
	SIMIX_cond_wait(cond, mutex);
	SIMIX_unregister_action_to_condition(compute_action, cond);
	SIMIX_mutex_unlock(mutex);

	SIMIX_mutex_destroy(mutex);
	SIMIX_cond_destroy(cond);

	// FIXME: check for success/failure?

	return;
}
