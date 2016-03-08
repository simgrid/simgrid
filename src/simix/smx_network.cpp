/* Copyright (c) 2009-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/surf/surf_interface.hpp"
#include "src/simix/smx_private.h"
#include "xbt/log.h"
#include "mc/mc.h"
#include "src/mc/mc_replay.h"
#include "xbt/dict.h"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(simix_network, simix, "SIMIX network-related synchronization");

static xbt_dict_t rdv_points = NULL;
XBT_EXPORT_NO_IMPORT(unsigned long int) smx_total_comms = 0;

static void SIMIX_waitany_remove_simcall_from_actions(smx_simcall_t simcall);
static void SIMIX_comm_copy_data(smx_synchro_t comm);
static smx_synchro_t SIMIX_comm_new(e_smx_comm_type_t type);
static inline void SIMIX_rdv_push(smx_mailbox_t rdv, smx_synchro_t comm);
static smx_synchro_t SIMIX_fifo_probe_comm(xbt_fifo_t fifo, e_smx_comm_type_t type,
                                        int (*match_fun)(void *, void *,smx_synchro_t),
                                        void *user_data, smx_synchro_t my_synchro);
static smx_synchro_t SIMIX_fifo_get_comm(xbt_fifo_t fifo, e_smx_comm_type_t type,
                                        int (*match_fun)(void *, void *,smx_synchro_t),
                                        void *user_data, smx_synchro_t my_synchro);
static void SIMIX_rdv_free(void *data);
static void SIMIX_comm_start(smx_synchro_t synchro);

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

smx_mailbox_t SIMIX_rdv_create(const char *name)
{
  /* two processes may have pushed the same rdv_create simcall at the same time */
  smx_mailbox_t rdv = name ? (smx_mailbox_t) xbt_dict_get_or_null(rdv_points, name) : NULL;

  if (!rdv) {
    rdv = xbt_new0(s_smx_rvpoint_t, 1);
    rdv->name = name ? xbt_strdup(name) : NULL;
    rdv->comm_fifo = xbt_fifo_new();
    rdv->done_comm_fifo = xbt_fifo_new();
    rdv->permanent_receiver=NULL;

    XBT_DEBUG("Creating a mailbox at %p with name %s", rdv, name);

    if (rdv->name)
      xbt_dict_set(rdv_points, rdv->name, rdv, NULL);
  }
  return rdv;
}

void SIMIX_rdv_destroy(smx_mailbox_t rdv)
{
  if (rdv->name)
    xbt_dict_remove(rdv_points, rdv->name);
}

void SIMIX_rdv_free(void *data)
{
  XBT_DEBUG("rdv free %p", data);
  smx_mailbox_t rdv = (smx_mailbox_t) data;
  xbt_free(rdv->name);
  xbt_fifo_free(rdv->comm_fifo);
  xbt_fifo_free(rdv->done_comm_fifo);

  xbt_free(rdv);
}

xbt_dict_t SIMIX_get_rdv_points()
{
  return rdv_points;
}

smx_mailbox_t SIMIX_rdv_get_by_name(const char *name)
{
  return (smx_mailbox_t) xbt_dict_get_or_null(rdv_points, name);
}

int SIMIX_rdv_comm_count_by_host(smx_mailbox_t rdv, sg_host_t host)
{
  smx_synchro_t comm = NULL;
  xbt_fifo_item_t item = NULL;
  int count = 0;

  xbt_fifo_foreach(rdv->comm_fifo, item, comm, smx_synchro_t) {
    if (comm->comm.src_proc->host == host)
      count++;
  }

  return count;
}

smx_synchro_t SIMIX_rdv_get_head(smx_mailbox_t rdv)
{
  return (smx_synchro_t) xbt_fifo_get_item_content(
    xbt_fifo_get_first_item(rdv->comm_fifo));
}

/**
 *  \brief get the receiver (process associated to the mailbox)
 *  \param rdv The rendez-vous point
 *  \return process The receiving process (NULL if not set)
 */
smx_process_t SIMIX_rdv_get_receiver(smx_mailbox_t rdv)
{
  return rdv->permanent_receiver;
}

/**
 *  \brief set the receiver of the rendez vous point to allow eager sends
 *  \param rdv The rendez-vous point
 *  \param process The receiving process
 */
void SIMIX_rdv_set_receiver(smx_mailbox_t rdv, smx_process_t process)
{
  rdv->permanent_receiver=process;
}

/**
 *  \brief Pushes a communication synchro into a rendez-vous point
 *  \param rdv The rendez-vous point
 *  \param comm The communication synchro
 */
static inline void SIMIX_rdv_push(smx_mailbox_t rdv, smx_synchro_t comm)
{
  xbt_fifo_push(rdv->comm_fifo, comm);
  comm->comm.rdv = rdv;
}

/**
 *  \brief Removes a communication synchro from a rendez-vous point
 *  \param rdv The rendez-vous point
 *  \param comm The communication synchro
 */
void SIMIX_rdv_remove(smx_mailbox_t rdv, smx_synchro_t comm)
{
  xbt_fifo_remove(rdv->comm_fifo, comm);
  comm->comm.rdv = NULL;
}

/**
 *  \brief Checks if there is a communication synchro queued in a fifo matching our needs
 *  \param type The type of communication we are looking for (comm_send, comm_recv)
 *  \return The communication synchro if found, NULL otherwise
 */
