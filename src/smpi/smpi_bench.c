#include "private.h"

void smpi_execute(double duration) {
        smx_host_t host = SIMIX_host_self();
        smx_action_t action;

	SIMIX_mutex_lock(smpi_global->execute_mutex);

	action = SIMIX_action_execute(host, "computation", duration * SMPI_DEFAULT_SPEED);

        SIMIX_register_action_to_condition(action, smpi_global->execute_cond);
        SIMIX_cond_wait(smpi_global->execute_cond, smpi_global->execute_mutex);
        SIMIX_unregister_action_to_condition(action, smpi_global->execute_cond);
        SIMIX_action_destroy(action);

        SIMIX_mutex_unlock(smpi_global->execute_mutex);

	return;
}

void smpi_bench_begin()
{
	SIMIX_mutex_lock(smpi_global->timer_mutex);
	xbt_os_timer_start(smpi_global->timer);
	return;
}

double smpi_bench_end()
{
	double duration;

	xbt_os_timer_stop(smpi_global->timer);

	duration = xbt_os_timer_elapsed(smpi_global->timer);

	SIMIX_mutex_unlock(smpi_global->timer_mutex);

	smpi_execute(duration);

	return duration;
}


void smpi_bench_skip() {
	double duration = smpi_global->times[0];
	smpi_execute(duration);
	return;
}

void smpi_do_once_1() {
	smpi_bench_end();
	SIMIX_mutex_lock(smpi_global->times_mutex);
	if (0 < smpi_global->times[0]) {
		smpi_bench_skip();
	}
	return;
}

int smpi_do_once_2() {
	int retval = 1;
	if (0 < smpi_global->times[0]) {
		SIMIX_mutex_unlock(smpi_global->times_mutex);
		retval = 0;
	}
	smpi_bench_begin();
	return retval;
}

void smpi_do_once_3() {
	smpi_global->times[0] = smpi_bench_end();
	return;
}
