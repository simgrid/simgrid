#include "mailbox.h"
#include "msg/private.h"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(msg_mailbox, msg,
                                "Logging specific to MSG (mailbox)");

static xbt_dict_t msg_mailboxes = NULL;

void MSG_mailbox_mod_init(void)
{
  msg_mailboxes = xbt_dict_new();
}

void MSG_mailbox_mod_exit(void)
{
  xbt_dict_free(&msg_mailboxes);
}

msg_mailbox_t MSG_mailbox_create(const char *alias)
{
  msg_mailbox_t mailbox = xbt_new0(s_msg_mailbox_t, 1);

  mailbox->cond = NULL;
  mailbox->alias = alias ? xbt_strdup(alias) : NULL;
  mailbox->rdv = SIMIX_rdv_create(alias);
  
  return mailbox;
}

msg_mailbox_t MSG_mailbox_new(const char *alias)
{
  msg_mailbox_t mailbox = MSG_mailbox_create(alias);

  /* add the mbox in the dictionary */
  xbt_dict_set(msg_mailboxes, alias, mailbox, MSG_mailbox_free);

  return mailbox;
}

void MSG_mailbox_free(void *mailbox)
{
  msg_mailbox_t _mailbox = (msg_mailbox_t) mailbox;

  free(_mailbox->alias);
  SIMIX_rdv_destroy(_mailbox->rdv);
  
  free(_mailbox);
}

smx_cond_t MSG_mailbox_get_cond(msg_mailbox_t mailbox)
{
  return mailbox->cond;
}

int MSG_mailbox_is_empty(msg_mailbox_t mailbox)
{
  return (NULL == SIMIX_rdv_get_head(mailbox->rdv));
}

m_task_t MSG_mailbox_get_head(msg_mailbox_t mailbox)
{
  smx_comm_t comm = SIMIX_rdv_get_head(mailbox->rdv);

  if(!comm)
    return NULL; 
  
  return (m_task_t)SIMIX_communication_get_data(comm);
}

int
MSG_mailbox_get_count_host_waiting_tasks(msg_mailbox_t mailbox, m_host_t host)
{
  return SIMIX_rdv_get_count_waiting_comm (mailbox->rdv, host->simdata->smx_host);
}

void MSG_mailbox_set_cond(msg_mailbox_t mailbox, smx_cond_t cond)
{
  mailbox->cond = cond;
}

const char *MSG_mailbox_get_alias(msg_mailbox_t mailbox)
{
  return mailbox->alias;
}

msg_mailbox_t MSG_mailbox_get_by_alias(const char *alias)
{

  msg_mailbox_t mailbox = xbt_dict_get_or_null(msg_mailboxes, alias);

  if (!mailbox)
    mailbox = MSG_mailbox_new(alias);

  return mailbox;
}

msg_mailbox_t MSG_mailbox_get_by_channel(m_host_t host, m_channel_t channel)
{
  xbt_assert0((host != NULL), "Invalid host");
  xbt_assert1((channel >= 0)
              && (channel < msg_global->max_channel), "Invalid channel %d",
              channel);

  return host->simdata->mailboxes[(size_t) channel];
}

MSG_error_t
MSG_mailbox_get_task_ext(msg_mailbox_t mailbox, m_task_t *task, m_host_t host,
                         double timeout)
{
  xbt_ex_t e;
  MSG_error_t ret = MSG_OK;
  smx_comm_t comm;

  /* We no longer support getting a task from a specific host */
  if (host) THROW_UNIMPLEMENTED;

  CHECK_HOST();

  memset(&comm,0,sizeof(comm));

  /* Kept for compatibility with older implementation */
  xbt_assert1(!MSG_mailbox_get_cond(mailbox),
              "A process is already blocked on this channel %s", 
              MSG_mailbox_get_alias(mailbox));

  /* Sanity check */
  xbt_assert0(task, "Null pointer for the task storage");

  if (*task)
    CRITICAL0("MSG_task_get() was asked to write in a non empty task struct.");

  /* Try to receive it by calling SIMIX network layer */
  TRY{
    SIMIX_network_recv(mailbox->rdv, timeout, task, NULL, &comm);
    //INFO2("Got task %s from %s",(*task)->name,mailbox->alias);
    (*task)->simdata->refcount--;
  }
  CATCH(e){
    switch(e.category){
      case host_error:
        ret = MSG_HOST_FAILURE;
        break;
      case network_error:
        ret = MSG_TRANSFER_FAILURE;
        break;
      case timeout_error:
        ret = MSG_TIMEOUT_FAILURE;
        break;      
      default:
        xbt_die(bprintf("Unhandled SIMIX network exception: %s",e.msg));
    }
    xbt_ex_free(e);        
  }
  
  MSG_RETURN(ret);        
}

MSG_error_t
MSG_mailbox_put_with_timeout(msg_mailbox_t mailbox, m_task_t task,
                             double timeout)
{
  xbt_ex_t e;
  MSG_error_t ret = MSG_OK;
  simdata_task_t t_simdata = NULL;
  m_process_t process = MSG_process_self();
  
  CHECK_HOST();

  /* Prepare the task to send */
  t_simdata = task->simdata;
  t_simdata->sender = process;
  t_simdata->source = MSG_host_self();

  xbt_assert0(t_simdata->refcount == 1,
              "This task is still being used somewhere else. You cannot send it now. Go fix your code!");

  t_simdata->refcount++;
  msg_global->sent_msg++;

  process->simdata->waiting_task = task;
  
  /* Try to send it by calling SIMIX network layer */
  TRY{
    /* Kept for semantical compatibility with older implementation */
    if(mailbox->cond)
      SIMIX_cond_signal(mailbox->cond);

    SIMIX_network_send(mailbox->rdv, t_simdata->message_size, t_simdata->rate,
                       timeout, task, sizeof(void*), &t_simdata->comm, task);
  }

  CATCH(e){
    switch(e.category){
      case host_error:
        ret = MSG_HOST_FAILURE;
        break;
      case network_error:
        ret = MSG_TRANSFER_FAILURE;
        break;
      case timeout_error:
        ret = MSG_TIMEOUT_FAILURE;
        break;
      default:
        xbt_die(bprintf("Unhandled SIMIX network exception: %s",e.msg));
    }
    xbt_ex_free(e);

    /* Decrement the refcount only on failure */
    t_simdata->refcount--;
  }

  process->simdata->waiting_task = NULL;
   
  MSG_RETURN(ret);        
}
