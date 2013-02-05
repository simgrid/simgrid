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
XBT_PUBLIC(unsigned long int) smx_total_comms = 0;

static void SIMIX_waitany_remove_simcall_from_actions(smx_simcall_t simcall);
static void SIMIX_comm_copy_data(smx_action_t comm);
static smx_action_t SIMIX_comm_new(e_smx_comm_type_t type);
static XBT_INLINE void SIMIX_rdv_push(smx_rdv_t rdv, smx_action_t comm);
static smx_action_t SIMIX_fifo_probe_comm(xbt_fifo_t fifo, e_smx_comm_type_t type,
                                        int (*match_fun)(void *, void *,smx_action_t),
                                        void *user_data, smx_action_t my_action);
static smx_action_t SIMIX_fifo_get_comm(xbt_fifo_t fifo, e_smx_comm_type_t type,
                                        int (*match_fun)(void *, void *,smx_action_t),
                                        void *user_data, smx_action_t my_action);
static void SIMIX_rdv_free(void *data);

void SIMIX_network_init(void)
{
  rdv_points = xbt_dict_new_homogeneous(SIMIX_rdv_free);
  if(MC_is_active())
    MC_ignore_data_bss(&smx_total_comms, sizeof(smx_total_comms));
}

void SIMIX_network_exit(void)
{
  xbt_dict_free(&rdv_points);
}

/******************************************************************************/
/*                           Rendez-Vous Points                               */
/******************************************************************************/

smx_rdv_t SIMIX_pre_rdv_create(smx_simcall_t simcall, const char *name){
  return SIMIX_rdv_create(name);
}
smx_rdv_t SIMIX_rdv_create(const char *name)
{
  /* two processes may have pushed the same rdv_create simcall at the same time */
  smx_rdv_t rdv = name ? xbt_dict_get_or_null(rdv_points, name) : NULL;

  if (!rdv) {
    rdv = xbt_new0(s_smx_rvpoint_t, 1);
    rdv->name = name ? xbt_strdup(name) : NULL;
    rdv->comm_fifo = xbt_fifo_new();
    rdv->done_comm_fifo = xbt_fifo_new();
    rdv->permanent_receiver=NULL;

    XBT_DEBUG("Creating a mailbox at %p with name %s\n", rdv, name);

    if (rdv->name)
      xbt_dict_set(rdv_points, rdv->name, rdv, NULL);
  }
  return rdv;
}

void SIMIX_pre_rdv_destroy(smx_simcall_t simcall, smx_rdv_t rdv){
  return SIMIX_rdv_destroy(rdv);
}
void SIMIX_rdv_destroy(smx_rdv_t rdv)
{
  if (rdv->name)
    xbt_dict_remove(rdv_points, rdv->name);
}

void SIMIX_rdv_free(void *data)
{
  XBT_DEBUG("rdv free %p", data);
  smx_rdv_t rdv = (smx_rdv_t) data;
  xbt_free(rdv->name);
  xbt_fifo_free(rdv->comm_fifo);
  xbt_fifo_free(rdv->done_comm_fifo);

  xbt_free(rdv);  
}

xbt_dict_t SIMIX_get_rdv_points()
{
  return rdv_points;
}

smx_rdv_t SIMIX_pre_rdv_get_by_name(smx_simcall_t simcall, const char *name){
  return SIMIX_rdv_get_by_name(name);
}
smx_rdv_t SIMIX_rdv_get_by_name(const char *name)
{
  return xbt_dict_get_or_null(rdv_points, name);
}

