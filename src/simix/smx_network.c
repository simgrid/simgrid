/* Copyright (c) 2009, 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "smx_private.h"
#include "xbt/log.h"
#include "mc/mc.h"
#include "xbt/dict.h"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(simix_network, simix,
                                "Logging specific to SIMIX (network)");

static xbt_dict_t rdv_points = NULL;
unsigned long int smx_total_comms = 0;

static void SIMIX_waitany_req_remove_from_actions(smx_req_t req);
static void SIMIX_comm_copy_data(smx_action_t comm);
static smx_action_t SIMIX_comm_new(e_smx_comm_type_t type);
static XBT_INLINE void SIMIX_rdv_push(smx_rdv_t rdv, smx_action_t comm);
static smx_action_t SIMIX_rdv_get_request(smx_rdv_t rdv, e_smx_comm_type_t type,
					  int (*match_fun)(void *, void *), void *);
static void SIMIX_rdv_free(void *data);

void SIMIX_network_init(void)
{
  rdv_points = xbt_dict_new_homogeneous(SIMIX_rdv_free);
}

void SIMIX_network_exit(void)
{
  xbt_dict_free(&rdv_points);
}

/******************************************************************************/
/*                           Rendez-Vous Points                               */
/******************************************************************************/

smx_rdv_t SIMIX_rdv_create(const char *name)
{
  /* two processes may have pushed the same rdv_create request at the same time */
  smx_rdv_t rdv = name ? xbt_dict_get_or_null(rdv_points, name) : NULL;

  if (!rdv) {
    rdv = xbt_new0(s_smx_rvpoint_t, 1);
    rdv->name = name ? xbt_strdup(name) : NULL;
    rdv->comm_fifo = xbt_fifo_new();

    if (rdv->name)
      xbt_dict_set(rdv_points, rdv->name, rdv, NULL);
  }
  return rdv;
}

void SIMIX_rdv_destroy(smx_rdv_t rdv)
{
  if (rdv->name)
    xbt_dict_remove(rdv_points, rdv->name);
}

void SIMIX_rdv_free(void *data)
{
  smx_rdv_t rdv = (smx_rdv_t) data;
  xbt_free(rdv->name);
  xbt_fifo_free(rdv->comm_fifo);
  xbt_free(rdv);  
}

smx_rdv_t SIMIX_rdv_get_by_name(const char *name)
{
  return xbt_dict_get_or_null(rdv_points, name);
}

int SIMIX_rdv_comm_count_by_host(smx_rdv_t rdv, smx_host_t host)
{
  smx_action_t comm = NULL;
  xbt_fifo_item_t item = NULL;
  int count = 0;

  xbt_fifo_foreach(rdv->comm_fifo, item, comm, smx_action_t) {
    if (comm->comm.src_proc->smx_host == host)
      count++;
  }

  return count;
}

smx_action_t SIMIX_rdv_get_head(smx_rdv_t rdv)
{
  return xbt_fifo_get_item_content(xbt_fifo_get_first_item(rdv->comm_fifo));
}

/**
 *  \brief Push a communication request into a rendez-vous point
 *  \param rdv The rendez-vous point
 *  \param comm The communication request
 */
static XBT_INLINE void SIMIX_rdv_push(smx_rdv_t rdv, smx_action_t comm)
{
  xbt_fifo_push(rdv->comm_fifo, comm);
  comm->comm.rdv = rdv;
}

/**
 *  \brief Remove a communication request from a rendez-vous point
 *  \param rdv The rendez-vous point
 *  \param comm The communication request
 */
XBT_INLINE void SIMIX_rdv_remove(smx_rdv_t rdv, smx_action_t comm)
{
  xbt_fifo_remove(rdv->comm_fifo, comm);
  comm->comm.rdv = NULL;
}

/**
 *  \brief Wrapper to SIMIX_rdv_get_request
 */