smx_synchro_t SIMIX_fifo_get_comm(xbt_fifo_t fifo, e_smx_comm_type_t type,
                                 int (*match_fun)(void *, void *,smx_synchro_t),
                                 void *this_user_data, smx_synchro_t my_synchro)
{
  smx_synchro_t synchro;
  xbt_fifo_item_t item;
  void* other_user_data = NULL;

  xbt_fifo_foreach(fifo, item, synchro, smx_synchro_t) {
    if (synchro->comm.type == SIMIX_COMM_SEND) {
      other_user_data = synchro->comm.src_data;
    } else if (synchro->comm.type == SIMIX_COMM_RECEIVE) {
      other_user_data = synchro->comm.dst_data;
    }
    if (synchro->comm.type == type &&
        (!match_fun              ||              match_fun(this_user_data,  other_user_data, synchro)) &&
        (!synchro->comm.match_fun || synchro->comm.match_fun(other_user_data, this_user_data,  my_synchro))) {
      XBT_DEBUG("Found a matching communication synchro %p", synchro);
      xbt_fifo_remove_item(fifo, item);
      xbt_fifo_free_item(item);
      synchro->comm.refcount++;
#if HAVE_MC
      synchro->comm.rdv_cpy = synchro->comm.rdv;
#endif
      synchro->comm.rdv = NULL;
      return synchro;
    }
    XBT_DEBUG("Sorry, communication synchro %p does not match our needs:"
              " its type is %d but we are looking for a comm of type %d (or maybe the filtering didn't match)",
              synchro, (int)synchro->comm.type, (int)type);
  }
  XBT_DEBUG("No matching communication synchro found");
  return NULL;
}


/**
 *  \brief Checks if there is a communication synchro queued in a fifo matching our needs, but leave it there
 *  \param type The type of communication we are looking for (comm_send, comm_recv)
 *  \return The communication synchro if found, NULL otherwise
 */
smx_synchro_t SIMIX_fifo_probe_comm(xbt_fifo_t fifo, e_smx_comm_type_t type,
                                 int (*match_fun)(void *, void *,smx_synchro_t),
                                 void *this_user_data, smx_synchro_t my_synchro)
{
  smx_synchro_t synchro;
  xbt_fifo_item_t item;
  void* other_user_data = NULL;

  xbt_fifo_foreach(fifo, item, synchro, smx_synchro_t) {
    if (synchro->comm.type == SIMIX_COMM_SEND) {
      other_user_data = synchro->comm.src_data;
    } else if (synchro->comm.type == SIMIX_COMM_RECEIVE) {
      other_user_data = synchro->comm.dst_data;
    }
    if (synchro->comm.type == type &&
        (!match_fun              ||              match_fun(this_user_data,  other_user_data, synchro)) &&
        (!synchro->comm.match_fun || synchro->comm.match_fun(other_user_data, this_user_data,  my_synchro))) {
      XBT_DEBUG("Found a matching communication synchro %p", synchro);
      synchro->comm.refcount++;

      return synchro;
    }
    XBT_DEBUG("Sorry, communication synchro %p does not match our needs:"
              " its type is %d but we are looking for a comm of type %d (or maybe the filtering didn't match)",
              synchro, (int)synchro->comm.type, (int)type);
  }
  XBT_DEBUG("No matching communication synchro found");
  return NULL;
}
/******************************************************************************/
/*                          Communication synchros                            */
/******************************************************************************/

/**
 *  \brief Creates a new communicate synchro
 *  \param type The direction of communication (comm_send, comm_recv)
 *  \return The new communicate synchro
 */
smx_synchro_t SIMIX_comm_new(e_smx_comm_type_t type)
{
  smx_synchro_t synchro;

  /* alloc structures */
  synchro = (smx_synchro_t) xbt_mallocator_get(simix_global->synchro_mallocator);

  synchro->type = SIMIX_SYNC_COMMUNICATE;
  synchro->state = SIMIX_WAITING;

  /* set communication */
  synchro->comm.type = type;
  synchro->comm.refcount = 1;
  synchro->comm.src_data=NULL;
  synchro->comm.dst_data=NULL;

  synchro->category = NULL;

  XBT_DEBUG("Create communicate synchro %p", synchro);
  ++smx_total_comms;

  return synchro;
}

/**
 *  \brief Destroy a communicate synchro
 *  \param synchro The communicate synchro to be destroyed
 */
void SIMIX_comm_destroy(smx_synchro_t synchro)
{
  XBT_DEBUG("Destroy synchro %p (refcount: %d), state: %d",
            synchro, synchro->comm.refcount, (int)synchro->state);

  if (synchro->comm.refcount <= 0) {
    xbt_backtrace_display_current();
    xbt_die("The refcount of comm %p is already 0 before decreasing it. "
            "That's a bug! If you didn't test and/or wait the same communication twice in your code, then the bug is SimGrid's...", synchro);
  }
  synchro->comm.refcount--;
  if (synchro->comm.refcount > 0)
      return;
  XBT_DEBUG("Really free communication %p; refcount is now %d", synchro,
      synchro->comm.refcount);

  xbt_free(synchro->name);
  SIMIX_comm_destroy_internal_actions(synchro);

  if (synchro->comm.detached && synchro->state != SIMIX_DONE) {
    /* the communication has failed and was detached:
     * we have to free the buffer */
    if (synchro->comm.clean_fun) {
      synchro->comm.clean_fun(synchro->comm.src_buff);
    }
    synchro->comm.src_buff = NULL;
  }

  if(synchro->comm.rdv)
    SIMIX_rdv_remove(synchro->comm.rdv, synchro);

  xbt_mallocator_release(simix_global->synchro_mallocator, synchro);
}

