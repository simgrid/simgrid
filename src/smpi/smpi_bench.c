#include "private.h"
#include <string.h>

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(smpi_bench, smpi,
                                "Logging specific to SMPI (benchmarking)");

void smpi_execute(double duration)
{
  smx_host_t host;
  smx_action_t action;
  smx_mutex_t mutex;
  smx_cond_t cond;
  e_surf_action_state_t state;


  if(duration > 0.001) {
    host = SIMIX_host_self();
    mutex = SIMIX_mutex_init();
    cond = SIMIX_cond_init();
    DEBUG1("Sleep for %f to handle real computation time", duration);
    duration *= xbt_cfg_get_double(_surf_cfg_set, "reference_speed");
    action = SIMIX_action_sleep(host, duration);
    SIMIX_mutex_lock(mutex);
    SIMIX_register_action_to_condition(action, cond);
    for (state = SIMIX_action_get_state(action);
         state == SURF_ACTION_READY ||
         state == SURF_ACTION_RUNNING; state = SIMIX_action_get_state(action)) {
      SIMIX_cond_wait(cond, mutex);
    }
    SIMIX_unregister_action_to_condition(action, cond);
    SIMIX_mutex_unlock(mutex);
    SIMIX_action_destroy(action);
    SIMIX_cond_destroy(cond);
    SIMIX_mutex_destroy(mutex);
  }
}

void smpi_bench_begin()
{
  xbt_os_timer_start(smpi_process_timer());
}

void smpi_bench_end()
{
  xbt_os_timer_t timer = smpi_process_timer();

  xbt_os_timer_stop(timer);
  smpi_execute(xbt_os_timer_elapsed(timer));
}

/*
TODO
void smpi_do_once_1(const char *file, int line)
{
  smpi_do_once_duration_node_t curr, prev;

  smpi_bench_end();
  SIMIX_mutex_lock(smpi_global->do_once_mutex);
  prev = NULL;
  for(curr = smpi_global->do_once_duration_nodes;
      NULL != curr && (strcmp(curr->file, file) || curr->line != line);
      curr = curr->next) {
    prev = curr;
  }
  if(NULL == curr) {
    curr = xbt_new(s_smpi_do_once_duration_node_t, 1);
    curr->file = xbt_strdup(file);
    curr->line = line;
    curr->duration = -1;
    curr->next = NULL;
    if(NULL == prev) {
      smpi_global->do_once_duration_nodes = curr;
    } else {
      prev->next = curr;
    }
  }
  smpi_global->do_once_duration = &curr->duration;
}

int smpi_do_once_2()
{
  double duration = *(smpi_global->do_once_duration);

  if(0 > duration) {
    smpi_start_timer();
    return 1;
  }
  SIMIX_mutex_unlock(smpi_global->do_once_mutex);
  smpi_execute(duration);
  smpi_bench_begin();
  return 0;
}

void smpi_do_once_3()
{
  *(smpi_global->do_once_duration) = smpi_stop_timer();
}
*/
