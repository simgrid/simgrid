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
void MC_init(int method)
{
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

  switch (method) {
  case 0:
    MC_dfs_init();
    break;
  case 1:
    MC_dpor_init();
    break;
  default:
    break;
  }

  /* Save the initial state */
  MC_SET_RAW_MEM;
  initial_snapshot = xbt_new(s_mc_snapshot_t, 1);
  MC_take_snapshot(initial_snapshot);
  MC_UNSET_RAW_MEM;
}

void MC_modelcheck(int method)
{

  MC_init(method);

  switch (method) {
  case 0:
    MC_dfs();
    break;
  case 1:
    MC_dpor();
    break;
  default:
    break;
  }

  MC_exit(method);
}

void MC_exit(int method)
{
  mc_state_t state;

  switch (method) {
  case 0:
    //MC_dfs_exit();
    break;
  case 1:
    //MC_dpor_exit();
    break;
  default:
    break;
  }

  /* Destroy MC data structures (in RAW memory) */
  MC_SET_RAW_MEM;
  xbt_free(mc_stats);

  while ((state = (mc_state_t) xbt_fifo_pop(mc_stack)) != NULL)
    MC_state_delete(state);

  xbt_fifo_free(mc_stack);
  xbt_setset_destroy(mc_setset);
  MC_UNSET_RAW_MEM;
}

int MC_random(int min, int max)
{
  MC_trans_intercept_random(min, max);
  return mc_current_state->executed_transition->random.value;
}

/**
 * \brief Re-executes from the initial state all the transitions indicated by
 *        a given model-checker stack.
 * \param stack The stack with the transitions to execute.
*/
void MC_replay(xbt_fifo_t stack)
{
  xbt_fifo_item_t item;
  mc_transition_t trans;

  DEBUG0("**** Begin Replay ****");

  /* Restore the initial state */
  MC_restore_snapshot(initial_snapshot);

  mc_replay_mode = TRUE;

  MC_UNSET_RAW_MEM;

  /* Traverse the stack from the initial state and re-execute the transitions */
  for (item = xbt_fifo_get_last_item(stack);
       item != xbt_fifo_get_first_item(stack);
       item = xbt_fifo_get_prev_item(item)) {

    mc_current_state = (mc_state_t) xbt_fifo_get_item_content(item);
    trans = mc_current_state->executed_transition;

    /* Update statistics */
    mc_stats->visited_states++;
    mc_stats->executed_transitions++;

    DEBUG1("Executing transition %s", trans->name);
    SIMIX_process_schedule(trans->process);

    /* Do all surf's related black magic tricks to keep all working */
    MC_execute_surf_actions();

    /* Schedule every process that got enabled due to the executed transition */
    MC_schedule_enabled_processes();
  }
  mc_replay_mode = FALSE;
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
  mc_transition_t trans;
  xbt_fifo_item_t item;

  for (item = xbt_fifo_get_last_item(stack);
       (item ? (state = (mc_state_t) (xbt_fifo_get_item_content(item)))
        : (NULL)); item = xbt_fifo_get_prev_item(item)) {
    trans = state->executed_transition;
    if (trans) {
      INFO1("%s", trans->name);
    }
  }
}

/**
 * \brief Schedules all the process that are ready to run
 *        As a side effect it performs some clean-up required by SIMIX 
 */
void MC_schedule_enabled_processes(void)
{
  smx_process_t process;

  //SIMIX_process_empty_trash();

  /* Schedule every process that is ready to run due to an finished action */
  while ((process = xbt_swag_extract(simix_global->process_to_run))) {
    DEBUG2("Scheduling %s on %s", process->name, process->smx_host->name);
    SIMIX_process_schedule(process);
  }
}

/******************************** States **************************************/

/**
 * \brief Creates a state data structure used by the exploration algorithm
 */
mc_state_t MC_state_new(void)
{
  mc_state_t state = NULL;

  state = xbt_new0(s_mc_state_t, 1);
  state->created_transitions = xbt_setset_new_set(mc_setset);
  state->transitions = xbt_setset_new_set(mc_setset);
  state->enabled_transitions = xbt_setset_new_set(mc_setset);
  state->interleave = xbt_setset_new_set(mc_setset);
  state->done = xbt_setset_new_set(mc_setset);
  state->executed_transition = NULL;

  mc_stats->expanded_states++;

  return state;
}

/**
 * \brief Deletes a state data structure
 * \param trans The state to be deleted
 */
void MC_state_delete(mc_state_t state)
{
  xbt_setset_cursor_t cursor;
  mc_transition_t trans;

  xbt_setset_foreach(state->created_transitions, cursor, trans) {
    xbt_setset_elm_remove(mc_setset, trans);
    MC_transition_delete(trans);
  }

  xbt_setset_destroy_set(state->created_transitions);
  xbt_setset_destroy_set(state->transitions);
  xbt_setset_destroy_set(state->enabled_transitions);
  xbt_setset_destroy_set(state->interleave);
  xbt_setset_destroy_set(state->done);

  xbt_free(state);
}

/************************** SURF Emulation ************************************/

/* Dirty hack, we manipulate surf's clock to simplify the integration of the
   model-checker */
extern double NOW;

/**
 * \brief Executes all the actions at every model
 */
void MC_execute_surf_actions(void)
{
  unsigned int iter;
  surf_action_t action = NULL;
  surf_model_t model = NULL;
  smx_action_t smx_action = NULL;

  /* Execute all the actions in every model */
  xbt_dynar_foreach(model_list, iter, model) {
    while ((action = xbt_swag_extract(model->states.running_action_set))) {
      /* FIXME: timeouts are not calculated correctly */
      if (NOW >= action->max_duration) {
        surf_action_state_set(action, SURF_ACTION_DONE);
        smx_action = action->data;
        DEBUG5
            ("Resource [%s] (%d): Executing RUNNING action \"%s\" (%p) MaxDuration %lf",
             model->name, xbt_swag_size(model->states.running_action_set),
             smx_action->name, smx_action, action->max_duration);

        if (smx_action)
          SIMIX_action_signal_all(smx_action);
      }
    }
    /*FIXME: check if this is always empty or not */
    while ((action = xbt_swag_extract(model->states.failed_action_set))) {
      smx_action = action->data;
      DEBUG4("Resource [%s] (%d): Executing FAILED action \"%s\" (%p)",
             model->name, xbt_swag_size(model->states.running_action_set),
             smx_action->name, smx_action);
      if (smx_action)
        SIMIX_action_signal_all(smx_action);
    }
  }
  /* That's it, now go one step deeper into the model-checking process! */
  NOW += 0.5;                   /* FIXME: Check time increases */
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
  if (!prop) {
    INFO0("**************************");
    INFO0("*** PROPERTY NOT VALID ***");
    INFO0("**************************");
    INFO0("Counter-example execution trace:");
    MC_dump_stack(mc_stack);
    MC_print_statistics(mc_stats);
    xbt_abort();
  }
}
