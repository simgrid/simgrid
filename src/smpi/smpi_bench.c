#include "private.h"

// FIXME: could cause trouble with multithreaded procs on same host...
// FIXME: add benchmarking flag?

void smpi_bench_begin()
{
	SIMIX_mutex_lock(smpi_global->timer_mutex);
	xbt_os_timer_start(smpi_global->timer);
	return;
}

void smpi_bench_end()
{
	double duration;
	smx_host_t host;
	char computation[] = "computation";
	smx_action_t action;

	xbt_os_timer_stop(smpi_global->timer);
	duration = xbt_os_timer_elapsed(smpi_global->timer);
	host     = SIMIX_host_self();
	action   = SIMIX_action_execute(host, computation, duration * SMPI_DEFAULT_SPEED);

	SIMIX_register_action_to_condition(action, smpi_global->timer_cond);
	SIMIX_cond_wait(smpi_global->timer_cond, smpi_global->timer_mutex);
	SIMIX_unregister_action_to_condition(action, smpi_global->timer_cond);

	SIMIX_mutex_unlock(smpi_global->timer_mutex);

	return;
}
