#include "private.h"

int MC_request_depend(smx_req_t r1, smx_req_t r2)
{
  if (r1->issuer == r2->issuer)
    return FALSE;

  if (r1->call == REQ_COMM_ISEND && r2->call == REQ_COMM_ISEND
      && r1->comm_isend.rdv != r2->comm_isend.rdv)
    return FALSE;

  if (r1->call == REQ_COMM_IRECV && r2->call == REQ_COMM_IRECV
      && r1->comm_irecv.rdv != r2->comm_irecv.rdv)
    return FALSE;

  if (r1->call == REQ_COMM_WAIT && r2->call == REQ_COMM_WAIT
      && r1->comm_wait.comm == r2->comm_wait.comm)
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
      args  = bprintf("%p [%s(%lu) -> %s(%lu)]", act,
                      act->comm.src_proc ? act->comm.src_proc->name : "",
                      act->comm.src_proc ? act->comm.src_proc->pid : 0,
                      act->comm.dst_proc ? act->comm.dst_proc->name : "",
                      act->comm.dst_proc ? act->comm.dst_proc->pid : 0);
      break;
    case REQ_COMM_TEST:
      act = req->comm_test.comm;
      type = bprintf("Test");
      args  = bprintf("%p [%s -> %s]", act, 
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
