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
	smx_mutex_t mutex;
	smx_cond_t cond;
	smx_host_t host;
	smx_action_t action;

	smpi_bench_end();
	host   = SIMIX_host_self();
	action = SIMIX_action_sleep(host, seconds);
	mutex  = SIMIX_mutex_init();
	cond   = SIMIX_cond_init();

	SIMIX_mutex_lock(mutex);
	SIMIX_register_action_to_condition(action, cond);
	SIMIX_cond_wait(cond, mutex);
	SIMIX_unregister_action_to_condition(action, cond);
	SIMIX_mutex_unlock(mutex);

	SIMIX_mutex_destroy(mutex);
	SIMIX_cond_destroy(cond);
	SIMIX_action_destroy(action);

	// FIXME: check for success/failure?

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