smx_action_t SIMIX_comm_get_send_match(smx_rdv_t rdv, int (*match_fun)(void*, void*), void* data) {
   return SIMIX_rdv_get_request(rdv, SIMIX_COMM_SEND, match_fun, data);
}

/**
 *  \brief Checks if there is a communication action queued in a rendez-vous matching our needs
 *  \param type The type of communication we are looking for (comm_send, comm_recv)
 *  \return The communication action if found, NULL otherwise
 */
smx_action_t SIMIX_rdv_get_request(smx_rdv_t rdv, e_smx_comm_type_t type,
                                   int (*match_fun)(void *, void *), void *data)
{
  // FIXME rewrite this function by using SIMIX_rdv_has_send/recv_match
  smx_action_t action;
  xbt_fifo_item_t item;
  void* req_data = NULL;

  xbt_fifo_foreach(rdv->comm_fifo, item, action, smx_action_t) {
    if (action->comm.type == SIMIX_COMM_SEND) {
      req_data = action->comm.src_data;
    } else if (action->comm.type == SIMIX_COMM_RECEIVE) {
      req_data = action->comm.dst_data;
    }
    if (action->comm.type == type && (!match_fun || match_fun(data, req_data))) {
      XBT_DEBUG("Found a matching communication action %p", action);
      xbt_fifo_remove_item(rdv->comm_fifo, item);
      xbt_fifo_free_item(item);
      action->comm.refcount++;
      action->comm.rdv = NULL;
      return action;
    }
    XBT_DEBUG("Sorry, communication action %p does not match our needs:"
           " its type is %d but we are looking for a comm of type %d",
           action, action->comm.type, type);
  }
  XBT_DEBUG("No matching communication action found");
  return NULL;
}

/**
 *  \brief Checks if there is a send communication action
 *  queued in a rendez-vous matching our needs.
 *  \return 1 if found, 0 otherwise
 */
int SIMIX_comm_has_send_match(smx_rdv_t rdv, int (*match_fun)(void*, void*), void* data) {

  smx_action_t action;
  xbt_fifo_item_t item;

  xbt_fifo_foreach(rdv->comm_fifo, item, action, smx_action_t){
    if (action->comm.type == SIMIX_COMM_SEND
        && (!match_fun || match_fun(data, action->comm.src_data))) {
      XBT_DEBUG("Found a matching communication action %p", action);
      return 1;
    }
  }
  XBT_DEBUG("No matching communication action found");
  return 0;
}

/**
 *  \brief Checks if there is a recv communication action
 *  queued in a rendez-vous matching our needs.
 *  \return 1 if found, 0 otherwise
 */
int SIMIX_comm_has_recv_match(smx_rdv_t rdv, int (*match_fun)(void*, void*), void* data) {

  smx_action_t action;
  xbt_fifo_item_t item;

  xbt_fifo_foreach(rdv->comm_fifo, item, action, smx_action_t) {
    if (action->comm.type == SIMIX_COMM_RECEIVE
        && (!match_fun || match_fun(data, action->comm.dst_data))) {
      XBT_DEBUG("Found a matching communication action %p", action);
      return 1;
    }
  }
  XBT_DEBUG("No matching communication action found");
  return 0;
}

/******************************************************************************/
/*                            Comunication Actions                            */
/******************************************************************************/

/**
 *  \brief Creates a new comunicate action
 *  \param type The type of request (comm_send, comm_recv)
 *  \return The new comunicate action
 */
smx_action_t SIMIX_comm_new(e_smx_comm_type_t type)
{
  smx_action_t act;

  /* alloc structures */
  act = xbt_mallocator_get(simix_global->action_mallocator);

  act->type = SIMIX_ACTION_COMMUNICATE;
  act->state = SIMIX_WAITING;

  /* set communication */
  act->comm.type = type;
  act->comm.refcount = 1;

#ifdef HAVE_LATENCY_BOUND_TRACKING
  //initialize with unknown value
  act->latency_limited = -1;
#endif

#ifdef HAVE_TRACING
  act->category = NULL;
#endif

  XBT_DEBUG("Create communicate action %p", act);
  ++smx_total_comms;

  return act;
}

