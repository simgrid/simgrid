#include "private.h"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(smpi_util, smpi,
                                "Logging specific to SMPI (utilities)");

/*
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
  smx_action_t action;

  smpi_bench_end();
  host = SIMIX_host_self();
  action = SIMIX_action_sleep(host, (double)seconds);
  smpi_process_wait_action(action);
  SIMIX_action_destroy(action);
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
*/
