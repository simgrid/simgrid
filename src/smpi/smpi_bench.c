#include "private.h"

// FIXME: could cause trouble with multithreaded procs on same host...
// FIXME: add benchmarking flag?

void smpi_bench_begin()
{
	int index = smpi_host_index();

	SIMIX_mutex_lock(smpi_global->timers_mutexes[index]);

	xbt_os_timer_start(smpi_global->timers[index]);

	return;
}

void smpi_bench_end()
{
	int index = smpi_host_index();
	double duration;
	smx_host_t host;
	char computation[] = "computation";
	smx_action_t action;
	smx_mutex_t mutex;
	smx_cond_t cond;

	xbt_os_timer_stop(smpi_global->timers[index]);

	duration = xbt_os_timer_elapsed(smpi_global->timers[index]);

	SIMIX_mutex_unlock(smpi_global->timers_mutexes[index]);

	host   = smpi_global->hosts[index];
	action = SIMIX_action_execute(host, computation, duration * SMPI_DEFAULT_SPEED);
	mutex  = SIMIX_mutex_init();
	cond   = SIMIX_cond_init();

	SIMIX_mutex_lock(mutex);
	SIMIX_register_action_to_condition(action, cond);
	SIMIX_cond_wait(cond, mutex);
	SIMIX_unregister_action_to_condition(action, cond);
	SIMIX_mutex_unlock(mutex);

	SIMIX_mutex_destroy(mutex);
	SIMIX_cond_destroy(cond);
	//SIMIX_action_destroy(action);

	return;
}