void SIMIX_comm_destroy_internal_actions(smx_synchro_t synchro)
{
  if (synchro->comm.surf_comm){
    synchro->comm.surf_comm->unref();
    synchro->comm.surf_comm = NULL;
  }

  if (synchro->comm.src_timeout){
    synchro->comm.src_timeout->unref();
    synchro->comm.src_timeout = NULL;
  }

  if (synchro->comm.dst_timeout){
    synchro->comm.dst_timeout->unref();
    synchro->comm.dst_timeout = NULL;
  }
}

void simcall_HANDLER_comm_send(smx_simcall_t simcall, smx_process_t src, smx_mailbox_t rdv,
                                  double task_size, double rate,
                                  void *src_buff, size_t src_buff_size,
                                  int (*match_fun)(void *, void *,smx_synchro_t),
                                  void (*copy_data_fun)(smx_synchro_t, void*, size_t),
          void *data, double timeout){
  smx_synchro_t comm = simcall_HANDLER_comm_isend(simcall, src, rdv, task_size, rate,
                           src_buff, src_buff_size, match_fun, NULL, copy_data_fun,
               data, 0);
  SIMCALL_SET_MC_VALUE(simcall, 0);
  simcall_HANDLER_comm_wait(simcall, comm, timeout);
}
smx_synchro_t simcall_HANDLER_comm_isend(smx_simcall_t simcall, smx_process_t src_proc, smx_mailbox_t rdv,
                                  double task_size, double rate,
                                  void *src_buff, size_t src_buff_size,
                                  int (*match_fun)(void *, void *,smx_synchro_t),
                                  void (*clean_fun)(void *), // used to free the synchro in case of problem after a detached send
                                  void (*copy_data_fun)(smx_synchro_t, void*, size_t),// used to copy data if not default one
                          void *data, int detached)
{
  XBT_DEBUG("send from %p", rdv);

  /* Prepare a synchro describing us, so that it gets passed to the user-provided filter of other side */
  smx_synchro_t this_synchro = SIMIX_comm_new(SIMIX_COMM_SEND);

  /* Look for communication synchro matching our needs. We also provide a description of
   * ourself so that the other side also gets a chance of choosing if it wants to match with us.
   *
   * If it is not found then push our communication into the rendez-vous point */
  smx_synchro_t other_synchro = SIMIX_fifo_get_comm(rdv->comm_fifo, SIMIX_COMM_RECEIVE, match_fun, data, this_synchro);

  if (!other_synchro) {
    other_synchro = this_synchro;

    if (rdv->permanent_receiver!=NULL){
      //this mailbox is for small messages, which have to be sent right now
      other_synchro->state = SIMIX_READY;
      other_synchro->comm.dst_proc=rdv->permanent_receiver;
      other_synchro->comm.refcount++;
      xbt_fifo_push(rdv->done_comm_fifo,other_synchro);
      other_synchro->comm.rdv=rdv;
      XBT_DEBUG("pushing a message into the permanent receive fifo %p, comm %p", rdv, &(other_synchro->comm));

    }else{
      SIMIX_rdv_push(rdv, this_synchro);
    }
  } else {
    XBT_DEBUG("Receive already pushed");

    SIMIX_comm_destroy(this_synchro);
    --smx_total_comms; // this creation was a pure waste

    other_synchro->state = SIMIX_READY;
    other_synchro->comm.type = SIMIX_COMM_READY;

  }
  xbt_fifo_push(src_proc->comms, other_synchro);

  /* if the communication synchro is detached then decrease the refcount
   * by one, so it will be eliminated by the receiver's destroy call */
  if (detached) {
    other_synchro->comm.detached = 1;
    other_synchro->comm.refcount--;
    other_synchro->comm.clean_fun = clean_fun;
  } else {
    other_synchro->comm.clean_fun = NULL;
  }

  /* Setup the communication synchro */
  other_synchro->comm.src_proc = src_proc;
  other_synchro->comm.task_size = task_size;
  other_synchro->comm.rate = rate;
  other_synchro->comm.src_buff = src_buff;
  other_synchro->comm.src_buff_size = src_buff_size;
  other_synchro->comm.src_data = data;

  other_synchro->comm.match_fun = match_fun;
  other_synchro->comm.copy_data_fun = copy_data_fun;


  if (MC_is_active() || MC_record_replay_is_active()) {
    other_synchro->state = SIMIX_RUNNING;
    return (detached ? NULL : other_synchro);
  }

  SIMIX_comm_start(other_synchro);
  return (detached ? NULL : other_synchro);
}