int SIMIX_pre_rdv_comm_count_by_host(smx_simcall_t simcall, smx_rdv_t rdv, smx_host_t host){
  return SIMIX_rdv_comm_count_by_host(rdv, host);
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

smx_action_t SIMIX_pre_rdv_get_head(smx_simcall_t simcall, smx_rdv_t rdv){
  return SIMIX_rdv_get_head(rdv);
}
smx_action_t SIMIX_rdv_get_head(smx_rdv_t rdv)
{
  return xbt_fifo_get_item_content(xbt_fifo_get_first_item(rdv->comm_fifo));
}

smx_process_t SIMIX_pre_rdv_get_receiver(smx_simcall_t simcall, smx_rdv_t rdv){
  return SIMIX_rdv_get_receiver(rdv);
}
/**
 *  \brief get the receiver (process associated to the mailbox)
 *  \param rdv The rendez-vous point
 *  \return process The receiving process (NULL if not set)
 */
smx_process_t SIMIX_rdv_get_receiver(smx_rdv_t rdv)
{
  return rdv->permanent_receiver;
}

void SIMIX_pre_rdv_set_receiver(smx_simcall_t simcall, smx_rdv_t rdv,
		            smx_process_t process){
  SIMIX_rdv_set_receiver(rdv, process);
}
/**
 *  \brief set the receiver of the rendez vous point to allow eager sends
 *  \param rdv The rendez-vous point
 *  \param process The receiving process
 */
void SIMIX_rdv_set_receiver(smx_rdv_t rdv, smx_process_t process)
{
  rdv->permanent_receiver=process;
}

/**
 *  \brief Pushes a communication action into a rendez-vous point
 *  \param rdv The rendez-vous point
 *  \param comm The communication action
 */
static XBT_INLINE void SIMIX_rdv_push(smx_rdv_t rdv, smx_action_t comm)
{
  xbt_fifo_push(rdv->comm_fifo, comm);
  comm->comm.rdv = rdv;
}

/**
 *  \brief Removes a communication action from a rendez-vous point
 *  \param rdv The rendez-vous point
 *  \param comm The communication action
 */
XBT_INLINE void SIMIX_rdv_remove(smx_rdv_t rdv, smx_action_t comm)
{
  xbt_fifo_remove(rdv->comm_fifo, comm);
  comm->comm.rdv = NULL;
}

/**
 *  \brief Checks if there is a communication action queued in a fifo matching our needs
 *  \param type The type of communication we are looking for (comm_send, comm_recv)
 *  \return The communication action if found, NULL otherwise
 */
smx_action_t SIMIX_fifo_get_comm(xbt_fifo_t fifo, e_smx_comm_type_t type,
                                 int (*match_fun)(void *, void *,smx_action_t),
                                 void *this_user_data, smx_action_t my_action)
{
  smx_action_t action;
  xbt_fifo_item_t item;
  void* other_user_data = NULL;

  xbt_fifo_foreach(fifo, item, action, smx_action_t) {
    if (action->comm.type == SIMIX_COMM_SEND) {
      other_user_data = action->comm.src_data;
    } else if (action->comm.type == SIMIX_COMM_RECEIVE) {
      other_user_data = action->comm.dst_data;
    }
    if (action->comm.type == type &&
        (!match_fun              ||              match_fun(this_user_data,  other_user_data, action)) &&
        (!action->comm.match_fun || action->comm.match_fun(other_user_data, this_user_data,  my_action))) {
      XBT_DEBUG("Found a matching communication action %p", action);
      xbt_fifo_remove_item(fifo, item);
      xbt_fifo_free_item(item);
      action->comm.refcount++;
      action->comm.rdv = NULL;
      return action;
    }
    XBT_DEBUG("Sorry, communication action %p does not match our needs:"
              " its type is %d but we are looking for a comm of type %d (or maybe the filtering didn't match)",
              action, (int)action->comm.type, (int)type);
  }
  XBT_DEBUG("No matching communication action found");
  return NULL;
}


/**
 *  \brief Checks if there is a communication action queued in a fifo matching our needs, but leave it there
 *  \param type The type of communication we are looking for (comm_send, comm_recv)
 *  \return The communication action if found, NULL otherwise
 */
smx_action_t SIMIX_fifo_probe_comm(xbt_fifo_t fifo, e_smx_comm_type_t type,
                                 int (*match_fun)(void *, void *,smx_action_t),
                                 void *this_user_data, smx_action_t my_action)
{
  smx_action_t action;
  xbt_fifo_item_t item;
  void* other_user_data = NULL;

  xbt_fifo_foreach(fifo, item, action, smx_action_t) {
    if (action->comm.type == SIMIX_COMM_SEND) {
      other_user_data = action->comm.src_data;
    } else if (action->comm.type == SIMIX_COMM_RECEIVE) {
      other_user_data = action->comm.dst_data;
    }
    if (action->comm.type == type &&
        (!match_fun              ||              match_fun(this_user_data,  other_user_data, action)) &&
        (!action->comm.match_fun || action->comm.match_fun(other_user_data, this_user_data,  my_action))) {
      XBT_DEBUG("Found a matching communication action %p", action);
      action->comm.refcount++;

      return action;
    }
    XBT_DEBUG("Sorry, communication action %p does not match our needs:"
              " its type is %d but we are looking for a comm of type %d (or maybe the filtering didn't match)",
              action, (int)action->comm.type, (int)type);
  }
  XBT_DEBUG("No matching communication action found");
  return NULL;
}
/******************************************************************************/
/*                            Communication Actions                            */
/******************************************************************************/

/**
 *  \brief Creates a new communicate action
 *  \param type The direction of communication (comm_send, comm_recv)
 *  \return The new communicate action
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
  act->comm.src_data=NULL;
  act->comm.dst_data=NULL;


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

void SIMIX_pre_comm_destroy(smx_simcall_t simcall, smx_action_t action){
  SIMIX_comm_destroy(action);
}
/**
 *  \brief Destroy a communicate action
 *  \param action The communicate action to be destroyed
 */
void SIMIX_comm_destroy(smx_action_t action)
{
  XBT_DEBUG("Destroy action %p (refcount: %d), state: %d",
            action, action->comm.refcount, (int)action->state);

  if (action->comm.refcount <= 0) {
    xbt_backtrace_display_current();
    xbt_die("The refcount of comm %p is already 0 before decreasing it. "
            "That's a bug! If you didn't test and/or wait the same communication twice in your code, then the bug is SimGrid's...", action);
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

  if(action->comm.rdv)
    SIMIX_rdv_remove(action->comm.rdv, action);

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

void SIMIX_pre_comm_send(smx_simcall_t simcall, smx_rdv_t rdv,
                                  double task_size, double rate,
                                  void *src_buff, size_t src_buff_size,
                                  int (*match_fun)(void *, void *,smx_action_t),
				  void *data, double timeout){
  smx_action_t comm = SIMIX_comm_isend(simcall->issuer, rdv, task_size, rate,
		                       src_buff, src_buff_size, match_fun, NULL,
				       data, 0);
  simcall->mc_value = 0;
  SIMIX_pre_comm_wait(simcall, comm, timeout);
}
smx_action_t SIMIX_pre_comm_isend(smx_simcall_t simcall, smx_rdv_t rdv,
                                  double task_size, double rate,
                                  void *src_buff, size_t src_buff_size,
                                  int (*match_fun)(void *, void *,smx_action_t),
                                  void (*clean_fun)(void *), 
				  void *data, int detached){
  return SIMIX_comm_isend(simcall->issuer, rdv, task_size, rate, src_buff,
		          src_buff_size, match_fun, clean_fun, data, detached);

}
smx_action_t SIMIX_comm_isend(smx_process_t src_proc, smx_rdv_t rdv,
                              double task_size, double rate,
                              void *src_buff, size_t src_buff_size,
                              int (*match_fun)(void *, void *,smx_action_t),
                              void (*clean_fun)(void *), // used to free the action in case of problem after a detached send
                              void *data,
                              int detached)
{
  XBT_DEBUG("send from %p\n", rdv);

  /* Prepare an action describing us, so that it gets passed to the user-provided filter of other side */
  smx_action_t this_action = SIMIX_comm_new(SIMIX_COMM_SEND);

  /* Look for communication action matching our needs. We also provide a description of
   * ourself so that the other side also gets a chance of choosing if it wants to match with us.
   *
   * If it is not found then push our communication into the rendez-vous point */
  smx_action_t other_action = SIMIX_fifo_get_comm(rdv->comm_fifo, SIMIX_COMM_RECEIVE, match_fun, data, this_action);

  if (!other_action) {
    other_action = this_action;

    if (rdv->permanent_receiver!=NULL){
      //this mailbox is for small messages, which have to be sent right now
      other_action->state = SIMIX_READY;
      other_action->comm.dst_proc=rdv->permanent_receiver;
      other_action->comm.refcount++;
      other_action->comm.rdv = rdv;
      xbt_fifo_push(rdv->done_comm_fifo,other_action);
      other_action->comm.rdv=rdv;
      XBT_DEBUG("pushing a message into the permanent receive fifo %p, comm %p \n", rdv, &(other_action->comm));

    }else{
      SIMIX_rdv_push(rdv, this_action);
    }
  } else {
    XBT_DEBUG("Receive already pushed\n");

    SIMIX_comm_destroy(this_action);
    --smx_total_comms; // this creation was a pure waste

    other_action->state = SIMIX_READY;
    other_action->comm.type = SIMIX_COMM_READY;

  }
  xbt_fifo_push(src_proc->comms, other_action);

  /* if the communication action is detached then decrease the refcount
   * by one, so it will be eliminated by the receiver's destroy call */
  if (detached) {
    other_action->comm.detached = 1;
    other_action->comm.refcount--;
    other_action->comm.clean_fun = clean_fun;
  } else {
    other_action->comm.clean_fun = NULL;
  }

  /* Setup the communication action */
  other_action->comm.src_proc = src_proc;
  other_action->comm.task_size = task_size;
  other_action->comm.rate = rate;
  other_action->comm.src_buff = src_buff;
  other_action->comm.src_buff_size = src_buff_size;
  other_action->comm.src_data = data;

  other_action->comm.match_fun = match_fun;

  if (MC_is_active()) {
    other_action->state = SIMIX_RUNNING;
    return other_action;
  }

  SIMIX_comm_start(other_action);
  return (detached ? NULL : other_action);
}

void SIMIX_pre_comm_recv(smx_simcall_t simcall, smx_rdv_t rdv,
                                  void *dst_buff, size_t *dst_buff_size,
                                  int (*match_fun)(void *, void *, smx_action_t),
				  void *data, double timeout){
  smx_action_t comm = SIMIX_comm_irecv(simcall->issuer, rdv, dst_buff,
		                       dst_buff_size, match_fun, data);
  simcall->mc_value = 0;
  SIMIX_pre_comm_wait(simcall, comm, timeout);
}
smx_action_t SIMIX_pre_comm_irecv(smx_simcall_t simcall, smx_rdv_t rdv,
                                  void *dst_buff, size_t *dst_buff_size,
                                  int (*match_fun)(void *, void *, smx_action_t),
				  void *data){
  return SIMIX_comm_irecv(simcall->issuer, rdv, dst_buff, dst_buff_size,
		          match_fun, data);
}
smx_action_t SIMIX_comm_irecv(smx_process_t dst_proc, smx_rdv_t rdv,
                              void *dst_buff, size_t *dst_buff_size,
                              int (*match_fun)(void *, void *, smx_action_t), void *data)
{
  XBT_DEBUG("recv from %p %p\n", rdv, rdv->comm_fifo);
  smx_action_t this_action = SIMIX_comm_new(SIMIX_COMM_RECEIVE);

  smx_action_t other_action;
  //communication already done, get it inside the fifo of completed comms
  //permanent receive v1
  //int already_received=0;
  if(rdv->permanent_receiver && xbt_fifo_size(rdv->done_comm_fifo)!=0){

    XBT_DEBUG("We have a comm that has probably already been received, trying to match it, to skip the communication\n");
    //find a match in the already received fifo
    other_action = SIMIX_fifo_get_comm(rdv->done_comm_fifo, SIMIX_COMM_SEND, match_fun, data, this_action);
    //if not found, assume the receiver came first, register it to the mailbox in the classical way
    if (!other_action)  {
      XBT_DEBUG("We have messages in the permanent receive list, but not the one we are looking for, pushing request into fifo\n");
      other_action = this_action;
      SIMIX_rdv_push(rdv, this_action);
    }else{
      if(other_action->comm.surf_comm && 	SIMIX_comm_get_remains(other_action)==0.0)
      {
        XBT_DEBUG("comm %p has been already sent, and is finished, destroy it\n",&(other_action->comm));
        other_action->state = SIMIX_DONE;
        other_action->comm.type = SIMIX_COMM_DONE;
        other_action->comm.rdv = NULL;
        //SIMIX_comm_destroy(this_action);
        //--smx_total_comms; // this creation was a pure waste
        //already_received=1;
        //other_action->comm.refcount--;
      }/*else{
         XBT_DEBUG("Not yet finished, we have to wait %d\n", xbt_fifo_size(rdv->comm_fifo));
         }*/
      other_action->comm.refcount--;
      SIMIX_comm_destroy(this_action);
      --smx_total_comms; // this creation was a pure waste
    }
  }else{
    /* Prepare an action describing us, so that it gets passed to the user-provided filter of other side */

    /* Look for communication action matching our needs. We also provide a description of
     * ourself so that the other side also gets a chance of choosing if it wants to match with us.
     *
     * If it is not found then push our communication into the rendez-vous point */
    other_action = SIMIX_fifo_get_comm(rdv->comm_fifo, SIMIX_COMM_SEND, match_fun, data, this_action);

    if (!other_action) {
      XBT_DEBUG("Receive pushed first %d\n", xbt_fifo_size(rdv->comm_fifo));
      other_action = this_action;
      SIMIX_rdv_push(rdv, this_action);
    } else {
      SIMIX_comm_destroy(this_action);
      --smx_total_comms; // this creation was a pure waste
      other_action->state = SIMIX_READY;
      other_action->comm.type = SIMIX_COMM_READY;
      //other_action->comm.refcount--;
    }
    xbt_fifo_push(dst_proc->comms, other_action);
  }

  /* Setup communication action */
  other_action->comm.dst_proc = dst_proc;
  other_action->comm.dst_buff = dst_buff;
  other_action->comm.dst_buff_size = dst_buff_size;
  other_action->comm.dst_data = data;

  other_action->comm.match_fun = match_fun;


  /*if(already_received)//do the actual copy, because the first one after the comm didn't have all the info
    SIMIX_comm_copy_data(other_action);*/


  if (MC_is_active()) {
    other_action->state = SIMIX_RUNNING;
    return other_action;
  }

  SIMIX_comm_start(other_action);
  // }
  return other_action;
}

smx_action_t SIMIX_pre_comm_iprobe(smx_simcall_t simcall, smx_rdv_t rdv,
                                   int src, int tag,
                                   int (*match_fun)(void *, void *, smx_action_t),
                                   void *data){
  return SIMIX_comm_iprobe(simcall->issuer, rdv, src, tag, match_fun, data);
}

smx_action_t SIMIX_comm_iprobe(smx_process_t dst_proc, smx_rdv_t rdv, int src,
                              int tag, int (*match_fun)(void *, void *, smx_action_t), void *data)
{
  XBT_DEBUG("iprobe from %p %p\n", rdv, rdv->comm_fifo);
  smx_action_t this_action = SIMIX_comm_new(SIMIX_COMM_RECEIVE);

  smx_action_t other_action=NULL;
  if(rdv->permanent_receiver && xbt_fifo_size(rdv->done_comm_fifo)!=0){
    //find a match in the already received fifo
      XBT_DEBUG("first try in the perm recv mailbox \n");

    other_action = SIMIX_fifo_probe_comm(rdv->done_comm_fifo, SIMIX_COMM_SEND, match_fun, data, this_action);
  }
 // }else{
    if(!other_action){
        XBT_DEBUG("second try in the other mailbox");
        other_action = SIMIX_fifo_probe_comm(rdv->comm_fifo, SIMIX_COMM_SEND, match_fun, data, this_action);
    }
//  }
  if(other_action)other_action->comm.refcount--;

  SIMIX_comm_destroy(this_action);
  --smx_total_comms;
  return other_action;
}

void SIMIX_pre_comm_wait(smx_simcall_t simcall, smx_action_t action, double timeout)
{
  int idx = simcall->mc_value;
  /* the simcall may be a wait, a send or a recv */
  surf_action_t sleep;

  /* Associate this simcall to the wait action */
  XBT_DEBUG("SIMIX_pre_comm_wait, %p", action);

  xbt_fifo_push(action->simcalls, simcall);
  simcall->issuer->waiting_action = action;

  if (MC_is_active()) {
    if (idx == 0) {
      action->state = SIMIX_DONE;
    } else {
      /* If we reached this point, the wait simcall must have a timeout */
      /* Otherwise it shouldn't be enabled and executed by the MC */
      if (timeout == -1)
        THROW_IMPOSSIBLE;

      if (action->comm.src_proc == simcall->issuer)
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
    sleep = surf_workstation_model->extension.workstation.sleep(simcall->issuer->smx_host, timeout);
    surf_workstation_model->action_data_set(sleep, action);

    if (simcall->issuer == action->comm.src_proc)
      action->comm.src_timeout = sleep;
    else
      action->comm.dst_timeout = sleep;
  }
}

void SIMIX_pre_comm_test(smx_simcall_t simcall, smx_action_t action)
{
  if(MC_is_active()){
    simcall_comm_test__set__result(simcall, action->comm.src_proc && action->comm.dst_proc);
    if(simcall_comm_test__get__result(simcall)){
      action->state = SIMIX_DONE;
      xbt_fifo_push(action->simcalls, simcall);
      SIMIX_comm_finish(action);
    }else{
      SIMIX_simcall_answer(simcall);
    }
    return;
  }

  simcall_comm_test__set__result(simcall, (action->state != SIMIX_WAITING && action->state != SIMIX_RUNNING));
  if (simcall_comm_test__get__result(simcall)) {
    xbt_fifo_push(action->simcalls, simcall);
    SIMIX_comm_finish(action);
  } else {
    SIMIX_simcall_answer(simcall);
  }
}

void SIMIX_pre_comm_testany(smx_simcall_t simcall, xbt_dynar_t actions)
{
  int idx = simcall->mc_value;
  unsigned int cursor;
  smx_action_t action;
  simcall_comm_testany__set__result(simcall, -1);

  if (MC_is_active()){
    if(idx == -1){
      SIMIX_simcall_answer(simcall);
    }else{
      action = xbt_dynar_get_as(actions, idx, smx_action_t);
      simcall_comm_testany__set__result(simcall, idx);
      xbt_fifo_push(action->simcalls, simcall);
      action->state = SIMIX_DONE;
      SIMIX_comm_finish(action);
    }
    return;
  }

  xbt_dynar_foreach(simcall_comm_testany__get__comms(simcall), cursor,action) {
    if (action->state != SIMIX_WAITING && action->state != SIMIX_RUNNING) {
      simcall_comm_testany__set__result(simcall, cursor);
      xbt_fifo_push(action->simcalls, simcall);
      SIMIX_comm_finish(action);
      return;
    }
  }
  SIMIX_simcall_answer(simcall);
}

void SIMIX_pre_comm_waitany(smx_simcall_t simcall, xbt_dynar_t actions)
{
  int idx = simcall->mc_value;
  smx_action_t action;
  unsigned int cursor = 0;

  if (MC_is_active()){
    action = xbt_dynar_get_as(actions, idx, smx_action_t);
    xbt_fifo_push(action->simcalls, simcall);
    simcall_comm_waitany__set__result(simcall, idx);
    action->state = SIMIX_DONE;
    SIMIX_comm_finish(action);
    return;
  }

  xbt_dynar_foreach(actions, cursor, action){
    /* associate this simcall to the the action */
    xbt_fifo_push(action->simcalls, simcall);

    /* see if the action is already finished */
    if (action->state != SIMIX_WAITING && action->state != SIMIX_RUNNING){
      SIMIX_comm_finish(action);
      break;
    }
  }
}

void SIMIX_waitany_remove_simcall_from_actions(smx_simcall_t simcall)
{
  smx_action_t action;
  unsigned int cursor = 0;
  xbt_dynar_t actions = simcall_comm_waitany__get__comms(simcall);

  xbt_dynar_foreach(actions, cursor, action) {
    xbt_fifo_remove(action->simcalls, simcall);
  }
}

/**
 *  \brief Starts the simulation of a communication action.
 *  \param action the communication action
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
      communicate(sender, receiver, action->comm.task_size, action->comm.rate);

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

      if (SIMIX_process_is_suspended(action->comm.src_proc))
        XBT_DEBUG("The communication is suspended on startup because src (%s:%s) were suspended since it initiated the communication",
                  SIMIX_host_get_name(action->comm.src_proc->smx_host), action->comm.src_proc->name);
      else
        XBT_DEBUG("The communication is suspended on startup because dst (%s:%s) were suspended since it initiated the communication",
                  SIMIX_host_get_name(action->comm.dst_proc->smx_host), action->comm.dst_proc->name);

      surf_workstation_model->suspend(action->comm.surf_comm);

    }
  }
}

/**
 * \brief Answers the SIMIX simcalls associated to a communication action.
 * \param action a finished communication action
 */
void SIMIX_comm_finish(smx_action_t action)
{
  unsigned int destroy_count = 0;
  smx_simcall_t simcall;

  while ((simcall = xbt_fifo_shift(action->simcalls))) {

    /* If a waitany simcall is waiting for this action to finish, then remove
       it from the other actions in the waitany list. Afterwards, get the
       position of the actual action in the waitany dynar and
       return it as the result of the simcall */
    if (simcall->call == SIMCALL_COMM_WAITANY) {
      SIMIX_waitany_remove_simcall_from_actions(simcall);
      if (!MC_is_active())
        simcall_comm_waitany__set__result(simcall, xbt_dynar_search(simcall_comm_waitany__get__comms(simcall), &action));
    }

    /* If the action is still in a rendez-vous point then remove from it */
    if (action->comm.rdv)
      SIMIX_rdv_remove(action->comm.rdv, action);

    XBT_DEBUG("SIMIX_comm_finish: action state = %d", (int)action->state);

    /* Check out for errors */
    switch (action->state) {

    case SIMIX_DONE:
      XBT_DEBUG("Communication %p complete!", action);
      SIMIX_comm_copy_data(action);
      break;

    case SIMIX_SRC_TIMEOUT:
      SMX_EXCEPTION(simcall->issuer, timeout_error, 0,
                    "Communication timeouted because of sender");
      break;

    case SIMIX_DST_TIMEOUT:
      SMX_EXCEPTION(simcall->issuer, timeout_error, 0,
                    "Communication timeouted because of receiver");
      break;

    case SIMIX_SRC_HOST_FAILURE:
      if (simcall->issuer == action->comm.src_proc)
        simcall->issuer->context->iwannadie = 1;
//          SMX_EXCEPTION(simcall->issuer, host_error, 0, "Host failed");
      else
        SMX_EXCEPTION(simcall->issuer, network_error, 0, "Remote peer failed");
      break;

    case SIMIX_DST_HOST_FAILURE:
      if (simcall->issuer == action->comm.dst_proc)
        simcall->issuer->context->iwannadie = 1;
//          SMX_EXCEPTION(simcall->issuer, host_error, 0, "Host failed");
      else
        SMX_EXCEPTION(simcall->issuer, network_error, 0, "Remote peer failed");
      break;

    case SIMIX_LINK_FAILURE:
      XBT_DEBUG("Link failure in action %p between '%s' and '%s': posting an exception to the issuer: %s (%p) detached:%d",
                action,
                action->comm.src_proc ? sg_host_name(action->comm.src_proc->smx_host) : NULL,
                action->comm.dst_proc ? sg_host_name(action->comm.dst_proc->smx_host) : NULL,
                simcall->issuer->name, simcall->issuer, action->comm.detached);
      if (action->comm.src_proc == simcall->issuer) {
        XBT_DEBUG("I'm source");
      } else if (action->comm.dst_proc == simcall->issuer) {
        XBT_DEBUG("I'm dest");
      } else {
        XBT_DEBUG("I'm neither source nor dest");
      }
      SMX_EXCEPTION(simcall->issuer, network_error, 0, "Link failure");
      break;

    case SIMIX_CANCELED:
      if (simcall->issuer == action->comm.dst_proc)
        SMX_EXCEPTION(simcall->issuer, cancel_error, 0,
                      "Communication canceled by the sender");
      else
        SMX_EXCEPTION(simcall->issuer, cancel_error, 0,
                      "Communication canceled by the receiver");
      break;

    default:
      xbt_die("Unexpected action state in SIMIX_comm_finish: %d", (int)action->state);
    }

    /* if there is an exception during a waitany or a testany, indicate the position of the failed communication */
    if (simcall->issuer->doexception) {
      if (simcall->call == SIMCALL_COMM_WAITANY) {
        simcall->issuer->running_ctx->exception.value = xbt_dynar_search(simcall_comm_waitany__get__comms(simcall), &action);
      }
      else if (simcall->call == SIMCALL_COMM_TESTANY) {
        simcall->issuer->running_ctx->exception.value = xbt_dynar_search(simcall_comm_testany__get__comms(simcall), &action);
      }
    }

    if (surf_workstation_model->extension.
        workstation.get_state(simcall->issuer->smx_host) != SURF_RESOURCE_ON) {
      simcall->issuer->context->iwannadie = 1;
    }

    simcall->issuer->waiting_action = NULL;
    xbt_fifo_remove(simcall->issuer->comms, action);
    if(action->comm.detached){
      if(simcall->issuer == action->comm.src_proc){
        if(action->comm.dst_proc)
          xbt_fifo_remove(action->comm.dst_proc->comms, action);
      }
      if(simcall->issuer == action->comm.dst_proc){
        if(action->comm.src_proc)
          xbt_fifo_remove(action->comm.src_proc->comms, action);
      }
    }
    SIMIX_simcall_answer(simcall);
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
            action, (int)action->state, action->comm.src_proc, action->comm.dst_proc, action->comm.detached);

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

  /* if there are simcalls associated with the action, then answer them */
  if (xbt_fifo_size(action->simcalls)) {
    SIMIX_comm_finish(action);
  }
}

void SIMIX_pre_comm_cancel(smx_simcall_t simcall, smx_action_t action){
  SIMIX_comm_cancel(action);
}
void SIMIX_comm_cancel(smx_action_t action)
{
  /* if the action is a waiting state means that it is still in a rdv */
  /* so remove from it and delete it */
  if (action->state == SIMIX_WAITING) {
    SIMIX_rdv_remove(action->comm.rdv, action);
    action->state = SIMIX_CANCELED;
  }
  else if (!MC_is_active() /* when running the MC there are no surf actions */
           && (action->state == SIMIX_READY || action->state == SIMIX_RUNNING)) {

    surf_workstation_model->action_cancel(action->comm.surf_comm);
  }
}

void SIMIX_comm_suspend(smx_action_t action)
{
  /*FIXME: shall we suspend also the timeout actions? */
  if (action->comm.surf_comm)
    surf_workstation_model->suspend(action->comm.surf_comm);
  /* in the other case, the action will be suspended on creation, in SIMIX_comm_start() */
}

void SIMIX_comm_resume(smx_action_t action)
{
  /*FIXME: check what happen with the timeouts */
  if (action->comm.surf_comm)
    surf_workstation_model->resume(action->comm.surf_comm);
  /* in the other case, the action were not really suspended yet, see SIMIX_comm_suspend() and SIMIX_comm_start() */
}


/************* Action Getters **************/

double SIMIX_pre_comm_get_remains(smx_simcall_t simcall, smx_action_t action){
  return SIMIX_comm_get_remains(action);
}
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

e_smx_state_t SIMIX_pre_comm_get_state(smx_simcall_t simcall, smx_action_t action){
  return SIMIX_comm_get_state(action);
}
e_smx_state_t SIMIX_comm_get_state(smx_action_t action)
{
  return action->state;
}

void* SIMIX_pre_comm_get_src_data(smx_simcall_t simcall, smx_action_t action){
  return SIMIX_comm_get_src_data(action);
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

void* SIMIX_pre_comm_get_dst_data(smx_simcall_t simcall, smx_action_t action){
  return SIMIX_comm_get_dst_data(action);
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

smx_process_t SIMIX_pre_comm_get_src_proc(smx_simcall_t simcall, smx_action_t action){
  return SIMIX_comm_get_src_proc(action);
}
smx_process_t SIMIX_comm_get_src_proc(smx_action_t action)
{
  return action->comm.src_proc;
}

smx_process_t SIMIX_pre_comm_get_dst_proc(smx_simcall_t simcall, smx_action_t action){
  return SIMIX_comm_get_dst_proc(action);
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
  if (!comm->comm.src_buff || !comm->comm.dst_buff || comm->comm.copied)
    return;

  XBT_DEBUG("Copying comm %p data from %s (%p) -> %s (%p) (%zu bytes)",
            comm,
            comm->comm.src_proc ? sg_host_name(comm->comm.src_proc->smx_host) : "a finished process",
            comm->comm.src_buff,
            comm->comm.dst_proc ? sg_host_name(comm->comm.dst_proc->smx_host) : "a finished process",
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
