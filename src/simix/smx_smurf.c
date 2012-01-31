#include "smx_private.h"
#include "xbt/fifo.h"
#include "xbt/xbt_os_thread.h"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(simix_smurf, simix,
                                "Logging specific to SIMIX (SMURF)");

XBT_INLINE smx_simcall_t SIMIX_simcall_mine()
{
  smx_process_t issuer = SIMIX_process_self();
  return &issuer->simcall;
}

/**
 * \brief Makes the current process do a simcall to the kernel and yields
 * until completion.
 * \param self the current process
 */
void SIMIX_simcall_push(smx_process_t self)
{
  if (self != simix_global->maestro_process) {
    XBT_DEBUG("Yield process '%s' on simcall %s (%d)", self->name,
        SIMIX_simcall_name(self->simcall.call), self->simcall.call);
    SIMIX_process_yield(self);
  } else {
    SIMIX_simcall_pre(&self->simcall, 0);
  }
}

void SIMIX_simcall_answer(smx_simcall_t simcall)
{
  if (simcall->issuer != simix_global->maestro_process){
    XBT_DEBUG("Answer simcall %s (%d) issued by %s (%p)", SIMIX_simcall_name(simcall->call), simcall->call,
        simcall->issuer->name, simcall->issuer);
    simcall->issuer->simcall.call = SIMCALL_NONE;
    xbt_dynar_push_as(simix_global->process_to_run, smx_process_t, simcall->issuer);
  }
}

