#include "smx_private.h"
#include "xbt/fifo.h"
#include "xbt/xbt_os_thread.h"
#include "../mc/mc_private.h"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(simix_smurf, simix,
                                "Logging specific to SIMIX (SMURF)");

XBT_INLINE smx_simcall_t SIMIX_simcall_mine()
{
  smx_process_t issuer = SIMIX_process_self();
  return &issuer->simcall;
}

/**
 * \brief Makes the current process do a simcall to the kernel and yields
 * until completion. If the current thread is maestro, we don't yield and
 * execute the simcall directly.
 * \param self the current process
 */
void SIMIX_simcall_push(smx_process_t self)
{
  if (self != simix_global->maestro_process) {
    XBT_DEBUG("Yield process '%s' on simcall %s (%d)", self->name,
              SIMIX_simcall_name(self->simcall.call), (int)self->simcall.call);
    SIMIX_process_yield(self);
  } else {
    XBT_DEBUG("I'm the maestro, execute the simcall directly");
    SIMIX_simcall_pre(&self->simcall, 0);
  }
}

void SIMIX_simcall_answer(smx_simcall_t simcall)
{
  if (simcall->issuer != simix_global->maestro_process){
    XBT_DEBUG("Answer simcall %s (%d) issued by %s (%p)", SIMIX_simcall_name(simcall->call), (int)simcall->call,
        simcall->issuer->name, simcall->issuer);
    simcall->issuer->simcall.call = SIMCALL_NONE;
/*    This check should be useless and slows everyone. Reactivate if you see something
 *    weird in process scheduling.
 */
/*    if(!xbt_dynar_member(simix_global->process_to_run, &(simcall->issuer))) */
    xbt_dynar_push_as(simix_global->process_to_run, smx_process_t, simcall->issuer);
/*    else DIE_IMPOSSIBLE; */
  }
}

void SIMIX_simcall_pre(smx_simcall_t simcall, int value)
{
  XBT_DEBUG("Handling simcall %p: %s", simcall, SIMIX_simcall_name(simcall->call));
  simcall->mc_value = value;
  if (simcall->issuer->context->iwannadie && simcall->call != SIMCALL_PROCESS_CLEANUP)
    return;
  switch (simcall->call) {
SIMCALL_LIST(SIMCALL_CASE, SIMCALL_SEP_NOTHING)
    case NUM_SIMCALLS:;
      break;
    case SIMCALL_NONE:;
      THROWF(arg_error,0,"Asked to do the noop syscall on %s@%s",
          SIMIX_process_get_name(simcall->issuer),
          SIMIX_host_get_name(SIMIX_process_get_host(simcall->issuer))
          );
      break;

    /* ****************************************************************************************** */
    /* TUTORIAL: New API                                                                        */
    /* ****************************************************************************************** */
    case SIMCALL_NEW_API_INIT:
      SIMIX_pre_new_api_fct(simcall);
      break;
  }
}

void SIMIX_simcall_post(smx_action_t action)
{
  switch (action->type) {

    case SIMIX_ACTION_EXECUTE:
    case SIMIX_ACTION_PARALLEL_EXECUTE:
      SIMIX_post_host_execute(action);
      break;

    case SIMIX_ACTION_COMMUNICATE:
      SIMIX_post_comm(action);
      break;

    case SIMIX_ACTION_SLEEP:
      SIMIX_post_process_sleep(action);
      break;

    case SIMIX_ACTION_SYNCHRO:
      SIMIX_post_synchro(action);
      break;

    case SIMIX_ACTION_IO:
      SIMIX_post_io(action);
      break;

    /* ****************************************************************************************** */
    /* TUTORIAL: New API                                                                        */
    /* ****************************************************************************************** */
    case SIMIX_ACTION_NEW_API:
      SIMIX_post_new_api(action);
      break;
  }
}

/* New Simcal interface */

/* FIXME: add types for every simcall */
//const char *simcall_types[NUM_SIMCALLS] = { [SIMCALL_HOST_EXECUTE] = "%s%p%f%f%p" };
/* FIXME find a way to make this work
simcall_handler_t simcall_table[NUM_SIMCALLS] = {
#undef SIMCALL_ENUM_ELEMENT
#define SIMCALL_ENUM_ELEMENT(x,y) &y // generate strings from the enumeration values 
SIMCALL_LIST
#undef SIMCALL_ENUM_ELEMENT
};*/

/* New Simcal interface */

/* FIXME: add types for every simcall */
//const char *simcall_types[NUM_SIMCALLS] = { [SIMCALL_HOST_EXECUTE] = "%s%p%f%f%p", [SIMCALL_HOST_EXECUTION_WAIT] = "%p%p" };


/*TOFIX find a way to make this work
simcall_handler_t simcall_table[NUM_SIMCALLS] = {
#undef SIMCALL_ENUM_ELEMENT
#define SIMCALL_ENUM_ELEMENT(x,y) &y // generate strings from the enumeration values
SIMCALL_LIST
#undef SIMCALL_ENUM_ELEMENT
};*/

