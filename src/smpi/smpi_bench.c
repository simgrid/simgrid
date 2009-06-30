#include "private.h"
#include <string.h>

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(smpi_bench, smpi,
                                "Logging specific to SMPI (benchmarking)");

void smpi_execute(double duration)
{
  smx_host_t host = SIMIX_host_self();
  smx_mutex_t mutex = smpi_process_mutex();
  smx_cond_t cond = smpi_process_cond();
  smx_action_t action;
  e_surf_action_state_t state;

  SIMIX_mutex_lock(mutex);

  action =
    SIMIX_action_execute(host, "execute",
                         duration * smpi_global->reference_speed);

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

  return;
}

void smpi_start_timer()
{
  SIMIX_mutex_lock(smpi_global->timer_mutex);
  xbt_os_timer_start(smpi_global->timer);
}

double smpi_stop_timer()
{
  double duration;
  xbt_os_timer_stop(smpi_global->timer);
  duration = xbt_os_timer_elapsed(smpi_global->timer);
  SIMIX_mutex_unlock(smpi_global->timer_mutex);
  return duration;
}

void smpi_bench_begin()
{
  smpi_start_timer();
}

void smpi_bench_end()
{
  smpi_execute(smpi_stop_timer());
}

void smpi_do_once_1(const char *file, int line)
{
  smpi_do_once_duration_node_t curr, prev;
  smpi_bench_end();
  SIMIX_mutex_lock(smpi_global->do_once_mutex);
  prev = NULL;
  for (curr = smpi_global->do_once_duration_nodes;
       NULL != curr && (strcmp(curr->file, file) || curr->line != line);
       curr = curr->next) {
    prev = curr;
  }
  if (NULL == curr) {
    curr = xbt_new(s_smpi_do_once_duration_node_t, 1);
    curr->file = xbt_strdup(file);
    curr->line = line;
    curr->duration = -1;
    curr->next = NULL;
    if (NULL == prev) {
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
  if (0 > duration) {
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