void SIMIX_simcall_pre(smx_simcall_t simcall, int value)
{
  XBT_DEBUG("Handling simcall %p: %s", simcall, SIMIX_simcall_name(simcall->call));

  switch (simcall->call) {
    case SIMCALL_COMM_TEST:
      SIMIX_pre_comm_test(simcall);
      break;

    case SIMCALL_COMM_TESTANY:
      SIMIX_pre_comm_testany(simcall, value);
      break;

    case SIMCALL_COMM_WAIT:
      SIMIX_pre_comm_wait(simcall,
          simcall->comm_wait.comm,
          simcall->comm_wait.timeout,
          value);
      break;

    case SIMCALL_COMM_WAITANY:
      SIMIX_pre_comm_waitany(simcall, value);
      break;

    case SIMCALL_COMM_SEND:
    {
      smx_action_t comm = SIMIX_comm_isend(
          simcall->issuer,
          simcall->comm_send.rdv,
          simcall->comm_send.task_size,
          simcall->comm_send.rate,
          simcall->comm_send.src_buff,
          simcall->comm_send.src_buff_size,
          simcall->comm_send.match_fun,
          NULL, /* no clean function since it's not detached */
          simcall->comm_send.data,
          0);
      SIMIX_pre_comm_wait(simcall, comm, simcall->comm_send.timeout, 0);
      break;
    }

    case SIMCALL_COMM_ISEND:
      simcall->comm_isend.result = SIMIX_comm_isend(
          simcall->issuer,
          simcall->comm_isend.rdv,
          simcall->comm_isend.task_size,
          simcall->comm_isend.rate,
          simcall->comm_isend.src_buff,
          simcall->comm_isend.src_buff_size,
          simcall->comm_isend.match_fun,
          simcall->comm_isend.clean_fun,
          simcall->comm_isend.data,
          simcall->comm_isend.detached);
      SIMIX_simcall_answer(simcall);
      break;

    case SIMCALL_COMM_RECV:
    {
      smx_action_t comm = SIMIX_comm_irecv(
          simcall->issuer,
          simcall->comm_recv.rdv,
          simcall->comm_recv.dst_buff,
          simcall->comm_recv.dst_buff_size,
          simcall->comm_recv.match_fun,
          simcall->comm_recv.data);
      SIMIX_pre_comm_wait(simcall, comm, simcall->comm_recv.timeout, 0);
      break;
    }

    case SIMCALL_COMM_IRECV:
      simcall->comm_irecv.result = SIMIX_comm_irecv(
          simcall->issuer,
          simcall->comm_irecv.rdv,
          simcall->comm_irecv.dst_buff,
          simcall->comm_irecv.dst_buff_size,
          simcall->comm_irecv.match_fun,
          simcall->comm_irecv.data);
      SIMIX_simcall_answer(simcall);
      break;

    case SIMCALL_COMM_DESTROY:
      SIMIX_comm_destroy(simcall->comm_destroy.comm);
      SIMIX_simcall_answer(simcall);
      break;

    case SIMCALL_COMM_CANCEL:
      SIMIX_comm_cancel(simcall->comm_cancel.comm);
      SIMIX_simcall_answer(simcall);
      break;

    case SIMCALL_COMM_GET_REMAINS:
      simcall->comm_get_remains.result =
          SIMIX_comm_get_remains(simcall->comm_get_remains.comm);
      SIMIX_simcall_answer(simcall);
      break;

    case SIMCALL_COMM_GET_STATE:
      simcall->comm_get_state.result =
          SIMIX_comm_get_state(simcall->comm_get_state.comm);
      SIMIX_simcall_answer(simcall);
      break;

    case SIMCALL_COMM_GET_SRC_DATA:
      simcall->comm_get_src_data.result = SIMIX_comm_get_src_data(simcall->comm_get_src_data.comm);
      SIMIX_simcall_answer(simcall);
      break;

    case SIMCALL_COMM_GET_DST_DATA:
      simcall->comm_get_dst_data.result = SIMIX_comm_get_dst_data(simcall->comm_get_dst_data.comm);
      SIMIX_simcall_answer(simcall);
      break;

    case SIMCALL_COMM_GET_SRC_PROC:
      simcall->comm_get_src_proc.result =
          SIMIX_comm_get_src_proc(simcall->comm_get_src_proc.comm);
      SIMIX_simcall_answer(simcall);
      break;

    case SIMCALL_COMM_GET_DST_PROC:
      simcall->comm_get_dst_proc.result =
          SIMIX_comm_get_dst_proc(simcall->comm_get_dst_proc.comm);
      SIMIX_simcall_answer(simcall);
      break;

#ifdef HAVE_LATENCY_BOUND_TRACKING
    case SIMCALL_COMM_IS_LATENCY_BOUNDED:
      simcall->comm_is_latency_bounded.result =
          SIMIX_comm_is_latency_bounded(simcall->comm_is_latency_bounded.comm);
      SIMIX_simcall_answer(simcall);
      break;
#endif

    case SIMCALL_RDV_CREATE:
      simcall->rdv_create.result = SIMIX_rdv_create(simcall->rdv_create.name);
      SIMIX_simcall_answer(simcall);
      break;

    case SIMCALL_RDV_DESTROY:
      SIMIX_rdv_destroy(simcall->rdv_destroy.rdv);
      SIMIX_simcall_answer(simcall);
      break;

    case SIMCALL_RDV_GEY_BY_NAME:
      simcall->rdv_get_by_name.result =
        SIMIX_rdv_get_by_name(simcall->rdv_get_by_name.name);
      SIMIX_simcall_answer(simcall);
      break;

    case SIMCALL_RDV_COMM_COUNT_BY_HOST:
      simcall->rdv_comm_count_by_host.result = SIMIX_rdv_comm_count_by_host(
          simcall->rdv_comm_count_by_host.rdv,
          simcall->rdv_comm_count_by_host.host);
      SIMIX_simcall_answer(simcall);
      break;

    case SIMCALL_RDV_GET_HEAD:
      simcall->rdv_get_head.result = SIMIX_rdv_get_head(simcall->rdv_get_head.rdv);
      SIMIX_simcall_answer(simcall);
      break;

    case SIMCALL_HOST_GET_BY_NAME:
      simcall->host_get_by_name.result =
        SIMIX_host_get_by_name(simcall->host_get_by_name.name);
      SIMIX_simcall_answer(simcall);
      break;

    case SIMCALL_HOST_GET_NAME:
      simcall->host_get_name.result =	SIMIX_host_get_name(simcall->host_get_name.host);
      SIMIX_simcall_answer(simcall);
      break;

    case SIMCALL_HOST_GET_PROPERTIES:
      simcall->host_get_properties.result =
        SIMIX_host_get_properties(simcall->host_get_properties.host);
      SIMIX_simcall_answer(simcall);
      break;

    case SIMCALL_HOST_GET_SPEED:
      simcall->host_get_speed.result = 
        SIMIX_host_get_speed(simcall->host_get_speed.host);
      SIMIX_simcall_answer(simcall);
      break;

    case SIMCALL_HOST_GET_AVAILABLE_SPEED:
      simcall->host_get_available_speed.result =
      	SIMIX_host_get_available_speed(simcall->host_get_available_speed.host);
      SIMIX_simcall_answer(simcall);
      break;

    case SIMCALL_HOST_GET_STATE:
      simcall->host_get_state.result = 
        SIMIX_host_get_state(simcall->host_get_state.host);
      SIMIX_simcall_answer(simcall);
      break;

    case SIMCALL_HOST_GET_DATA:
      simcall->host_get_data.result =	SIMIX_host_get_data(simcall->host_get_data.host);
      SIMIX_simcall_answer(simcall);
      break;

    case SIMCALL_HOST_SET_DATA:
      SIMIX_host_set_data(simcall->host_set_data.host, simcall->host_set_data.data);
      SIMIX_simcall_answer(simcall);
      break;

    case SIMCALL_HOST_EXECUTE:
      simcall->host_execute.result = SIMIX_host_execute(
	  simcall->host_execute.name,
	  simcall->host_execute.host,
	  simcall->host_execute.computation_amount,
	  simcall->host_execute.priority);
      SIMIX_simcall_answer(simcall);
      break;

    case SIMCALL_HOST_PARALLEL_EXECUTE:
      simcall->host_parallel_execute.result = SIMIX_host_parallel_execute(
	  simcall->host_parallel_execute.name,
	  simcall->host_parallel_execute.host_nb,
	  simcall->host_parallel_execute.host_list,
	  simcall->host_parallel_execute.computation_amount,
	  simcall->host_parallel_execute.communication_amount,
	  simcall->host_parallel_execute.amount,
	  simcall->host_parallel_execute.rate);
      SIMIX_simcall_answer(simcall);
      break;

    case SIMCALL_HOST_EXECUTION_DESTROY:
      SIMIX_host_execution_destroy(simcall->host_execution_destroy.execution);
      SIMIX_simcall_answer(simcall);
      break;

    case SIMCALL_HOST_EXECUTION_CANCEL:
      SIMIX_host_execution_cancel(simcall->host_execution_cancel.execution);
      SIMIX_simcall_answer(simcall);
      break;

    case SIMCALL_HOST_EXECUTION_GET_REMAINS:
      simcall->host_execution_get_remains.result =
        SIMIX_host_execution_get_remains(simcall->host_execution_get_remains.execution);
      SIMIX_simcall_answer(simcall);
      break;

    case SIMCALL_HOST_EXECUTION_GET_STATE:
      simcall->host_execution_get_state.result =
      	SIMIX_host_execution_get_state(simcall->host_execution_get_state.execution);
      SIMIX_simcall_answer(simcall);
      break;

    case SIMCALL_HOST_EXECUTION_SET_PRIORITY:
      SIMIX_host_execution_set_priority(
	  simcall->host_execution_set_priority.execution,
	  simcall->host_execution_set_priority.priority);
      SIMIX_simcall_answer(simcall);
      break;

    case SIMCALL_HOST_EXECUTION_WAIT:
      SIMIX_pre_host_execution_wait(simcall);
      break;

    case SIMCALL_PROCESS_CREATE:
      SIMIX_process_create(
          simcall->process_create.process,
	  simcall->process_create.name,
	  simcall->process_create.code,
	  simcall->process_create.data,
	  simcall->process_create.hostname,
	  simcall->process_create.argc,
	  simcall->process_create.argv,
	  simcall->process_create.properties);
      SIMIX_simcall_answer(simcall);
      break;

    case SIMCALL_PROCESS_KILL:
      SIMIX_process_kill(simcall->process_kill.process);
      SIMIX_simcall_answer(simcall);
      break;

    case SIMCALL_PROCESS_KILLALL:
      SIMIX_process_killall(simcall->issuer);
      SIMIX_simcall_answer(simcall);
      break;

    case SIMCALL_PROCESS_CLEANUP:
      SIMIX_process_cleanup(simcall->process_cleanup.process);
      SIMIX_simcall_answer(simcall);
      break;

    case SIMCALL_PROCESS_CHANGE_HOST:
      SIMIX_pre_process_change_host(
	  simcall->process_change_host.process,
	  simcall->process_change_host.dest);
      SIMIX_simcall_answer(simcall);
      break;

    case SIMCALL_PROCESS_SUSPEND:
      SIMIX_pre_process_suspend(simcall);
      break;

    case SIMCALL_PROCESS_RESUME:
      SIMIX_process_resume(simcall->process_resume.process, simcall->issuer);
      SIMIX_simcall_answer(simcall);
      break;

    case SIMCALL_PROCESS_COUNT:
      simcall->process_count.result = SIMIX_process_count();
      SIMIX_simcall_answer(simcall);
      break;

    case SIMCALL_PROCESS_GET_DATA:
      simcall->process_get_data.result =
        SIMIX_process_get_data(simcall->process_get_data.process);
      SIMIX_simcall_answer(simcall);
      break;

    case SIMCALL_PROCESS_SET_DATA:
      SIMIX_process_set_data(
	  simcall->process_set_data.process,
	  simcall->process_set_data.data);
      SIMIX_simcall_answer(simcall);
      break;

    case SIMCALL_PROCESS_GET_HOST:
      simcall->process_get_host.result = SIMIX_process_get_host(simcall->process_get_host.process);
      SIMIX_simcall_answer(simcall);
      break;

    case SIMCALL_PROCESS_GET_NAME:
      simcall->process_get_name.result = SIMIX_process_get_name(simcall->process_get_name.process);
      SIMIX_simcall_answer(simcall);
      break;

    case SIMCALL_PROCESS_IS_SUSPENDED:
      simcall->process_is_suspended.result =
        SIMIX_process_is_suspended(simcall->process_is_suspended.process);
      SIMIX_simcall_answer(simcall);
      break;

    case SIMCALL_PROCESS_GET_PROPERTIES:
      simcall->process_get_properties.result =
        SIMIX_process_get_properties(simcall->process_get_properties.process);
      SIMIX_simcall_answer(simcall);
      break;

    case SIMCALL_PROCESS_SLEEP:
      SIMIX_pre_process_sleep(simcall);
      break;

#ifdef HAVE_TRACING
    case SIMCALL_SET_CATEGORY:
      SIMIX_set_category(
          simcall->set_category.action,
          simcall->set_category.category);
      SIMIX_simcall_answer(simcall);
      break;
#endif

    case SIMCALL_MUTEX_INIT:
      simcall->mutex_init.result = SIMIX_mutex_init();
      SIMIX_simcall_answer(simcall);
      break;

    case SIMCALL_MUTEX_DESTROY:
      SIMIX_mutex_destroy(simcall->mutex_destroy.mutex);
      SIMIX_simcall_answer(simcall);
      break;

    case SIMCALL_MUTEX_LOCK:
      SIMIX_pre_mutex_lock(simcall);
      break;

    case SIMCALL_MUTEX_TRYLOCK:
      simcall->mutex_trylock.result =
	      SIMIX_mutex_trylock(simcall->mutex_trylock.mutex, simcall->issuer);
      SIMIX_simcall_answer(simcall);
      break;

    case SIMCALL_MUTEX_UNLOCK:
      SIMIX_mutex_unlock(simcall->mutex_unlock.mutex, simcall->issuer);
      SIMIX_simcall_answer(simcall);
      break;

    case SIMCALL_COND_INIT:
      simcall->cond_init.result = SIMIX_cond_init();
      SIMIX_simcall_answer(simcall);
      break;

    case SIMCALL_COND_DESTROY:
      SIMIX_cond_destroy(simcall->cond_destroy.cond);
      SIMIX_simcall_answer(simcall);
      break;

    case SIMCALL_COND_SIGNAL:
      SIMIX_cond_signal(simcall->cond_signal.cond);
      SIMIX_simcall_answer(simcall);
      break;

    case SIMCALL_COND_WAIT:
      SIMIX_pre_cond_wait(simcall);
      break;

    case SIMCALL_COND_WAIT_TIMEOUT:
      SIMIX_pre_cond_wait_timeout(simcall);
      break;

    case SIMCALL_COND_BROADCAST:
      SIMIX_cond_broadcast(simcall->cond_broadcast.cond);
      SIMIX_simcall_answer(simcall);
      break;

    case SIMCALL_SEM_INIT:
      simcall->sem_init.result = SIMIX_sem_init(simcall->sem_init.capacity);
      SIMIX_simcall_answer(simcall);
      break;

    case SIMCALL_SEM_DESTROY:
      SIMIX_sem_destroy(simcall->sem_destroy.sem);
      SIMIX_simcall_answer(simcall);
      break;

    case SIMCALL_SEM_RELEASE:
      SIMIX_sem_release(simcall->sem_release.sem);
      SIMIX_simcall_answer(simcall);
      break;

    case SIMCALL_SEM_WOULD_BLOCK:
      simcall->sem_would_block.result =
      	SIMIX_sem_would_block(simcall->sem_would_block.sem);
      SIMIX_simcall_answer(simcall);
      break;

    case SIMCALL_SEM_ACQUIRE:
      SIMIX_pre_sem_acquire(simcall);
      break;

    case SIMCALL_SEM_ACQUIRE_TIMEOUT:
      SIMIX_pre_sem_acquire_timeout(simcall);
      break;

    case SIMCALL_SEM_GET_CAPACITY:
      simcall->sem_get_capacity.result = 
        SIMIX_sem_get_capacity(simcall->sem_get_capacity.sem);
      SIMIX_simcall_answer(simcall);
      break;

    case SIMCALL_FILE_READ:
      SIMIX_pre_file_read(simcall);
      break;

    case SIMCALL_FILE_WRITE:
      SIMIX_pre_file_write(simcall);
      break;

    case SIMCALL_FILE_OPEN:
      SIMIX_pre_file_open(simcall);
      break;

    case SIMCALL_FILE_CLOSE:
      SIMIX_pre_file_close(simcall);
      break;

    case SIMCALL_FILE_STAT:
      SIMIX_pre_file_stat(simcall);
      break;

    case SIMCALL_NONE:
      THROWF(arg_error,0,"Asked to do the noop syscall on %s@%s",
          SIMIX_process_get_name(simcall->issuer),
          SIMIX_host_get_name(SIMIX_process_get_host(simcall->issuer))
          );
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
  }
}
