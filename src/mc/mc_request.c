#include "private.h"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(mc_request, mc,
                                "Logging specific to MC (request)");

static char* pointer_to_string(void* pointer);
static char* buff_size_to_string(size_t size);

int MC_request_depend(smx_req_t r1, smx_req_t r2)
{
  if(_surf_do_model_check == 2)
    return TRUE;
  

  if (r1->issuer == r2->issuer)
    return FALSE;

  if(r1->call == REQ_COMM_ISEND && r2->call == REQ_COMM_IRECV)
    return FALSE;

  if(r1->call == REQ_COMM_IRECV && r2->call == REQ_COMM_ISEND)
    return FALSE;

  if(   (r1->call == REQ_COMM_ISEND || r1->call == REQ_COMM_IRECV)
       &&  r2->call == REQ_COMM_WAIT){

    if(r2->comm_wait.comm->comm.rdv == NULL)
      return FALSE;

    smx_rdv_t rdv = r1->call == REQ_COMM_ISEND ? r1->comm_isend.rdv : r1->comm_irecv.rdv;

    if(r2->comm_wait.comm->comm.rdv != rdv)
      return FALSE;

    if(r2->comm_wait.comm->comm.type == SIMIX_COMM_SEND && r1->call == REQ_COMM_ISEND)
      return FALSE;

    if(r2->comm_wait.comm->comm.type == SIMIX_COMM_RECEIVE && r1->call == REQ_COMM_IRECV)
      return FALSE;
  }

  if(   (r2->call == REQ_COMM_ISEND || r2->call == REQ_COMM_IRECV)
       &&  r1->call == REQ_COMM_WAIT){

    if(r1->comm_wait.comm->comm.rdv != NULL)
      return FALSE;

    smx_rdv_t rdv = r2->call == REQ_COMM_ISEND ? r2->comm_isend.rdv : r2->comm_irecv.rdv;

    if(r1->comm_wait.comm->comm.rdv != rdv)
      return FALSE;

    if(r1->comm_wait.comm->comm.type == SIMIX_COMM_SEND && r2->call == REQ_COMM_ISEND)
      return FALSE;

    if(r1->comm_wait.comm->comm.type == SIMIX_COMM_RECEIVE && r2->call == REQ_COMM_IRECV)
      return FALSE;
  }

  /* FIXME: the following rule assumes that the result of the
   * isend/irecv call is not stored in a buffer used in the
   * test call. */
  if(   (r1->call == REQ_COMM_ISEND || r1->call == REQ_COMM_IRECV)
     &&  r2->call == REQ_COMM_TEST)
    return FALSE;

  /* FIXME: the following rule assumes that the result of the
   * isend/irecv call is not stored in a buffer used in the
   * test call.*/
  if(   (r2->call == REQ_COMM_ISEND || r2->call == REQ_COMM_IRECV)
     && r1->call == REQ_COMM_TEST)
    return FALSE;

  if(r1->call == REQ_COMM_ISEND && r2->call == REQ_COMM_ISEND
      && r1->comm_isend.rdv != r2->comm_isend.rdv)
    return FALSE;

  if(r1->call == REQ_COMM_IRECV && r2->call == REQ_COMM_IRECV
      && r1->comm_irecv.rdv != r2->comm_irecv.rdv)
    return FALSE;

  if(r1->call == REQ_COMM_WAIT && (r2->call == REQ_COMM_WAIT || r2->call == REQ_COMM_TEST)
     && (r1->comm_wait.comm->comm.src_proc == NULL
         || r1->comm_wait.comm->comm.dst_proc == NULL))
    return FALSE;

  if(r2->call == REQ_COMM_WAIT && (r1->call == REQ_COMM_WAIT || r1->call == REQ_COMM_TEST)
     && (r2->comm_wait.comm->comm.src_proc == NULL
         || r2->comm_wait.comm->comm.dst_proc == NULL))
    return FALSE;

  if(r1->call == REQ_COMM_WAIT && r2->call == REQ_COMM_WAIT
      && r1->comm_wait.comm->comm.src_buff == r2->comm_wait.comm->comm.src_buff
      && r1->comm_wait.comm->comm.dst_buff == r2->comm_wait.comm->comm.dst_buff)
    return FALSE;

  if (r1->call == REQ_COMM_WAIT && r2->call == REQ_COMM_WAIT
      && r1->comm_wait.comm->comm.src_buff != NULL
      && r1->comm_wait.comm->comm.dst_buff != NULL
      && r2->comm_wait.comm->comm.src_buff != NULL
      && r2->comm_wait.comm->comm.dst_buff != NULL
      && r1->comm_wait.comm->comm.dst_buff != r2->comm_wait.comm->comm.src_buff
      && r1->comm_wait.comm->comm.dst_buff != r2->comm_wait.comm->comm.dst_buff
      && r2->comm_wait.comm->comm.dst_buff != r1->comm_wait.comm->comm.src_buff)
    return FALSE;

  if(r1->call == REQ_COMM_TEST &&
      (r1->comm_test.comm == NULL
       || r1->comm_test.comm->comm.src_buff == NULL
       || r1->comm_test.comm->comm.dst_buff == NULL))
    return FALSE;

  if(r2->call == REQ_COMM_TEST &&
      (r2->comm_test.comm == NULL
       || r2->comm_test.comm->comm.src_buff == NULL
       || r2->comm_test.comm->comm.dst_buff == NULL))
    return FALSE;

  if(r1->call == REQ_COMM_TEST && r2->call == REQ_COMM_WAIT
      && r1->comm_test.comm->comm.src_buff == r2->comm_wait.comm->comm.src_buff
      && r1->comm_test.comm->comm.dst_buff == r2->comm_wait.comm->comm.dst_buff)
    return FALSE;

  if(r1->call == REQ_COMM_WAIT && r2->call == REQ_COMM_TEST
      && r1->comm_wait.comm->comm.src_buff == r2->comm_test.comm->comm.src_buff
      && r1->comm_wait.comm->comm.dst_buff == r2->comm_test.comm->comm.dst_buff)
    return FALSE;

  if (r1->call == REQ_COMM_WAIT && r2->call == REQ_COMM_TEST
        && r1->comm_wait.comm->comm.src_buff != NULL
        && r1->comm_wait.comm->comm.dst_buff != NULL
        && r2->comm_test.comm->comm.src_buff != NULL
        && r2->comm_test.comm->comm.dst_buff != NULL
        && r1->comm_wait.comm->comm.dst_buff != r2->comm_test.comm->comm.src_buff
        && r1->comm_wait.comm->comm.dst_buff != r2->comm_test.comm->comm.dst_buff
        && r2->comm_test.comm->comm.dst_buff != r1->comm_wait.comm->comm.src_buff)
    return FALSE;

  if (r1->call == REQ_COMM_TEST && r2->call == REQ_COMM_WAIT
          && r1->comm_test.comm->comm.src_buff != NULL
          && r1->comm_test.comm->comm.dst_buff != NULL
          && r2->comm_wait.comm->comm.src_buff != NULL
          && r2->comm_wait.comm->comm.dst_buff != NULL
          && r1->comm_test.comm->comm.dst_buff != r2->comm_wait.comm->comm.src_buff
          && r1->comm_test.comm->comm.dst_buff != r2->comm_wait.comm->comm.dst_buff
          && r2->comm_wait.comm->comm.dst_buff != r1->comm_test.comm->comm.src_buff)
      return FALSE;


  return TRUE;
}