/**
 *  \brief Destroy a communicate action
 *  \param action The communicate action to be destroyed
 */
void SIMIX_comm_destroy(smx_action_t action)
{
  XBT_DEBUG("Destroy action %p (refcount: %d), state: %d",
      action, action->comm.refcount, action->state);

  if (action->comm.refcount <= 0) {
	xbt_backtrace_display_current();
    xbt_die("the refcount of comm %p is already 0 before decreasing it. "
            "That's a bug!", action);
  }
  action->comm.refcount--;
  if (action->comm.refcount > 0)
    return;
  XBT_DEBUG("Really free communication %p; refcount is now %d", action,
        action->comm.refcount);

#ifdef HAVE_LATENCY_BOUND_TRACKING
    action->latency_limited = SIMIX_comm_is_latency_bounded( action ) ;
#endif

  xbt_free(action->name);
  SIMIX_comm_destroy_internal_actions(action);

  if (action->comm.detached && action->state != SIMIX_DONE) {
    /* the communication has failed and was detached:
     * we have to free the buffer */
    if (action->comm.clean_fun) {
      action->comm.clean_fun(action->comm.src_buff);
    }
    action->comm.src_buff = NULL;
  }

  xbt_mallocator_release(simix_global->action_mallocator, action);
}

void SIMIX_comm_destroy_internal_actions(smx_action_t action)
{
  if (action->comm.surf_comm){
#ifdef HAVE_LATENCY_BOUND_TRACKING
    action->latency_limited = SIMIX_comm_is_latency_bounded(action);
#endif
    action->comm.surf_comm->model_type->action_unref(action->comm.surf_comm);
    action->comm.surf_comm = NULL;
  }

  if (action->comm.src_timeout){
    action->comm.src_timeout->model_type->action_unref(action->comm.src_timeout);
    action->comm.src_timeout = NULL;
  }

  if (action->comm.dst_timeout){
    action->comm.dst_timeout->model_type->action_unref(action->comm.dst_timeout);
    action->comm.dst_timeout = NULL;
  }
}

smx_action_t SIMIX_comm_isend(smx_process_t src_proc, smx_rdv_t rdv,
                              double task_size, double rate,
                              void *src_buff, size_t src_buff_size,
                              int (*match_fun)(void *, void *),
                              void (*clean_fun)(void *), // used to free the action in case of problem after a detached send
                              void *data,
                              int detached)
{
  smx_action_t action;

  /* Look for communication request matching our needs.
     If it is not found then create it and push it into the rendez-vous point */
  action = SIMIX_rdv_get_request(rdv, SIMIX_COMM_RECEIVE, match_fun, data);

  if (!action) {
    action = SIMIX_comm_new(SIMIX_COMM_SEND);
    SIMIX_rdv_push(rdv, action);
  } else {
    action->state = SIMIX_READY;
    action->comm.type = SIMIX_COMM_READY;
  }
  xbt_fifo_push(src_proc->comms, action);

  /* if the communication action is detached then decrease the refcount
   * by one, so it will be eliminated by the receiver's destroy call */
  if (detached) {
    action->comm.detached = 1;
    action->comm.refcount--;
    action->comm.clean_fun = clean_fun;
  } else {
    action->comm.clean_fun = NULL;
  }

  /* Setup the communication request */
  action->comm.src_proc = src_proc;
  action->comm.task_size = task_size;
  action->comm.rate = rate;
  action->comm.src_buff = src_buff;
  action->comm.src_buff_size = src_buff_size;
  action->comm.src_data = data;

  if (MC_IS_ENABLED) {
    action->state = SIMIX_RUNNING;
    return action;
  }

  SIMIX_comm_start(action);
  return (detached ? NULL : action);
}

