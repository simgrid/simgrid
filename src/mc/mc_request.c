#include "private.h"

int MC_request_depend(smx_req_t r1, smx_req_t r2)
{
  if (r1->issuer == r2->issuer)
    return FALSE;

  if(r1->call == REQ_COMM_ISEND && r2->call == REQ_COMM_IRECV)
    return FALSE;

  if(r1->call == REQ_COMM_IRECV && r2->call == REQ_COMM_ISEND)
    return FALSE;

  /* FIXME: the following rule assumes that the result of the
   * isend/irecv call is not stored in a buffer used in the
   * wait/test call.
  if(   (r1->call == REQ_COMM_ISEND || r1->call == REQ_COMM_IRECV)
     && (r2->call == REQ_COMM_WAIT || r2->call == REQ_COMM_TEST))
    return FALSE;*/

  /* FIXME: the following rule assumes that the result of the
   * isend/irecv call is not stored in a buffer used in the
   * wait/test call.
  if(   (r2->call == REQ_COMM_ISEND || r2->call == REQ_COMM_IRECV)
     && (r1->call == REQ_COMM_WAIT || r1->call == REQ_COMM_TEST))
    return FALSE;*/

  if(r1->call == REQ_COMM_ISEND && r2->call == REQ_COMM_ISEND
      && r1->comm_isend.rdv != r2->comm_isend.rdv)
    return FALSE;

  if(r1->call == REQ_COMM_IRECV && r2->call == REQ_COMM_IRECV
      && r1->comm_irecv.rdv != r2->comm_irecv.rdv)
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

  return TRUE;
}

char *MC_request_to_string(smx_req_t req)
{
  char *type = NULL, *args = NULL, *str = NULL; 
  smx_action_t act = NULL;
  size_t size = 0;
  
  switch(req->call){
    case REQ_COMM_ISEND:
      type = bprintf("iSend");
      args = bprintf("src=%s, buff=%p, size=%zu", req->issuer->name, 
                     req->comm_isend.src_buff, req->comm_isend.src_buff_size);
      break;
    case REQ_COMM_IRECV:
      size = req->comm_irecv.dst_buff_size ? *req->comm_irecv.dst_buff_size : 0;
      type = bprintf("iRecv");
      args = bprintf("dst=%s, buff=%p, size=%zu", req->issuer->name, 
                     req->comm_irecv.dst_buff, size);
      break;
    case REQ_COMM_WAIT:
      act = req->comm_wait.comm;
      type = bprintf("Wait");
      args  = bprintf("comm=%p [%s(%lu) -> %s(%lu)]", act,
                      act->comm.src_proc ? act->comm.src_proc->name : "",
                      act->comm.src_proc ? act->comm.src_proc->pid : 0,
                      act->comm.dst_proc ? act->comm.dst_proc->name : "",
                      act->comm.dst_proc ? act->comm.dst_proc->pid : 0);
      break;
    case REQ_COMM_TEST:
      act = req->comm_test.comm;
      type = bprintf("Test");
      args  = bprintf("comm=%p [%s -> %s]", act,
                      act->comm.src_proc ? act->comm.src_proc->name : "",
                      act->comm.dst_proc ? act->comm.dst_proc->name : "");
      break;

    case REQ_COMM_WAITANY:
      type = bprintf("WaitAny");
      args = bprintf("-");
      /* FIXME: improve output */
      break;

    case REQ_COMM_TESTANY:
       type = bprintf("TestAny");
       args = bprintf("-");
       /* FIXME: improve output */
       break;

    default:
      THROW_UNIMPLEMENTED;
  }

  str = bprintf("[(%lu)%s] %s (%s)", req->issuer->pid ,req->issuer->name, type, args);
  xbt_free(type);
  xbt_free(args);
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
