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
xbt_setset_t mc_setset = NULL;
mc_stats_t mc_stats = NULL;
mc_state_t mc_current_state = NULL;
char mc_replay_mode = FALSE;

/**
 *  \brief Initialize the model-checker data structures
 */
void MC_init(void)
{
  smx_process_t process;
  
  /* Check if MC is already initialized */
  if (initial_snapshot)
    return;

  /* Initialize the data structures that must be persistent across every
     iteration of the model-checker (in RAW memory) */
  MC_SET_RAW_MEM;

  /* Initialize statistics */
  mc_stats = xbt_new0(s_mc_stats_t, 1);
  mc_stats->state_size = 1;

  /* Create exploration stack */
  mc_stack = xbt_fifo_new();

  /* Create the container for the sets */
  mc_setset = xbt_setset_new(20);

  /* Add the existing processes to mc's setset so they have an ID designated */
  /* FIXME: check what happen if there are processes created during the exploration */
  xbt_swag_foreach(process, simix_global->process_list){
    xbt_setset_elm_add(mc_setset, process);
  }
  
  MC_dpor_init();

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
  MC_memory_exit();
}

int MC_random(int min, int max)
{
  /*FIXME: return mc_current_state->executed_transition->random.value;*/
  return 0;
}

/**
 * \brief Re-executes from the initial state all the transitions indicated by
 *        a given model-checker stack.
 * \param stack The stack with the transitions to execute.
*/
void MC_replay(xbt_fifo_t stack)
{
  char *req_str;
  smx_req_t req = NULL, saved_req = NULL;
  xbt_fifo_item_t item;
  mc_state_t state;

  DEBUG0("**** Begin Replay ****");

  /* Restore the initial state */
  MC_restore_snapshot(initial_snapshot);

  /* Traverse the stack from the initial state and re-execute the transitions */
  for (item = xbt_fifo_get_last_item(stack);
       item != xbt_fifo_get_first_item(stack);
       item = xbt_fifo_get_prev_item(item)) {

    state = (mc_state_t) xbt_fifo_get_item_content(item);
    saved_req = MC_state_get_executed_request(state);
   
    if(saved_req){
      /* because we got a copy of the executed request, we have to fetch the  
         real one, pointed by the request field of the issuer process */
      req = &saved_req->issuer->request;

      /* Debug information */
      if(XBT_LOG_ISENABLED(mc_global, xbt_log_priority_debug)){
        req_str = MC_request_to_string(req); 
        DEBUG1("Replay: %s", req_str);
        xbt_free(req_str);
      }
    }
         
    SIMIX_request_pre(req);
    MC_wait_for_requests();
         
    /* Update statistics */
    mc_stats->visited_states++;
    mc_stats->executed_transitions++;
  }
  DEBUG0("**** End Replay ****");
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
  mc_state_t state;
  xbt_fifo_item_t item;
  smx_req_t req;
  char *req_str = NULL;
  
  for (item = xbt_fifo_get_last_item(stack);
       (item ? (state = (mc_state_t) (xbt_fifo_get_item_content(item)))
        : (NULL)); item = xbt_fifo_get_prev_item(item)) {
    req = MC_state_get_executed_request(state);
    if(req){
      req_str = MC_request_to_string(req); 
      INFO1("%s", req_str);
      xbt_free(req_str);
    }
  }
}

/**
 * \brief Schedules all the process that are ready to run
 */
void MC_wait_for_requests(void)
{
  char *req_str = NULL;
  smx_req_t req = NULL;

  do {
    SIMIX_context_runall(simix_global->process_to_run);
    while((req = SIMIX_request_pop())){
      if(!SIMIX_request_is_visible(req))
        SIMIX_request_pre(req);
      else if(req->call == REQ_COMM_WAITANY)
        THROW_UNIMPLEMENTED;
      else if(XBT_LOG_ISENABLED(mc_global, xbt_log_priority_debug)){
        req_str = MC_request_to_string(req);
        DEBUG1("Got: %s", req_str);
        xbt_free(req_str);
      }
    }
  } while (xbt_swag_size(simix_global->process_to_run));
}

/****************************** Statistics ************************************/
void MC_print_statistics(mc_stats_t stats)
{
  INFO1("State space size ~= %lu", stats->state_size);
  INFO1("Expanded states = %lu", stats->expanded_states);
  INFO1("Visited states = %lu", stats->visited_states);
  INFO1("Executed transitions = %lu", stats->executed_transitions);
  INFO1("Expanded / Visited = %lf",
        (double) stats->visited_states / stats->expanded_states);
  /*INFO1("Exploration coverage = %lf", 
     (double)stats->expanded_states / stats->state_size); */
}

/************************* Assertion Checking *********************************/
void MC_assert(int prop)
{
  if (MC_IS_ENABLED && !prop) {
    INFO0("**************************");
    INFO0("*** PROPERTY NOT VALID ***");
    INFO0("**************************");
    INFO0("Counter-example execution trace:");
    MC_dump_stack(mc_stack);
    MC_print_statistics(mc_stats);
    xbt_abort();
  }
}

void MC_show_deadlock(smx_req_t req)
{
  char *req_str = NULL;
  INFO0("**************************");
  INFO0("*** DEAD-LOCK DETECTED ***");
  INFO0("**************************");
  INFO0("Locked request:");
  req_str = MC_request_to_string(req);
  INFO1("%s", req_str);
  xbt_free(req_str);
  INFO0("Counter-example execution trace:");
  MC_dump_stack(mc_stack);
}