void simcall_HANDLER_comm_recv(smx_simcall_t simcall, smx_process_t receiver, smx_mailbox_t rdv,
                         void *dst_buff, size_t *dst_buff_size,
                         int (*match_fun)(void *, void *, smx_synchro_t),
                         void (*copy_data_fun)(smx_synchro_t, void*, size_t),
                         void *data, double timeout, double rate)
{
  smx_synchro_t comm = SIMIX_comm_irecv(receiver, rdv, dst_buff,
                           dst_buff_size, match_fun, copy_data_fun, data, rate);
  SIMCALL_SET_MC_VALUE(simcall, 0);
  simcall_HANDLER_comm_wait(simcall, comm, timeout);
}

smx_synchro_t simcall_HANDLER_comm_irecv(smx_simcall_t simcall, smx_process_t receiver, smx_mailbox_t rdv,
    void *dst_buff, size_t *dst_buff_size,
    int (*match_fun)(void *, void *, smx_synchro_t),
    void (*copy_data_fun)(smx_synchro_t, void*, size_t),
    void *data, double rate)
{
  return SIMIX_comm_irecv(receiver, rdv, dst_buff, dst_buff_size, match_fun, copy_data_fun, data, rate);
}

smx_synchro_t SIMIX_comm_irecv(smx_process_t dst_proc, smx_mailbox_t rdv, void *dst_buff, size_t *dst_buff_size,
    int (*match_fun)(void *, void *, smx_synchro_t),
    void (*copy_data_fun)(smx_synchro_t, void*, size_t), // used to copy data if not default one
    void *data, double rate)
{
  XBT_DEBUG("recv from %p %p", rdv, rdv->comm_fifo);
  smx_synchro_t this_synchro = SIMIX_comm_new(SIMIX_COMM_RECEIVE);

  smx_synchro_t other_synchro;
  //communication already done, get it inside the fifo of completed comms
  if (rdv->permanent_receiver && xbt_fifo_size(rdv->done_comm_fifo)!=0) {

    XBT_DEBUG("We have a comm that has probably already been received, trying to match it, to skip the communication");
    //find a match in the already received fifo
    other_synchro = SIMIX_fifo_get_comm(rdv->done_comm_fifo, SIMIX_COMM_SEND, match_fun, data, this_synchro);
    //if not found, assume the receiver came first, register it to the mailbox in the classical way
    if (!other_synchro)  {
      XBT_DEBUG("We have messages in the permanent receive list, but not the one we are looking for, pushing request into fifo");
      other_synchro = this_synchro;
      SIMIX_rdv_push(rdv, this_synchro);
    } else {
      if(other_synchro->comm.surf_comm && SIMIX_comm_get_remains(other_synchro)==0.0) {
        XBT_DEBUG("comm %p has been already sent, and is finished, destroy it",&(other_synchro->comm));
        other_synchro->state = SIMIX_DONE;
        other_synchro->comm.type = SIMIX_COMM_DONE;
        other_synchro->comm.rdv = NULL;
      }
      other_synchro->comm.refcount--;
      SIMIX_comm_destroy(this_synchro);
      --smx_total_comms; // this creation was a pure waste
    }
  } else {
    /* Prepare a synchro describing us, so that it gets passed to the user-provided filter of other side */

    /* Look for communication synchro matching our needs. We also provide a description of
     * ourself so that the other side also gets a chance of choosing if it wants to match with us.
     *
     * If it is not found then push our communication into the rendez-vous point */
    other_synchro = SIMIX_fifo_get_comm(rdv->comm_fifo, SIMIX_COMM_SEND, match_fun, data, this_synchro);

    if (!other_synchro) {
      XBT_DEBUG("Receive pushed first %d", xbt_fifo_size(rdv->comm_fifo));
      other_synchro = this_synchro;
      SIMIX_rdv_push(rdv, this_synchro);
    } else {
      SIMIX_comm_destroy(this_synchro);
      --smx_total_comms; // this creation was a pure waste
      other_synchro->state = SIMIX_READY;
      other_synchro->comm.type = SIMIX_COMM_READY;
      //other_synchro->comm.refcount--;
    }
    xbt_fifo_push(dst_proc->comms, other_synchro);
  }

  /* Setup communication synchro */
  other_synchro->comm.dst_proc = dst_proc;
  other_synchro->comm.dst_buff = dst_buff;
  other_synchro->comm.dst_buff_size = dst_buff_size;
  other_synchro->comm.dst_data = data;

  if (rate != -1.0 && (other_synchro->comm.rate == -1.0 || rate < other_synchro->comm.rate))
    other_synchro->comm.rate = rate;

  other_synchro->comm.match_fun = match_fun;
  other_synchro->comm.copy_data_fun = copy_data_fun;

  if (MC_is_active() || MC_record_replay_is_active()) {
    other_synchro->state = SIMIX_RUNNING;
    return other_synchro;
  }

  SIMIX_comm_start(other_synchro);
  return other_synchro;
}

smx_synchro_t simcall_HANDLER_comm_iprobe(smx_simcall_t simcall, smx_mailbox_t rdv,
                                   int type, int src, int tag,
                                   int (*match_fun)(void *, void *, smx_synchro_t),
                                   void *data){
  return SIMIX_comm_iprobe(simcall->issuer, rdv, type, src, tag, match_fun, data);
}