static char* pointer_to_string(void* pointer) {

  if (XBT_LOG_ISENABLED(mc_request, xbt_log_priority_verbose))
    return bprintf("%p", pointer);

  return xbt_strdup("(verbose only)");
}

static char* buff_size_to_string(size_t buff_size) {

  if (XBT_LOG_ISENABLED(mc_request, xbt_log_priority_verbose))
    return bprintf("%zu", buff_size);

  return xbt_strdup("(verbose only)");
}


char *MC_request_to_string(smx_req_t req, int value)
{
  char *type = NULL, *args = NULL, *str = NULL, *p = NULL, *bs = NULL;
  smx_action_t act = NULL;
  size_t size = 0;

  switch(req->call){
    case REQ_COMM_ISEND:
      type = xbt_strdup("iSend");
      p = pointer_to_string(req->comm_isend.src_buff);
      bs = buff_size_to_string(req->comm_isend.src_buff_size);
      args = bprintf("src=%s, buff=%s, size=%s", req->issuer->name, p, bs);
      break;
    case REQ_COMM_IRECV:
      size = req->comm_irecv.dst_buff_size ? *req->comm_irecv.dst_buff_size : 0;
      type = xbt_strdup("iRecv");
      p = pointer_to_string(req->comm_irecv.dst_buff); 
      bs = buff_size_to_string(size);
      args = bprintf("dst=%s, buff=%s, size=%s", req->issuer->name, p, bs);
      break;
    case REQ_COMM_WAIT:
      act = req->comm_wait.comm;
      if(value == -1){
        type = xbt_strdup("WaitTimeout");
	p = pointer_to_string(act);
	args = bprintf("comm=%p", p);
      }else{
        type = xbt_strdup("Wait");
	p = pointer_to_string(act);
	args  = bprintf("comm=%s [(%lu)%s -> (%lu)%s]", p,
                        act->comm.src_proc ? act->comm.src_proc->pid : 0,
                        act->comm.src_proc ? act->comm.src_proc->name : "",
                        act->comm.dst_proc ? act->comm.dst_proc->pid : 0,
                        act->comm.dst_proc ? act->comm.dst_proc->name : "");
      }
      break;
    case REQ_COMM_TEST:
      act = req->comm_test.comm;
      if(act->comm.src_proc == NULL || act->comm.dst_proc == NULL){
        type = xbt_strdup("Test FALSE");
	p = pointer_to_string(act);
        args = bprintf("comm=%s", p);
      }else{
        type = xbt_strdup("Test TRUE");
	p = pointer_to_string(act);
        args  = bprintf("comm=%s [(%lu)%s -> (%lu)%s]", p,
                        act->comm.src_proc->pid, act->comm.src_proc->name,
                        act->comm.dst_proc->pid, act->comm.dst_proc->name);
      }
      break;

    case REQ_COMM_WAITANY:
      type = xbt_strdup("WaitAny");
      p = pointer_to_string(xbt_dynar_get_as(req->comm_waitany.comms, value, smx_action_t));
      args = bprintf("comm=%s (%d of %lu)", p,
                     value+1, xbt_dynar_length(req->comm_waitany.comms));
      break;

    case REQ_COMM_TESTANY:
      if(value == -1){
        type = xbt_strdup("TestAny FALSE");
        args = xbt_strdup("-");
      }else{
        type = xbt_strdup("TestAny");
        args = bprintf("(%d of %lu)", value+1, xbt_dynar_length(req->comm_testany.comms));
      }
      break;

    default:
      THROW_UNIMPLEMENTED;
  }

  str = bprintf("[(%lu)%s] %s (%s)", req->issuer->pid ,req->issuer->name, type, args);
  xbt_free(type);
  xbt_free(args);
  xbt_free(p);
  xbt_free(bs);
  return str;
}