smx_action_t SIMIX_comm_irecv(smx_process_t dst_proc, smx_rdv_t rdv,
                      void *dst_buff, size_t *dst_buff_size,
                      int (*match_fun)(void *, void *), void *data)
{
  smx_action_t action;

  /* Look for communication request matching our needs.
   * If it is not found then create it and push it into the rendez-vous point
   */
  action = SIMIX_rdv_get_request(rdv, SIMIX_COMM_SEND, match_fun, data);

  if (!action) {
    action = SIMIX_comm_new(SIMIX_COMM_RECEIVE);
    SIMIX_rdv_push(rdv, action);
  } else {
    action->state = SIMIX_READY;
    action->comm.type = SIMIX_COMM_READY;
  }
  xbt_fifo_push(dst_proc->comms, action);

  /* Setup communication request */
  action->comm.dst_proc = dst_proc;
  action->comm.dst_buff = dst_buff;
  action->comm.dst_buff_size = dst_buff_size;
  action->comm.dst_data = data;

  if (MC_IS_ENABLED) {
    action->state = SIMIX_RUNNING;
    return action;
  }

  SIMIX_comm_start(action);
  return action;
}

void SIMIX_pre_comm_wait(smx_req_t req, smx_action_t action, double timeout, int idx)
{

  /* the request may be a wait, a send or a recv */
  surf_action_t sleep;

  /* Associate this request to the action */
  xbt_fifo_push(action->request_list, req);
  req->issuer->waiting_action = action;

  if (MC_IS_ENABLED) {
    if (idx == 0) {
      action->state = SIMIX_DONE;
    } else {
      /* If we reached this point, the wait request must have a timeout */
      /* Otherwise it shouldn't be enabled and executed by the MC */
      if (timeout == -1)
        THROW_IMPOSSIBLE;

      if (action->comm.src_proc == req->issuer)
        action->state = SIMIX_SRC_TIMEOUT;
      else
        action->state = SIMIX_DST_TIMEOUT;
    }

    SIMIX_comm_finish(action);
    return;
  }

  /* If the action has already finish perform the error handling, */
  /* otherwise set up a waiting timeout on the right side         */
  if (action->state != SIMIX_WAITING && action->state != SIMIX_RUNNING) {
    SIMIX_comm_finish(action);
  } else { /* if (timeout >= 0) { we need a surf sleep action even when there is no timeout, otherwise surf won't tell us when the host fails */
    sleep = surf_workstation_model->extension.workstation.sleep(req->issuer->smx_host->host, timeout);
    surf_workstation_model->action_data_set(sleep, action);

    if (req->issuer == action->comm.src_proc)
      action->comm.src_timeout = sleep;
    else
      action->comm.dst_timeout = sleep;
  }
}

void SIMIX_pre_comm_test(smx_req_t req)
{
  smx_action_t action = req->comm_test.comm;

  if(MC_IS_ENABLED){
    req->comm_test.result = action->comm.src_proc && action->comm.dst_proc;
    if(req->comm_test.result){
      action->state = SIMIX_DONE;
      xbt_fifo_push(action->request_list, req);
      SIMIX_comm_finish(action);
    }else{
      SIMIX_request_answer(req);
    }
    return;
  }

  req->comm_test.result = (action->state != SIMIX_WAITING && action->state != SIMIX_RUNNING);
  if (req->comm_test.result) {
    xbt_fifo_push(action->request_list, req);
    SIMIX_comm_finish(action);
  } else {
    SIMIX_request_answer(req);
  }
}

void SIMIX_pre_comm_testany(smx_req_t req, int idx)
{
  unsigned int cursor;
  smx_action_t action;
  xbt_dynar_t actions = req->comm_testany.comms;
  req->comm_testany.result = -1;

  if (MC_IS_ENABLED){
    if(idx == -1){
      SIMIX_request_answer(req);
    }else{
      action = xbt_dynar_get_as(actions, idx, smx_action_t);
      req->comm_testany.result = idx;
      xbt_fifo_push(action->request_list, req);
      action->state = SIMIX_DONE;
      SIMIX_comm_finish(action);
    }
    return;
  }

  xbt_dynar_foreach(req->comm_testany.comms,cursor,action) {
    if (action->state != SIMIX_WAITING && action->state != SIMIX_RUNNING) {
      req->comm_testany.result = cursor;
      xbt_fifo_push(action->request_list, req);
      SIMIX_comm_finish(action);
      return;
    }
  }
  SIMIX_request_answer(req);
}