smx_synchro_t SIMIX_comm_iprobe(smx_process_t dst_proc, smx_mailbox_t rdv, int type, int src,
                              int tag, int (*match_fun)(void *, void *, smx_synchro_t), void *data)
{
  XBT_DEBUG("iprobe from %p %p", rdv, rdv->comm_fifo);
  smx_synchro_t this_synchro;
  int smx_type;
  if(type == 1){
    this_synchro=SIMIX_comm_new(SIMIX_COMM_SEND);
    smx_type = SIMIX_COMM_RECEIVE;
  } else{
    this_synchro=SIMIX_comm_new(SIMIX_COMM_RECEIVE);
    smx_type = SIMIX_COMM_SEND;
  } 
  smx_synchro_t other_synchro=NULL;
  if(rdv->permanent_receiver && xbt_fifo_size(rdv->done_comm_fifo)!=0){
    //find a match in the already received fifo
      XBT_DEBUG("first try in the perm recv mailbox");

    other_synchro = SIMIX_fifo_probe_comm(
      rdv->done_comm_fifo, (e_smx_comm_type_t) smx_type,
      match_fun, data, this_synchro);
  }
 // }else{
    if(!other_synchro){
        XBT_DEBUG("try in the normal mailbox");
        other_synchro = SIMIX_fifo_probe_comm(
          rdv->comm_fifo, (e_smx_comm_type_t) smx_type,
          match_fun, data, this_synchro);
    }
//  }
  if(other_synchro)other_synchro->comm.refcount--;

  SIMIX_comm_destroy(this_synchro);
  --smx_total_comms;
  return other_synchro;
}

void simcall_HANDLER_comm_wait(smx_simcall_t simcall, smx_synchro_t synchro, double timeout)
{
  /* the simcall may be a wait, a send or a recv */
  surf_action_t sleep;

  /* Associate this simcall to the wait synchro */
  XBT_DEBUG("simcall_HANDLER_comm_wait, %p", synchro);

  xbt_fifo_push(synchro->simcalls, simcall);
  simcall->issuer->waiting_synchro = synchro;

  if (MC_is_active() || MC_record_replay_is_active()) {
    int idx = SIMCALL_GET_MC_VALUE(simcall);
    if (idx == 0) {
      synchro->state = SIMIX_DONE;
    } else {
      /* If we reached this point, the wait simcall must have a timeout */
      /* Otherwise it shouldn't be enabled and executed by the MC */
      if (timeout == -1)
        THROW_IMPOSSIBLE;

      if (synchro->comm.src_proc == simcall->issuer)
        synchro->state = SIMIX_SRC_TIMEOUT;
      else
        synchro->state = SIMIX_DST_TIMEOUT;
    }

    SIMIX_comm_finish(synchro);
    return;
  }

  /* If the synchro has already finish perform the error handling, */
  /* otherwise set up a waiting timeout on the right side          */
  if (synchro->state != SIMIX_WAITING && synchro->state != SIMIX_RUNNING) {
    SIMIX_comm_finish(synchro);
  } else { /* if (timeout >= 0) { we need a surf sleep action even when there is no timeout, otherwise surf won't tell us when the host fails */
    sleep = surf_host_sleep(simcall->issuer->host, timeout);
    sleep->setData(synchro);

    if (simcall->issuer == synchro->comm.src_proc)
      synchro->comm.src_timeout = sleep;
    else
      synchro->comm.dst_timeout = sleep;
  }
}

void simcall_HANDLER_comm_test(smx_simcall_t simcall, smx_synchro_t synchro)
{
  if(MC_is_active() || MC_record_replay_is_active()){
    simcall_comm_test__set__result(simcall, synchro->comm.src_proc && synchro->comm.dst_proc);
    if(simcall_comm_test__get__result(simcall)){
      synchro->state = SIMIX_DONE;
      xbt_fifo_push(synchro->simcalls, simcall);
      SIMIX_comm_finish(synchro);
    }else{
      SIMIX_simcall_answer(simcall);
    }
    return;
  }

  simcall_comm_test__set__result(simcall, (synchro->state != SIMIX_WAITING && synchro->state != SIMIX_RUNNING));
  if (simcall_comm_test__get__result(simcall)) {
    xbt_fifo_push(synchro->simcalls, simcall);
    SIMIX_comm_finish(synchro);
  } else {
    SIMIX_simcall_answer(simcall);
  }
}

void simcall_HANDLER_comm_testany(smx_simcall_t simcall, xbt_dynar_t synchros)
{
  unsigned int cursor;
  smx_synchro_t synchro;
  simcall_comm_testany__set__result(simcall, -1);

  if (MC_is_active() || MC_record_replay_is_active()){
    int idx = SIMCALL_GET_MC_VALUE(simcall);
    if(idx == -1){
      SIMIX_simcall_answer(simcall);
    }else{
      synchro = xbt_dynar_get_as(synchros, idx, smx_synchro_t);
      simcall_comm_testany__set__result(simcall, idx);
      xbt_fifo_push(synchro->simcalls, simcall);
      synchro->state = SIMIX_DONE;
      SIMIX_comm_finish(synchro);
    }
    return;
  }

  xbt_dynar_foreach(simcall_comm_testany__get__comms(simcall), cursor,synchro) {
    if (synchro->state != SIMIX_WAITING && synchro->state != SIMIX_RUNNING) {
      simcall_comm_testany__set__result(simcall, cursor);
      xbt_fifo_push(synchro->simcalls, simcall);
      SIMIX_comm_finish(synchro);
      return;
    }
  }
  SIMIX_simcall_answer(simcall);
}

