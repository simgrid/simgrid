#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "../surf/surf_private.h"
#include "../simix/private.h"
#include "xbt/fifo.h"
#include "private.h"


XBT_LOG_NEW_DEFAULT_SUBCATEGORY(mc_global, mc,
                                "Logging specific to MC (global)");

/* MC global data structures */
mc_snapshot_t initial_snapshot = NULL;
xbt_fifo_t mc_stack = NULL;
mc_stats_t mc_stats = NULL;
mc_state_t mc_current_state = NULL;
char mc_replay_mode = FALSE;
double *mc_time = NULL;
/**
 *  \brief Initialize the model-checker data structures
 */
void MC_init(void)
{
  /* Check if MC is already initialized */
  if (initial_snapshot)
    return;

  mc_time = xbt_new0(double, simix_process_maxpid);

  /* Initialize the data structures that must be persistent across every
     iteration of the model-checker (in RAW memory) */
  MC_SET_RAW_MEM;

  /* Initialize statistics */
  mc_stats = xbt_new0(s_mc_stats_t, 1);
  mc_stats->state_size = 1;

  /* Create exploration stack */
  mc_stack = xbt_fifo_new();

  MC_UNSET_RAW_MEM;

  MC_dpor_init();

  MC_SET_RAW_MEM;
  /* Save the initial state */
  initial_snapshot = xbt_new0(s_mc_snapshot_t, 1);
  MC_take_snapshot(initial_snapshot);
  MC_UNSET_RAW_MEM;
}

void MC_modelcheck(void)
{
  MC_init();
  MC_dpor();
  MC_exit();
}

void MC_exit(void)
{
  MC_print_statistics(mc_stats);
  xbt_free(mc_time);
  MC_memory_exit();
}

int MC_random(int min, int max)
{
  /*FIXME: return mc_current_state->executed_transition->random.value;*/
  return 0;
}

/**
 * \brief Schedules all the process that are ready to run
 */
void MC_wait_for_requests(void)
{
  smx_process_t process;
  smx_req_t req;
  unsigned int iter;

  while (!xbt_dynar_is_empty(simix_global->process_to_run)) {
    SIMIX_process_runall();
    xbt_dynar_foreach(simix_global->process_that_ran, iter, process) {
      req = &process->request;
      if (req->call != REQ_NO_REQ && !MC_request_is_visible(req))
          SIMIX_request_pre(req, 0);
    }
  }
}

int MC_deadlock_check()
{
  int deadlock = FALSE;
  smx_process_t process;
  if(xbt_swag_size(simix_global->process_list)){
    deadlock = TRUE;
    xbt_swag_foreach(process, simix_global->process_list){
      if(process->request.call != REQ_NO_REQ
         && MC_request_is_enabled(&process->request)){
        deadlock = FALSE;
        break;
      }
    }
  }
  return deadlock;
}

/**
 * \brief Re-executes from the initial state all the transitions indicated by
 *        a given model-checker stack.
 * \param stack The stack with the transitions to execute.
*/
void MC_replay(xbt_fifo_t stack)
{
  int value;
  char *req_str;
  smx_req_t req = NULL, saved_req = NULL;
  xbt_fifo_item_t item;
  mc_state_t state;

  XBT_DEBUG("**** Begin Replay ****");

  /* Restore the initial state */
  MC_restore_snapshot(initial_snapshot);
  /* At the moment of taking the snapshot the raw heap was set, so restoring
   * it will set it back again, we have to unset it to continue  */
  MC_UNSET_RAW_MEM;

  /* Traverse the stack from the initial state and re-execute the transitions */
  for (item = xbt_fifo_get_last_item(stack);
       item != xbt_fifo_get_first_item(stack);
       item = xbt_fifo_get_prev_item(item)) {

    state = (mc_state_t) xbt_fifo_get_item_content(item);
    saved_req = MC_state_get_executed_request(state, &value);
   
    if(saved_req){
      /* because we got a copy of the executed request, we have to fetch the  
         real one, pointed by the request field of the issuer process */
      req = &saved_req->issuer->request;

      /* Debug information */
      if(XBT_LOG_ISENABLED(mc_global, xbt_log_priority_debug)){
        req_str = MC_request_to_string(req, value);
        XBT_DEBUG("Replay: %s (%p)", req_str, state);
        xbt_free(req_str);
      }
    }
         
    SIMIX_request_pre(req, value);
    MC_wait_for_requests();
         
    /* Update statistics */
    mc_stats->visited_states++;
    mc_stats->executed_transitions++;
  }
  XBT_DEBUG("**** End Replay ****");
}

/**
 * \brief Dumps the contents of a model-checker's stack and shows the actual
 *        execution trace
 * \param stack The stack to dump
*/
void MC_dump_stack(xbt_fifo_t stack)
{
  mc_state_t state;

  MC_show_stack(stack);

  MC_SET_RAW_MEM;
  while ((state = (mc_state_t) xbt_fifo_pop(stack)) != NULL)
    MC_state_delete(state);
  MC_UNSET_RAW_MEM;
}

void MC_show_stack(xbt_fifo_t stack)
{
  int value;
  mc_state_t state;
  xbt_fifo_item_t item;
  smx_req_t req;
  char *req_str = NULL;
  
  for (item = xbt_fifo_get_last_item(stack);
       (item ? (state = (mc_state_t) (xbt_fifo_get_item_content(item)))
        : (NULL)); item = xbt_fifo_get_prev_item(item)) {
    req = MC_state_get_executed_request(state, &value);
    if(req){
      req_str = MC_request_to_string(req, value);
      XBT_INFO("%s", req_str);
      xbt_free(req_str);
    }
  }
}

void MC_show_deadlock(smx_req_t req)
{
  /*char *req_str = NULL;*/
  XBT_INFO("**************************");
  XBT_INFO("*** DEAD-LOCK DETECTED ***");
  XBT_INFO("**************************");
  XBT_INFO("Locked request:");
  /*req_str = MC_request_to_string(req);
  XBT_INFO("%s", req_str);
  xbt_free(req_str);*/
  XBT_INFO("Counter-example execution trace:");
  MC_dump_stack(mc_stack);
}

void MC_print_statistics(mc_stats_t stats)
{
  XBT_INFO("State space size ~= %lu", stats->state_size);
  XBT_INFO("Expanded states = %lu", stats->expanded_states);
  XBT_INFO("Visited states = %lu", stats->visited_states);
  XBT_INFO("Executed transitions = %lu", stats->executed_transitions);
  XBT_INFO("Expanded / Visited = %lf",
        (double) stats->visited_states / stats->expanded_states);
  /*XBT_INFO("Exploration coverage = %lf",
     (double)stats->expanded_states / stats->state_size); */
}

void MC_assert(int prop)
{
  if (MC_IS_ENABLED && !prop) {
    XBT_INFO("**************************");
    XBT_INFO("*** PROPERTY NOT VALID ***");
    XBT_INFO("**************************");
    XBT_INFO("Counter-example execution trace:");
    MC_dump_stack(mc_stack);
    MC_print_statistics(mc_stats);
    xbt_abort();
  }
}

void MC_process_clock_add(smx_process_t process, double amount)
{
  mc_time[process->pid] += amount;
}

double MC_process_clock_get(smx_process_t process)
{
  if(mc_time)
    return mc_time[process->pid];
  else
    return 0;
}