void SIMIX_pre_comm_waitany(smx_req_t req, int idx)
{
  smx_action_t action;
  unsigned int cursor = 0;
  xbt_dynar_t actions = req->comm_waitany.comms;

  if (MC_IS_ENABLED){
    action = xbt_dynar_get_as(actions, idx, smx_action_t);
    xbt_fifo_push(action->request_list, req);
    req->comm_waitany.result = idx;
    action->state = SIMIX_DONE;
    SIMIX_comm_finish(action);
    return;
  }

  xbt_dynar_foreach(actions, cursor, action){
    /* associate this request to the the action */
    xbt_fifo_push(action->request_list, req);

    /* see if the action is already finished */
    if (action->state != SIMIX_WAITING && action->state != SIMIX_RUNNING){
      SIMIX_comm_finish(action);
      break;
    }
  }
}

void SIMIX_waitany_req_remove_from_actions(smx_req_t req)
{
  smx_action_t action;
  unsigned int cursor = 0;
  xbt_dynar_t actions = req->comm_waitany.comms;

  xbt_dynar_foreach(actions, cursor, action){
    xbt_fifo_remove(action->request_list, req);
  }
}

/**
 *  \brief Start the simulation of a communication request
 *  \param action The communication action
 */

XBT_INLINE void SIMIX_comm_start(smx_action_t action)
{
  /* If both the sender and the receiver are already there, start the communication */
  if (action->state == SIMIX_READY) {

    smx_host_t sender = action->comm.src_proc->smx_host;
    smx_host_t receiver = action->comm.dst_proc->smx_host;

    XBT_DEBUG("Starting communication %p from '%s' to '%s'", action,
           SIMIX_host_get_name(sender), SIMIX_host_get_name(receiver));

    action->comm.surf_comm = surf_workstation_model->extension.workstation.
        communicate(sender->host, receiver->host, action->comm.task_size, action->comm.rate);

    surf_workstation_model->action_data_set(action->comm.surf_comm, action);

    action->state = SIMIX_RUNNING;

    /* If a link is failed, detect it immediately */
    if (surf_workstation_model->action_state_get(action->comm.surf_comm) == SURF_ACTION_FAILED) {
      XBT_DEBUG("Communication from '%s' to '%s' failed to start because of a link failure",
	  SIMIX_host_get_name(sender), SIMIX_host_get_name(receiver));
      action->state = SIMIX_LINK_FAILURE;
      SIMIX_comm_destroy_internal_actions(action);
    }

    /* If any of the process is suspend, create the action but stop its execution,
       it will be restarted when the sender process resume */
    if (SIMIX_process_is_suspended(action->comm.src_proc) ||
        SIMIX_process_is_suspended(action->comm.dst_proc)) {
      /* FIXME: check what should happen with the action state */
      surf_workstation_model->suspend(action->comm.surf_comm);
    }
  }
}

/**
 * \brief Answers the SIMIX requests associated to a communication action.
 * \param action a finished communication action
 */