void simcall_HANDLER_comm_waitany(smx_simcall_t simcall, xbt_dynar_t synchros)
{
  smx_synchro_t synchro;
  unsigned int cursor = 0;

  if (MC_is_active() || MC_record_replay_is_active()){
    int idx = SIMCALL_GET_MC_VALUE(simcall);
    synchro = xbt_dynar_get_as(synchros, idx, smx_synchro_t);
    xbt_fifo_push(synchro->simcalls, simcall);
    simcall_comm_waitany__set__result(simcall, idx);
    synchro->state = SIMIX_DONE;
    SIMIX_comm_finish(synchro);
    return;
  }

  xbt_dynar_foreach(synchros, cursor, synchro){
    /* associate this simcall to the the synchro */
    xbt_fifo_push(synchro->simcalls, simcall);

    /* see if the synchro is already finished */
    if (synchro->state != SIMIX_WAITING && synchro->state != SIMIX_RUNNING){
      SIMIX_comm_finish(synchro);
      break;
    }
  }
}

void SIMIX_waitany_remove_simcall_from_actions(smx_simcall_t simcall)
{
  smx_synchro_t synchro;
  unsigned int cursor = 0;
  xbt_dynar_t synchros = simcall_comm_waitany__get__comms(simcall);

  xbt_dynar_foreach(synchros, cursor, synchro) {
    xbt_fifo_remove(synchro->simcalls, simcall);
  }
}

/**
 *  \brief Starts the simulation of a communication synchro.
 *  \param synchro the communication synchro
 */
static inline void SIMIX_comm_start(smx_synchro_t synchro)
{
  /* If both the sender and the receiver are already there, start the communication */
  if (synchro->state == SIMIX_READY) {

    sg_host_t sender = synchro->comm.src_proc->host;
    sg_host_t receiver = synchro->comm.dst_proc->host;

    XBT_DEBUG("Starting communication %p from '%s' to '%s'", synchro,
              sg_host_get_name(sender), sg_host_get_name(receiver));

    synchro->comm.surf_comm = surf_network_model_communicate(surf_network_model,
                                                            sender, receiver,
                                                            synchro->comm.task_size, synchro->comm.rate);

    synchro->comm.surf_comm->setData(synchro);

    synchro->state = SIMIX_RUNNING;

    /* If a link is failed, detect it immediately */
    if (synchro->comm.surf_comm->getState() == SURF_ACTION_FAILED) {
      XBT_DEBUG("Communication from '%s' to '%s' failed to start because of a link failure",
                sg_host_get_name(sender), sg_host_get_name(receiver));
      synchro->state = SIMIX_LINK_FAILURE;
      SIMIX_comm_destroy_internal_actions(synchro);
    }

    /* If any of the process is suspend, create the synchro but stop its execution,
       it will be restarted when the sender process resume */
    if (SIMIX_process_is_suspended(synchro->comm.src_proc) ||
        SIMIX_process_is_suspended(synchro->comm.dst_proc)) {
      /* FIXME: check what should happen with the synchro state */

      if (SIMIX_process_is_suspended(synchro->comm.src_proc))
        XBT_DEBUG("The communication is suspended on startup because src (%s:%s) were suspended since it initiated the communication",
                  sg_host_get_name(synchro->comm.src_proc->host), synchro->comm.src_proc->name);
      else
        XBT_DEBUG("The communication is suspended on startup because dst (%s:%s) were suspended since it initiated the communication",
                  sg_host_get_name(synchro->comm.dst_proc->host), synchro->comm.dst_proc->name);

      synchro->comm.surf_comm->suspend();

    }
  }
}

/**
 * \brief Answers the SIMIX simcalls associated to a communication synchro.
 * \param synchro a finished communication synchro
 */
