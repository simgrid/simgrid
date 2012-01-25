#include "smx_private.h"
#include "xbt/fifo.h"
#include "xbt/xbt_os_thread.h"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(simix_smurf, simix,
                                "Logging specific to SIMIX (SMURF)");

XBT_INLINE smx_req_t SIMIX_req_mine()
{
  smx_process_t issuer = SIMIX_process_self();
  return &issuer->request;
}

/**
 * \brief Makes the current process do a request to the kernel and yields
 * until completion.
 * \param self the current process
 */
void SIMIX_request_push(smx_process_t self)
{
  if (self != simix_global->maestro_process) {
    XBT_DEBUG("Yield process '%s' on request of type %s (%d)", self->name,
        SIMIX_request_name(self->request.call), self->request.call);
    SIMIX_process_yield(self);
  } else {
    SIMIX_request_pre(&self->request, 0);
  }
}

void SIMIX_request_answer(smx_req_t req)
{
  if (req->issuer != simix_global->maestro_process){
    XBT_DEBUG("Answer request %s (%d) issued by %s (%p)", SIMIX_request_name(req->call), req->call,
        req->issuer->name, req->issuer);
    req->issuer->request.call = REQ_NO_REQ;
    xbt_dynar_push_as(simix_global->process_to_run, smx_process_t, req->issuer);
  }
}