unsigned int MC_request_testany_fail(smx_req_t req)
{
  unsigned int cursor;
  smx_action_t action;

  xbt_dynar_foreach(req->comm_testany.comms, cursor, action){
    if(action->comm.src_proc && action->comm.dst_proc)
      return FALSE;
  }

  return TRUE;
}

int MC_request_is_visible(smx_req_t req)
{
  return req->call == REQ_COMM_ISEND
     || req->call == REQ_COMM_IRECV
     || req->call == REQ_COMM_WAIT
     || req->call == REQ_COMM_WAITANY
     || req->call == REQ_COMM_TEST
     || req->call == REQ_COMM_TESTANY;
}

int MC_request_is_enabled(smx_req_t req)
{
  unsigned int index = 0;
  smx_action_t act;

  switch (req->call) {

    case REQ_COMM_WAIT:
      /* FIXME: check also that src and dst processes are not suspended */

      /* If it has a timeout it will be always be enabled, because even if the
       * communication is not ready, it can timeout and won't block.
       * On the other hand if it hasn't a timeout, check if the comm is ready.*/
      if(req->comm_wait.timeout >= 0){
        return TRUE;
      }else{
        act = req->comm_wait.comm;
        return (act->comm.src_proc && act->comm.dst_proc);
      }
      break;

    case REQ_COMM_WAITANY:
      /* Check if it has at least one communication ready */
      xbt_dynar_foreach(req->comm_waitany.comms, index, act) {
        if (act->comm.src_proc && act->comm.dst_proc){
          return TRUE;
        }
      }
      return FALSE;
      break;

    default:
      /* The rest of the request are always enabled */
      return TRUE;
  }
}

int MC_request_is_enabled_by_idx(smx_req_t req, unsigned int idx)
{
  smx_action_t act;

  switch (req->call) {

    case REQ_COMM_WAIT:
      /* FIXME: check also that src and dst processes are not suspended */
      act = req->comm_wait.comm;
      return (act->comm.src_proc && act->comm.dst_proc);
      break;

    case REQ_COMM_WAITANY:
      act = xbt_dynar_get_as(req->comm_waitany.comms, idx, smx_action_t);
      return (act->comm.src_proc && act->comm.dst_proc);
      break;

    case REQ_COMM_TESTANY:
      act = xbt_dynar_get_as(req->comm_testany.comms, idx, smx_action_t);
      return (act->comm.src_proc && act->comm.dst_proc);
      break;

    default:
      return TRUE;
  }
}

int MC_process_is_enabled(smx_process_t process)
{
  if (process->request.call != REQ_NO_REQ && MC_request_is_enabled(&process->request))
    return TRUE;

  return FALSE;
}