void SIMIX_comm_finish(smx_action_t action)
{
  volatile unsigned int destroy_count = 0;
  smx_req_t req;

  while ((req = xbt_fifo_shift(action->request_list))) {

    /* If a waitany request is waiting for this action to finish, then remove
       it from the other actions in the waitany list. Afterwards, get the
       position of the actual action in the waitany request's actions dynar and
       return it as the result of the call */
    if (req->call == REQ_COMM_WAITANY) {
      SIMIX_waitany_req_remove_from_actions(req);
      if (!MC_IS_ENABLED)
        req->comm_waitany.result = xbt_dynar_search(req->comm_waitany.comms, &action);
    }

    /* If the action is still in a rendez-vous point then remove from it */
    if (action->comm.rdv)
      SIMIX_rdv_remove(action->comm.rdv, action);

    XBT_DEBUG("SIMIX_comm_finish: action state = %d", action->state);

    /* Check out for errors */
    switch (action->state) {

      case SIMIX_DONE:
        XBT_DEBUG("Communication %p complete!", action);
        SIMIX_comm_copy_data(action);
        break;

      case SIMIX_SRC_TIMEOUT:
        TRY {
          THROWF(timeout_error, 0, "Communication timeouted because of sender");
        }
	CATCH(req->issuer->running_ctx->exception) {
          req->issuer->doexception = 1;
        }
        break;

      case SIMIX_DST_TIMEOUT:
        TRY {
          THROWF(timeout_error, 0, "Communication timeouted because of receiver");
        }
	CATCH(req->issuer->running_ctx->exception) {
          req->issuer->doexception = 1;
        }
        break;

      case SIMIX_SRC_HOST_FAILURE:
        TRY {
          if (req->issuer == action->comm.src_proc)
            THROWF(host_error, 0, "Host failed");
          else
            THROWF(network_error, 0, "Remote peer failed");
        }
	CATCH(req->issuer->running_ctx->exception) {
          req->issuer->doexception = 1;
        }
        break;

      case SIMIX_DST_HOST_FAILURE:
        TRY {
          if (req->issuer == action->comm.dst_proc)
            THROWF(host_error, 0, "Host failed");
          else
            THROWF(network_error, 0, "Remote peer failed");
        }
	CATCH(req->issuer->running_ctx->exception) {
          req->issuer->doexception = 1;
        }
        break;

      case SIMIX_LINK_FAILURE:
        TRY {
          XBT_DEBUG("Link failure in action %p between '%s' and '%s': posting an exception to the issuer: %s (%p) detached:%d",
              action,
              action->comm.src_proc ? action->comm.src_proc->smx_host->name : NULL,
              action->comm.dst_proc ? action->comm.dst_proc->smx_host->name : NULL,
              req->issuer->name, req->issuer, action->comm.detached);
          if (action->comm.src_proc == req->issuer) {
            XBT_DEBUG("I'm source");
          } else if (action->comm.dst_proc == req->issuer) {
            XBT_DEBUG("I'm dest");
          } else {
            XBT_DEBUG("I'm neither source nor dest");
          }
          THROWF(network_error, 0, "Link failure");
        }
	CATCH(req->issuer->running_ctx->exception) {
          req->issuer->doexception = 1;
        }
        break;

      case SIMIX_CANCELED:
        TRY {
          if (req->issuer == action->comm.dst_proc) {
            THROWF(cancel_error, 0, "Communication canceled by the sender");
          }
          else {
            THROWF(cancel_error, 0, "Communication canceled by the receiver");
          }
        }
        CATCH(req->issuer->running_ctx->exception) {
          req->issuer->doexception = 1;
        }
        break;

      default:
        xbt_die("Unexpected action state in SIMIX_comm_finish: %d", action->state);
    }

    /* if there is an exception during a waitany or a testany, indicate the position of the failed communication */
    if (req->issuer->doexception) {
      if (req->call == REQ_COMM_WAITANY) {
        req->issuer->running_ctx->exception.value = xbt_dynar_search(req->comm_waitany.comms, &action);
      }
      else if (req->call == REQ_COMM_TESTANY) {
        req->issuer->running_ctx->exception.value = xbt_dynar_search(req->comm_testany.comms, &action);
      }
    }

    req->issuer->waiting_action = NULL;
    xbt_fifo_remove(req->issuer->comms, action);
    SIMIX_request_answer(req);
    destroy_count++;
  }

  while (destroy_count-- > 0)
    SIMIX_comm_destroy(action);
}

/**
 * \brief This function is called when a Surf communication action is finished.
 * \param action the corresponding Simix communication
 */
