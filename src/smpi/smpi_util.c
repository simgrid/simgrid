#include "private.h"

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
	smx_host_t host;
	smx_action_t action;

	smpi_bench_end();

	host = SIMIX_host_self();

	SIMIX_mutex_lock(smpi_global->timer_mutex);

	action = SIMIX_action_sleep(host, seconds);

	SIMIX_register_action_to_condition(action, smpi_global->timer_cond);
	SIMIX_cond_wait(smpi_global->timer_cond, smpi_global->timer_mutex);
	SIMIX_unregister_action_to_condition(action, smpi_global->timer_cond);
	SIMIX_action_destroy(action);

	SIMIX_mutex_unlock(smpi_global->timer_mutex);

	smpi_bench_begin();
	return 0;
}

void smpi_exit(int status)
{
	smpi_bench_end();
	SIMIX_mutex_lock(smpi_global->running_hosts_count_mutex);
	smpi_global->running_hosts_count--;
	SIMIX_mutex_unlock(smpi_global->running_hosts_count_mutex);
	SIMIX_process_kill(SIMIX_process_self());
	return;
}