void SIMIX_comm_finish(smx_synchro_t synchro)
{
  unsigned int destroy_count = 0;
  smx_simcall_t simcall;

  while ((simcall = (smx_simcall_t) xbt_fifo_shift(synchro->simcalls))) {

    /* If a waitany simcall is waiting for this synchro to finish, then remove
       it from the other synchros in the waitany list. Afterwards, get the
       position of the actual synchro in the waitany dynar and
       return it as the result of the simcall */

    if (simcall->call == SIMCALL_NONE) //FIXME: maybe a better way to handle this case
      continue; // if process handling comm is killed
    if (simcall->call == SIMCALL_COMM_WAITANY) {
      SIMIX_waitany_remove_simcall_from_actions(simcall);
      if (!MC_is_active() && !MC_record_replay_is_active())
        simcall_comm_waitany__set__result(simcall, xbt_dynar_search(simcall_comm_waitany__get__comms(simcall), &synchro));
    }

    /* If the synchro is still in a rendez-vous point then remove from it */
    if (synchro->comm.rdv)
      SIMIX_rdv_remove(synchro->comm.rdv, synchro);

    XBT_DEBUG("SIMIX_comm_finish: synchro state = %d", (int)synchro->state);

    /* Check out for errors */

    if (simcall->issuer->host->isOff()) {
      simcall->issuer->context->iwannadie = 1;
      SMX_EXCEPTION(simcall->issuer, host_error, 0, "Host failed");
    } else

    switch (synchro->state) {

    case SIMIX_DONE:
      XBT_DEBUG("Communication %p complete!", synchro);
      SIMIX_comm_copy_data(synchro);
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
      if (simcall->issuer == synchro->comm.src_proc)
        simcall->issuer->context->iwannadie = 1;
//          SMX_EXCEPTION(simcall->issuer, host_error, 0, "Host failed");
      else
        SMX_EXCEPTION(simcall->issuer, network_error, 0, "Remote peer failed");
      break;

    case SIMIX_DST_HOST_FAILURE:
      if (simcall->issuer == synchro->comm.dst_proc)
        simcall->issuer->context->iwannadie = 1;
//          SMX_EXCEPTION(simcall->issuer, host_error, 0, "Host failed");
      else
        SMX_EXCEPTION(simcall->issuer, network_error, 0, "Remote peer failed");
      break;

    case SIMIX_LINK_FAILURE:

      XBT_DEBUG("Link failure in synchro %p between '%s' and '%s': posting an exception to the issuer: %s (%p) detached:%d",
                synchro,
                synchro->comm.src_proc ? sg_host_get_name(synchro->comm.src_proc->host) : NULL,
                synchro->comm.dst_proc ? sg_host_get_name(synchro->comm.dst_proc->host) : NULL,
                simcall->issuer->name, simcall->issuer, synchro->comm.detached);
      if (synchro->comm.src_proc == simcall->issuer) {
        XBT_DEBUG("I'm source");
      } else if (synchro->comm.dst_proc == simcall->issuer) {
        XBT_DEBUG("I'm dest");
      } else {
        XBT_DEBUG("I'm neither source nor dest");
      }
      SMX_EXCEPTION(simcall->issuer, network_error, 0, "Link failure");
      break;

    case SIMIX_CANCELED:
      if (simcall->issuer == synchro->comm.dst_proc)
        SMX_EXCEPTION(simcall->issuer, cancel_error, 0,
                      "Communication canceled by the sender");
      else
        SMX_EXCEPTION(simcall->issuer, cancel_error, 0,
                      "Communication canceled by the receiver");
      break;

    default:
      xbt_die("Unexpected synchro state in SIMIX_comm_finish: %d", (int)synchro->state);
    }

    /* if there is an exception during a waitany or a testany, indicate the position of the failed communication */
    if (simcall->issuer->doexception) {
      if (simcall->call == SIMCALL_COMM_WAITANY) {
        simcall->issuer->running_ctx->exception.value = xbt_dynar_search(simcall_comm_waitany__get__comms(simcall), &synchro);
      }
      else if (simcall->call == SIMCALL_COMM_TESTANY) {
        simcall->issuer->running_ctx->exception.value = xbt_dynar_search(simcall_comm_testany__get__comms(simcall), &synchro);
      }
    }

    if (simcall->issuer->host->isOff()) {
      simcall->issuer->context->iwannadie = 1;
    }

    simcall->issuer->waiting_synchro = NULL;
    xbt_fifo_remove(simcall->issuer->comms, synchro);
    if(synchro->comm.detached){
      if(simcall->issuer == synchro->comm.src_proc){
        if(synchro->comm.dst_proc)
          xbt_fifo_remove(synchro->comm.dst_proc->comms, synchro);
      }
      if(simcall->issuer == synchro->comm.dst_proc){
        if(synchro->comm.src_proc)
          xbt_fifo_remove(synchro->comm.src_proc->comms, synchro);
      }
    }
    SIMIX_simcall_answer(simcall);
    destroy_count++;
  }

  while (destroy_count-- > 0)
    SIMIX_comm_destroy(synchro);
}

/**
 * \brief This function is called when a Surf communication synchro is finished.
 * \param synchro the corresponding Simix communication
 */
void SIMIX_post_comm(smx_synchro_t synchro)
{
  /* Update synchro state */
  if (synchro->comm.src_timeout &&
      synchro->comm.src_timeout->getState() == SURF_ACTION_DONE)
    synchro->state = SIMIX_SRC_TIMEOUT;
  else if (synchro->comm.dst_timeout &&
    synchro->comm.dst_timeout->getState() == SURF_ACTION_DONE)
    synchro->state = SIMIX_DST_TIMEOUT;
  else if (synchro->comm.src_timeout &&
    synchro->comm.src_timeout->getState() == SURF_ACTION_FAILED)
    synchro->state = SIMIX_SRC_HOST_FAILURE;
  else if (synchro->comm.dst_timeout &&
      synchro->comm.dst_timeout->getState() == SURF_ACTION_FAILED)
    synchro->state = SIMIX_DST_HOST_FAILURE;
  else if (synchro->comm.surf_comm &&
    synchro->comm.surf_comm->getState() == SURF_ACTION_FAILED) {
    XBT_DEBUG("Puta madre. Surf says that the link broke");
    synchro->state = SIMIX_LINK_FAILURE;
  } else
    synchro->state = SIMIX_DONE;

  XBT_DEBUG("SIMIX_post_comm: comm %p, state %d, src_proc %p, dst_proc %p, detached: %d",
            synchro, (int)synchro->state, synchro->comm.src_proc, synchro->comm.dst_proc, synchro->comm.detached);

  /* destroy the surf actions associated with the Simix communication */
  SIMIX_comm_destroy_internal_actions(synchro);

  /* if there are simcalls associated with the synchro, then answer them */
  if (xbt_fifo_size(synchro->simcalls)) {
    SIMIX_comm_finish(synchro);
  }
}