void SIMIX_post_comm(smx_action_t action)
{
  /* Update action state */
  if (action->comm.src_timeout &&
     surf_workstation_model->action_state_get(action->comm.src_timeout) == SURF_ACTION_DONE)
     action->state = SIMIX_SRC_TIMEOUT;
  else if (action->comm.dst_timeout &&
          surf_workstation_model->action_state_get(action->comm.dst_timeout) == SURF_ACTION_DONE)
     action->state = SIMIX_DST_TIMEOUT;
  else if (action->comm.src_timeout &&
          surf_workstation_model->action_state_get(action->comm.src_timeout) == SURF_ACTION_FAILED)
     action->state = SIMIX_SRC_HOST_FAILURE;
  else if (action->comm.dst_timeout &&
          surf_workstation_model->action_state_get(action->comm.dst_timeout) == SURF_ACTION_FAILED)
     action->state = SIMIX_DST_HOST_FAILURE;
  else if (action->comm.surf_comm &&
          surf_workstation_model->action_state_get(action->comm.surf_comm) == SURF_ACTION_FAILED) {
	  XBT_DEBUG("Puta madre. Surf says that the link broke");
     action->state = SIMIX_LINK_FAILURE;
  } else
    action->state = SIMIX_DONE;

  XBT_DEBUG("SIMIX_post_comm: comm %p, state %d, src_proc %p, dst_proc %p, detached: %d",
      action, action->state, action->comm.src_proc, action->comm.dst_proc, action->comm.detached);

  /* destroy the surf actions associated with the Simix communication */
  SIMIX_comm_destroy_internal_actions(action);

  /* remove the communication action from the list of pending communications
   * of both processes (if they still exist) */
  if (action->comm.src_proc) {
    xbt_fifo_remove(action->comm.src_proc->comms, action);
  }
  if (action->comm.dst_proc) {
    xbt_fifo_remove(action->comm.dst_proc->comms, action);
  }

  /* if there are requests associated with the action, then answer them */
  if (xbt_fifo_size(action->request_list)) {
    SIMIX_comm_finish(action);
  }
}

void SIMIX_comm_cancel(smx_action_t action)
{
  /* if the action is a waiting state means that it is still in a rdv */
  /* so remove from it and delete it */
  if (action->state == SIMIX_WAITING) {
    SIMIX_rdv_remove(action->comm.rdv, action);
    action->state = SIMIX_CANCELED;
  }
  else if (!MC_IS_ENABLED /* when running the MC there are no surf actions */
      && (action->state == SIMIX_READY || action->state == SIMIX_RUNNING)) {

    surf_workstation_model->action_cancel(action->comm.surf_comm);
  }
}

void SIMIX_comm_suspend(smx_action_t action)
{
  /*FIXME: shall we suspend also the timeout actions? */
  surf_workstation_model->suspend(action->comm.surf_comm);
}

void SIMIX_comm_resume(smx_action_t action)
{
  /*FIXME: check what happen with the timeouts */
  surf_workstation_model->resume(action->comm.surf_comm);
}


/************* Action Getters **************/

/**
 *  \brief get the amount remaining from the communication
 *  \param action The communication
 */
double SIMIX_comm_get_remains(smx_action_t action)
{
  double remains;

  if(!action){
      return 0;
  }

  switch (action->state) {

    case SIMIX_RUNNING:
      remains = surf_workstation_model->get_remains(action->comm.surf_comm);
      break;

    case SIMIX_WAITING:
    case SIMIX_READY:
      remains = 0; /*FIXME: check what should be returned */
      break;

    default:
      remains = 0; /*FIXME: is this correct? */
      break;
  }
  return remains;
}

e_smx_state_t SIMIX_comm_get_state(smx_action_t action)
{
  return action->state;
}

/**
 *  \brief Return the user data associated to the sender of the communication
 *  \param action The communication
 *  \return the user data
 */
void* SIMIX_comm_get_src_data(smx_action_t action)
{
  return action->comm.src_data;
}

