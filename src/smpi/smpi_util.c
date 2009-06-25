#include "private.h"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(smpi_util, smpi,
                                "Logging specific to SMPI (utilities)");

int smpi_gettimeofday(struct timeval *tv, struct timezone *tz)
{
  double now;
  int retval;
  smpi_bench_end();
  retval = 0;
  if (NULL == tv) {
    retval = -1;
  } else {
    now = SIMIX_get_clock();
    tv->tv_sec = now;
    tv->tv_usec = ((now - (double) tv->tv_sec) * 1000000.0);
  }
  smpi_bench_begin();
  return retval;
}

unsigned int smpi_sleep(unsigned int seconds)
{
  smx_host_t host;
  smx_mutex_t mutex;
  smx_cond_t cond;
  smx_action_t action;
  e_surf_action_state_t state;

  smpi_bench_end();

  host = SIMIX_host_self();
  mutex = smpi_process_mutex();
  cond = smpi_process_cond();

  SIMIX_mutex_lock(mutex);

  // FIXME: explicit conversion to double?
  action = SIMIX_action_sleep(host, seconds);

  SIMIX_register_action_to_condition(action, cond);
  for (state = SIMIX_action_get_state(action);
       state == SURF_ACTION_READY ||
       state == SURF_ACTION_RUNNING; state = SIMIX_action_get_state(action)
    ) {
    SIMIX_cond_wait(cond, mutex);
  }
  SIMIX_unregister_action_to_condition(action, cond);
  SIMIX_action_destroy(action);

  SIMIX_mutex_unlock(mutex);

  smpi_bench_begin();
  return 0;
}

void smpi_exit(int status)
{
  smpi_bench_end();
  smpi_process_finalize();
  SIMIX_process_kill(SIMIX_process_self());
  return;
}