void SIMIX_request_pre(smx_req_t req, int value)
{
  XBT_DEBUG("Handling request %p: %s", req, SIMIX_request_name(req->call));

  switch (req->call) {
    case REQ_COMM_TEST:
      SIMIX_pre_comm_test(req);
      break;

    case REQ_COMM_TESTANY:
      SIMIX_pre_comm_testany(req, value);
      break;

    case REQ_COMM_WAIT:
      SIMIX_pre_comm_wait(req,
          req->comm_wait.comm,
          req->comm_wait.timeout,
          value);
      break;

    case REQ_COMM_WAITANY:
      SIMIX_pre_comm_waitany(req, value);
      break;

    case REQ_COMM_SEND:
    {
      smx_action_t comm = SIMIX_comm_isend(
          req->issuer,
          req->comm_send.rdv,
          req->comm_send.task_size,
          req->comm_send.rate,
          req->comm_send.src_buff,
          req->comm_send.src_buff_size,
          req->comm_send.match_fun,
          NULL, /* no clean function since it's not detached */
          req->comm_send.data,
          0);
      SIMIX_pre_comm_wait(req, comm, req->comm_send.timeout, 0);
      break;
    }

    case REQ_COMM_ISEND:
      req->comm_isend.result = SIMIX_comm_isend(
          req->issuer,
          req->comm_isend.rdv,
          req->comm_isend.task_size,
          req->comm_isend.rate,
          req->comm_isend.src_buff,
          req->comm_isend.src_buff_size,
          req->comm_isend.match_fun,
          req->comm_isend.clean_fun,
          req->comm_isend.data,
          req->comm_isend.detached);
      SIMIX_request_answer(req);
      break;

    case REQ_COMM_RECV:
    {
      smx_action_t comm = SIMIX_comm_irecv(
          req->issuer,
          req->comm_recv.rdv,
          req->comm_recv.dst_buff,
          req->comm_recv.dst_buff_size,
          req->comm_recv.match_fun,
          req->comm_recv.data);
      SIMIX_pre_comm_wait(req, comm, req->comm_recv.timeout, 0);
      break;
    }

    case REQ_COMM_IRECV:
      req->comm_irecv.result = SIMIX_comm_irecv(
          req->issuer,
          req->comm_irecv.rdv,
          req->comm_irecv.dst_buff,
          req->comm_irecv.dst_buff_size,
          req->comm_irecv.match_fun,
          req->comm_irecv.data);
      SIMIX_request_answer(req);
      break;

    case REQ_COMM_DESTROY:
      SIMIX_comm_destroy(req->comm_destroy.comm);
      SIMIX_request_answer(req);
      break;

    case REQ_COMM_CANCEL:
      SIMIX_comm_cancel(req->comm_cancel.comm);
      SIMIX_request_answer(req);
      break;

    case REQ_COMM_GET_REMAINS:
      req->comm_get_remains.result =
          SIMIX_comm_get_remains(req->comm_get_remains.comm);
      SIMIX_request_answer(req);
      break;

    case REQ_COMM_GET_STATE:
      req->comm_get_state.result =
          SIMIX_comm_get_state(req->comm_get_state.comm);
      SIMIX_request_answer(req);
      break;

    case REQ_COMM_GET_SRC_DATA:
      req->comm_get_src_data.result = SIMIX_comm_get_src_data(req->comm_get_src_data.comm);
      SIMIX_request_answer(req);
      break;

    case REQ_COMM_GET_DST_DATA:
      req->comm_get_dst_data.result = SIMIX_comm_get_dst_data(req->comm_get_dst_data.comm);
      SIMIX_request_answer(req);
      break;

    case REQ_COMM_GET_SRC_PROC:
      req->comm_get_src_proc.result =
          SIMIX_comm_get_src_proc(req->comm_get_src_proc.comm);
      SIMIX_request_answer(req);
      break;

    case REQ_COMM_GET_DST_PROC:
      req->comm_get_dst_proc.result =
          SIMIX_comm_get_dst_proc(req->comm_get_dst_proc.comm);
      SIMIX_request_answer(req);
      break;

#ifdef HAVE_LATENCY_BOUND_TRACKING
    case REQ_COMM_IS_LATENCY_BOUNDED:
      req->comm_is_latency_bounded.result =
          SIMIX_comm_is_latency_bounded(req->comm_is_latency_bounded.comm);
      SIMIX_request_answer(req);
      break;
#endif

    case REQ_RDV_CREATE:
      req->rdv_create.result = SIMIX_rdv_create(req->rdv_create.name);
      SIMIX_request_answer(req);
      break;

    case REQ_RDV_DESTROY:
      SIMIX_rdv_destroy(req->rdv_destroy.rdv);
      SIMIX_request_answer(req);
      break;

    case REQ_RDV_GEY_BY_NAME:
      req->rdv_get_by_name.result =
        SIMIX_rdv_get_by_name(req->rdv_get_by_name.name);
      SIMIX_request_answer(req);
      break;

    case REQ_RDV_COMM_COUNT_BY_HOST:
      req->rdv_comm_count_by_host.result = SIMIX_rdv_comm_count_by_host(
          req->rdv_comm_count_by_host.rdv,
          req->rdv_comm_count_by_host.host);
      SIMIX_request_answer(req);
      break;

    case REQ_RDV_GET_HEAD:
      req->rdv_get_head.result = SIMIX_rdv_get_head(req->rdv_get_head.rdv);
      SIMIX_request_answer(req);
      break;

    case REQ_HOST_GET_BY_NAME:
      req->host_get_by_name.result =
        SIMIX_host_get_by_name(req->host_get_by_name.name);
      SIMIX_request_answer(req);
      break;

    case REQ_HOST_GET_NAME:
      req->host_get_name.result =	SIMIX_host_get_name(req->host_get_name.host);
      SIMIX_request_answer(req);
      break;

    case REQ_HOST_GET_PROPERTIES:
      req->host_get_properties.result =
        SIMIX_host_get_properties(req->host_get_properties.host);
      SIMIX_request_answer(req);
      break;

    case REQ_HOST_GET_SPEED:
      req->host_get_speed.result = 
        SIMIX_host_get_speed(req->host_get_speed.host);
      SIMIX_request_answer(req);
      break;

    case REQ_HOST_GET_AVAILABLE_SPEED:
      req->host_get_available_speed.result =
      	SIMIX_host_get_available_speed(req->host_get_available_speed.host);
      SIMIX_request_answer(req);
      break;

    case REQ_HOST_GET_STATE:
      req->host_get_state.result = 
        SIMIX_host_get_state(req->host_get_state.host);
      SIMIX_request_answer(req);
      break;

    case REQ_HOST_GET_DATA:
      req->host_get_data.result =	SIMIX_host_get_data(req->host_get_data.host);
      SIMIX_request_answer(req);
      break;

    case REQ_HOST_SET_DATA:
      SIMIX_host_set_data(req->host_set_data.host, req->host_set_data.data);
      SIMIX_request_answer(req);
      break;

    case REQ_HOST_EXECUTE:
      req->host_execute.result = SIMIX_host_execute(
	  req->host_execute.name,
	  req->host_execute.host,
	  req->host_execute.computation_amount,
	  req->host_execute.priority);
      SIMIX_request_answer(req);
      break;

    case REQ_HOST_PARALLEL_EXECUTE:
      req->host_parallel_execute.result = SIMIX_host_parallel_execute(
	  req->host_parallel_execute.name,
	  req->host_parallel_execute.host_nb,
	  req->host_parallel_execute.host_list,
	  req->host_parallel_execute.computation_amount,
	  req->host_parallel_execute.communication_amount,
	  req->host_parallel_execute.amount,
	  req->host_parallel_execute.rate);
      SIMIX_request_answer(req);
      break;

    case REQ_HOST_EXECUTION_DESTROY:
      SIMIX_host_execution_destroy(req->host_execution_destroy.execution);
      SIMIX_request_answer(req);
      break;

    case REQ_HOST_EXECUTION_CANCEL:
      SIMIX_host_execution_cancel(req->host_execution_cancel.execution);
      SIMIX_request_answer(req);
      break;

    case REQ_HOST_EXECUTION_GET_REMAINS:
      req->host_execution_get_remains.result =
        SIMIX_host_execution_get_remains(req->host_execution_get_remains.execution);
      SIMIX_request_answer(req);
      break;

    case REQ_HOST_EXECUTION_GET_STATE:
      req->host_execution_get_state.result =
      	SIMIX_host_execution_get_state(req->host_execution_get_state.execution);
      SIMIX_request_answer(req);
      break;

    case REQ_HOST_EXECUTION_SET_PRIORITY:
      SIMIX_host_execution_set_priority(
	  req->host_execution_set_priority.execution,
	  req->host_execution_set_priority.priority);
      SIMIX_request_answer(req);
      break;

    case REQ_HOST_EXECUTION_WAIT:
      SIMIX_pre_host_execution_wait(req);
      break;

    case REQ_PROCESS_CREATE:
      SIMIX_process_create(
          req->process_create.process,
	  req->process_create.name,
	  req->process_create.code,
	  req->process_create.data,
	  req->process_create.hostname,
	  req->process_create.argc,
	  req->process_create.argv,
	  req->process_create.properties);
      SIMIX_request_answer(req);
      break;

    case REQ_PROCESS_KILL:
      SIMIX_process_kill(req->process_kill.process);
      SIMIX_request_answer(req);
      break;

    case REQ_PROCESS_KILLALL:
      SIMIX_process_killall(req->issuer);
      SIMIX_request_answer(req);
      break;

    case REQ_PROCESS_CLEANUP:
      SIMIX_process_cleanup(req->process_cleanup.process);
      SIMIX_request_answer(req);
      break;

    case REQ_PROCESS_CHANGE_HOST:
      SIMIX_pre_process_change_host(
	  req->process_change_host.process,
	  req->process_change_host.dest);
      SIMIX_request_answer(req);
      break;

    case REQ_PROCESS_SUSPEND:
      SIMIX_pre_process_suspend(req);
      break;

    case REQ_PROCESS_RESUME:
      SIMIX_process_resume(req->process_resume.process, req->issuer);
      SIMIX_request_answer(req);
      break;

    case REQ_PROCESS_COUNT:
      req->process_count.result = SIMIX_process_count();
      SIMIX_request_answer(req);
      break;

    case REQ_PROCESS_GET_DATA:
      req->process_get_data.result =
        SIMIX_process_get_data(req->process_get_data.process);
      SIMIX_request_answer(req);
      break;

    case REQ_PROCESS_SET_DATA:
      SIMIX_process_set_data(
	  req->process_set_data.process,
	  req->process_set_data.data);
      SIMIX_request_answer(req);
      break;

    case REQ_PROCESS_GET_HOST:
      req->process_get_host.result = SIMIX_process_get_host(req->process_get_host.process);
      SIMIX_request_answer(req);
      break;

    case REQ_PROCESS_GET_NAME:
      req->process_get_name.result = SIMIX_process_get_name(req->process_get_name.process);
      SIMIX_request_answer(req);
      break;

    case REQ_PROCESS_IS_SUSPENDED:
      req->process_is_suspended.result =
        SIMIX_process_is_suspended(req->process_is_suspended.process);
      SIMIX_request_answer(req);
      break;

    case REQ_PROCESS_GET_PROPERTIES:
      req->process_get_properties.result =
        SIMIX_process_get_properties(req->process_get_properties.process);
      SIMIX_request_answer(req);
      break;

    case REQ_PROCESS_SLEEP:
      SIMIX_pre_process_sleep(req);
      break;

#ifdef HAVE_TRACING
    case REQ_SET_CATEGORY:
      SIMIX_set_category(
          req->set_category.action,
          req->set_category.category);
      SIMIX_request_answer(req);
      break;
#endif

    case REQ_MUTEX_INIT:
      req->mutex_init.result = SIMIX_mutex_init();
      SIMIX_request_answer(req);
      break;

    case REQ_MUTEX_DESTROY:
      SIMIX_mutex_destroy(req->mutex_destroy.mutex);
      SIMIX_request_answer(req);
      break;

    case REQ_MUTEX_LOCK:
      SIMIX_pre_mutex_lock(req);
      break;

    case REQ_MUTEX_TRYLOCK:
      req->mutex_trylock.result =
	      SIMIX_mutex_trylock(req->mutex_trylock.mutex, req->issuer);
      SIMIX_request_answer(req);
      break;

    case REQ_MUTEX_UNLOCK:
      SIMIX_mutex_unlock(req->mutex_unlock.mutex, req->issuer);
      SIMIX_request_answer(req);
      break;

    case REQ_COND_INIT:
      req->cond_init.result = SIMIX_cond_init();
      SIMIX_request_answer(req);
      break;

    case REQ_COND_DESTROY:
      SIMIX_cond_destroy(req->cond_destroy.cond);
      SIMIX_request_answer(req);
      break;

    case REQ_COND_SIGNAL:
      SIMIX_cond_signal(req->cond_signal.cond);
      SIMIX_request_answer(req);
      break;

    case REQ_COND_WAIT:
      SIMIX_pre_cond_wait(req);
      break;

    case REQ_COND_WAIT_TIMEOUT:
      SIMIX_pre_cond_wait_timeout(req);
      break;

    case REQ_COND_BROADCAST:
      SIMIX_cond_broadcast(req->cond_broadcast.cond);
      SIMIX_request_answer(req);
      break;

    case REQ_SEM_INIT:
      req->sem_init.result = SIMIX_sem_init(req->sem_init.capacity);
      SIMIX_request_answer(req);
      break;

    case REQ_SEM_DESTROY:
      SIMIX_sem_destroy(req->sem_destroy.sem);
      SIMIX_request_answer(req);
      break;

    case REQ_SEM_RELEASE:
      SIMIX_sem_release(req->sem_release.sem);
      SIMIX_request_answer(req);
      break;

    case REQ_SEM_WOULD_BLOCK:
      req->sem_would_block.result =
      	SIMIX_sem_would_block(req->sem_would_block.sem);
      SIMIX_request_answer(req);
      break;

    case REQ_SEM_ACQUIRE:
      SIMIX_pre_sem_acquire(req);
      break;

    case REQ_SEM_ACQUIRE_TIMEOUT:
      SIMIX_pre_sem_acquire_timeout(req);
      break;

    case REQ_SEM_GET_CAPACITY:
      req->sem_get_capacity.result = 
        SIMIX_sem_get_capacity(req->sem_get_capacity.sem);
      SIMIX_request_answer(req);
      break;

    case REQ_FILE_READ:
      SIMIX_pre_file_read(req);
      break;

    case REQ_NO_REQ:
      THROWF(arg_error,0,"Asked to do the noop syscall on %s@%s",
          SIMIX_process_get_name(req->issuer),
          SIMIX_host_get_name(SIMIX_process_get_host(req->issuer))
          );
      break;

  }
}

void SIMIX_request_post(smx_action_t action)
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