void SIMIX_comm_cancel(smx_synchro_t synchro)
{
  /* if the synchro is a waiting state means that it is still in a rdv */
  /* so remove from it and delete it */
  if (synchro->state == SIMIX_WAITING) {
    SIMIX_rdv_remove(synchro->comm.rdv, synchro);
    synchro->state = SIMIX_CANCELED;
  }
  else if (!MC_is_active() /* when running the MC there are no surf actions */
           && !MC_record_replay_is_active()
           && (synchro->state == SIMIX_READY || synchro->state == SIMIX_RUNNING)) {

    synchro->comm.surf_comm->cancel();
  }
}

void SIMIX_comm_suspend(smx_synchro_t synchro)
{
  /*FIXME: shall we suspend also the timeout synchro? */
  if (synchro->comm.surf_comm)
    synchro->comm.surf_comm->suspend();
  /* in the other case, the action will be suspended on creation, in SIMIX_comm_start() */
}

void SIMIX_comm_resume(smx_synchro_t synchro)
{
  /*FIXME: check what happen with the timeouts */
  if (synchro->comm.surf_comm)
    synchro->comm.surf_comm->resume();
  /* in the other case, the synchro were not really suspended yet, see SIMIX_comm_suspend() and SIMIX_comm_start() */
}


/************* synchro Getters **************/

/**
 *  \brief get the amount remaining from the communication
 *  \param synchro The communication
 */
double SIMIX_comm_get_remains(smx_synchro_t synchro)
{
  double remains;

  if(!synchro){
    return 0;
  }

  switch (synchro->state) {

  case SIMIX_RUNNING:
    remains = synchro->comm.surf_comm->getRemains();
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

e_smx_state_t SIMIX_comm_get_state(smx_synchro_t synchro)
{
  return synchro->state;
}

/**
 *  \brief Return the user data associated to the sender of the communication
 *  \param synchro The communication
 *  \return the user data
 */
void* SIMIX_comm_get_src_data(smx_synchro_t synchro)
{
  return synchro->comm.src_data;
}

/**
 *  \brief Return the user data associated to the receiver of the communication
 *  \param synchro The communication
 *  \return the user data
 */
void* SIMIX_comm_get_dst_data(smx_synchro_t synchro)
{
  return synchro->comm.dst_data;
}

smx_process_t SIMIX_comm_get_src_proc(smx_synchro_t synchro)
{
  return synchro->comm.src_proc;
}

smx_process_t SIMIX_comm_get_dst_proc(smx_synchro_t synchro)
{
  return synchro->comm.dst_proc;
}

/******************************************************************************/
/*                    SIMIX_comm_copy_data callbacks                       */
/******************************************************************************/
static void (*SIMIX_comm_copy_data_callback) (smx_synchro_t, void*, size_t) =
  &SIMIX_comm_copy_pointer_callback;

void
SIMIX_comm_set_copy_data_callback(void (*callback) (smx_synchro_t, void*, size_t))
{
  SIMIX_comm_copy_data_callback = callback;
}

void SIMIX_comm_copy_pointer_callback(smx_synchro_t comm, void* buff, size_t buff_size)
{
  xbt_assert((buff_size == sizeof(void *)),
             "Cannot copy %zu bytes: must be sizeof(void*)", buff_size);
  *(void **) (comm->comm.dst_buff) = buff;
}

void SIMIX_comm_copy_buffer_callback(smx_synchro_t comm, void* buff, size_t buff_size)
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
void SIMIX_comm_copy_data(smx_synchro_t comm)
{
  size_t buff_size = comm->comm.src_buff_size;
  /* If there is no data to be copy then return */
  if (!comm->comm.src_buff || !comm->comm.dst_buff || comm->comm.copied)
    return;

  XBT_DEBUG("Copying comm %p data from %s (%p) -> %s (%p) (%zu bytes)",
            comm,
            comm->comm.src_proc ? sg_host_get_name(comm->comm.src_proc->host) : "a finished process",
            comm->comm.src_buff,
            comm->comm.dst_proc ? sg_host_get_name(comm->comm.dst_proc->host) : "a finished process",
            comm->comm.dst_buff, buff_size);

  /* Copy at most dst_buff_size bytes of the message to receiver's buffer */
  if (comm->comm.dst_buff_size)
    buff_size = MIN(buff_size, *(comm->comm.dst_buff_size));

  /* Update the receiver's buffer size to the copied amount */
  if (comm->comm.dst_buff_size)
    *comm->comm.dst_buff_size = buff_size;

  if (buff_size > 0){
      if(comm->comm.copy_data_fun)
        comm->comm.copy_data_fun (comm, comm->comm.src_buff, buff_size);
      else
        SIMIX_comm_copy_data_callback (comm, comm->comm.src_buff, buff_size);
  }


  /* Set the copied flag so we copy data only once */
  /* (this function might be called from both communication ends) */
  comm->comm.copied = 1;
}