/**
 *  \brief Return the user data associated to the receiver of the communication
 *  \param action The communication
 *  \return the user data
 */
void* SIMIX_comm_get_dst_data(smx_action_t action)
{
  return action->comm.dst_data;
}

smx_process_t SIMIX_comm_get_src_proc(smx_action_t action)
{
  return action->comm.src_proc;
}

smx_process_t SIMIX_comm_get_dst_proc(smx_action_t action)
{
  return action->comm.dst_proc;
}

#ifdef HAVE_LATENCY_BOUND_TRACKING
/**
 *  \brief verify if communication is latency bounded
 *  \param comm The communication
 */
XBT_INLINE int SIMIX_comm_is_latency_bounded(smx_action_t action)
{
  if(!action){
      return 0;
  }
  if (action->comm.surf_comm){
      XBT_DEBUG("Getting latency limited for surf_action (%p)", action->comm.surf_comm);
      action->latency_limited = surf_workstation_model->get_latency_limited(action->comm.surf_comm);
      XBT_DEBUG("Action limited is %d", action->latency_limited);
  }
  return action->latency_limited;
}
#endif

/******************************************************************************/
/*                    SIMIX_comm_copy_data callbacks                       */
/******************************************************************************/
static void (*SIMIX_comm_copy_data_callback) (smx_action_t, void*, size_t) =
    &SIMIX_comm_copy_pointer_callback;

void
SIMIX_comm_set_copy_data_callback(void (*callback) (smx_action_t, void*, size_t))
{
  SIMIX_comm_copy_data_callback = callback;
}

void SIMIX_comm_copy_pointer_callback(smx_action_t comm, void* buff, size_t buff_size)
{
  xbt_assert((buff_size == sizeof(void *)),
              "Cannot copy %zu bytes: must be sizeof(void*)", buff_size);
  *(void **) (comm->comm.dst_buff) = buff;
}

void SIMIX_comm_copy_buffer_callback(smx_action_t comm, void* buff, size_t buff_size)
{
  XBT_DEBUG("Copy the data over");
  memcpy(comm->comm.dst_buff, buff, buff_size);
}

void smpi_comm_copy_data_callback(smx_action_t comm, void* buff, size_t buff_size)
{
  XBT_DEBUG("Copy the data over");
  memcpy(comm->comm.dst_buff, buff, buff_size);
  if (comm->comm.detached) { // if this is a detached send, the source buffer was duplicated by SMPI sender to make the original buffer available to the application ASAP
    xbt_free(buff);
    comm->comm.src_buff = NULL;
  }
}

/**
 *  \brief Copy the communication data from the sender's buffer to the receiver's one
 *  \param comm The communication
 */
void SIMIX_comm_copy_data(smx_action_t comm)
{
  size_t buff_size = comm->comm.src_buff_size;
  /* If there is no data to be copy then return */
  if (!comm->comm.src_buff || !comm->comm.dst_buff || comm->comm.copied == 1)
    return;

  XBT_DEBUG("Copying comm %p data from %s (%p) -> %s (%p) (%zu bytes)",
         comm,
         comm->comm.src_proc ? comm->comm.src_proc->smx_host->name : "a finished process",
         comm->comm.src_buff,
         comm->comm.dst_proc ? comm->comm.dst_proc->smx_host->name : "a finished process",
         comm->comm.dst_buff, buff_size);

  /* Copy at most dst_buff_size bytes of the message to receiver's buffer */
  if (comm->comm.dst_buff_size)
    buff_size = MIN(buff_size, *(comm->comm.dst_buff_size));

  /* Update the receiver's buffer size to the copied amount */
  if (comm->comm.dst_buff_size)
    *comm->comm.dst_buff_size = buff_size;

  if (buff_size > 0)
    SIMIX_comm_copy_data_callback (comm, comm->comm.src_buff, buff_size);

  /* Set the copied flag so we copy data only once */
  /* (this function might be called from both communication ends) */
  comm->comm.copied = 1;
}
