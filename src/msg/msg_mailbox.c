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

  mailbox->tasks = xbt_fifo_new();
  mailbox->cond = NULL;
  mailbox->alias = alias ? xbt_strdup(alias) : NULL;
  mailbox->hostname = NULL;
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

  if (_mailbox->hostname)
    free(_mailbox->hostname);

  xbt_fifo_free(_mailbox->tasks);
  free(_mailbox->alias);
  SIMIX_rdv_destroy(_mailbox->rdv);
  
  free(_mailbox);
}

smx_cond_t MSG_mailbox_get_cond(msg_mailbox_t mailbox)
{
  return mailbox->cond;
}

void MSG_mailbox_remove(msg_mailbox_t mailbox, m_task_t task)
{
  xbt_fifo_remove(mailbox->tasks, task);
}

int MSG_mailbox_is_empty(msg_mailbox_t mailbox)
{
  return (NULL == xbt_fifo_get_first_item(mailbox->tasks));
}

m_task_t MSG_mailbox_pop_head(msg_mailbox_t mailbox)
{
  return (m_task_t) xbt_fifo_shift(mailbox->tasks);
}

m_task_t MSG_mailbox_get_head(msg_mailbox_t mailbox)
{
  xbt_fifo_item_t item;

  if (!(item = xbt_fifo_get_first_item(mailbox->tasks)))
    return NULL;

  return (m_task_t) xbt_fifo_get_item_content(item);
}


m_task_t MSG_mailbox_get_first_host_task(msg_mailbox_t mailbox, m_host_t host)
{
  m_task_t task = NULL;
  xbt_fifo_item_t item = NULL;

  xbt_fifo_foreach(mailbox->tasks, item, task, m_task_t)
    if (task->simdata->source == host) {
    xbt_fifo_remove_item(mailbox->tasks, item);
    return task;
  }

  return NULL;
}

int
MSG_mailbox_get_count_host_waiting_tasks(msg_mailbox_t mailbox, m_host_t host)
{
  m_task_t task = NULL;
  xbt_fifo_item_t item = NULL;
  int count = 0;

  xbt_fifo_foreach(mailbox->tasks, item, task, m_task_t) {
    if (task->simdata->source == host)
      count++;
  }

  return count;
}

void MSG_mailbox_set_cond(msg_mailbox_t mailbox, smx_cond_t cond)
{
  mailbox->cond = cond;
}

const char *MSG_mailbox_get_alias(msg_mailbox_t mailbox)
{
  return mailbox->alias;
}

const char *MSG_mailbox_get_hostname(msg_mailbox_t mailbox)
{
  return mailbox->hostname;
}

void MSG_mailbox_set_hostname(msg_mailbox_t mailbox, const char *hostname)
{
  mailbox->hostname = xbt_strdup(hostname);
}

msg_mailbox_t MSG_mailbox_get_by_alias(const char *alias)
{

  msg_mailbox_t mailbox = xbt_dict_get_or_null(msg_mailboxes, alias);

  if (!mailbox) {
    mailbox = MSG_mailbox_new(alias);
    MSG_mailbox_set_hostname(mailbox, MSG_host_self()->name);
  }

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
MSG_mailbox_get_task_ext(msg_mailbox_t mailbox, m_task_t *task,
                         m_host_t host, double timeout)
{
  xbt_ex_t e;
  MSG_error_t ret;
  smx_host_t smx_host;
  size_t task_size = sizeof(void*);
  CHECK_HOST();

  /* Sanity check */
  xbt_assert0(task, "Null pointer for the task storage");

  if (*task)
    CRITICAL0
      ("MSG_task_get() was asked to write in a non empty task struct.");

  smx_host = host ? host->simdata->smx_host : NULL;
  
  TRY{
    SIMIX_network_recv(mailbox->rdv, timeout, task, &task_size);
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
        ret = MSG_TRANSFER_FAILURE;
        break;      
      default:
        ret = MSG_OK;
        RETHROW;
        break;
        /*xbt_die("Unhandled SIMIX network exception");*/
    }
    xbt_ex_free(e);
    MSG_RETURN(ret);        
  }
 
  MSG_RETURN (MSG_OK);
}

MSG_error_t
MSG_mailbox_put_with_timeout(msg_mailbox_t mailbox, m_task_t task,
                             double timeout)
{
  xbt_ex_t e;
  MSG_error_t ret;
  m_process_t process = MSG_process_self();
  const char *hostname;
  simdata_task_t t_simdata = NULL;
  m_host_t local_host = NULL;
  m_host_t remote_host = NULL;

  CHECK_HOST();

  t_simdata = task->simdata;
  t_simdata->sender = process;
  t_simdata->source = MSG_process_get_host(process);

  xbt_assert0(t_simdata->refcount == 1,
              "This task is still being used somewhere else. You cannot send it now. Go fix your code!");

  t_simdata->comm = NULL;

  /*t_simdata->refcount++;*/
  local_host = ((simdata_process_t) process->simdata)->m_host;
  msg_global->sent_msg++;

  /* get the host name containing the mailbox */
  hostname = MSG_mailbox_get_hostname(mailbox);

  remote_host = MSG_get_host_by_name(hostname);

  if (!remote_host)
    THROW1(not_found_error, 0, "Host %s not fount", hostname);

  DEBUG4("Trying to send a task (%g kB) from %s to %s on the channel %s",
         t_simdata->message_size / 1000, local_host->name,
         remote_host->name, MSG_mailbox_get_alias(mailbox));

  TRY{
    SIMIX_network_send(mailbox->rdv, t_simdata->message_size, t_simdata->rate,
                       timeout, &task, sizeof(void *));
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
        ret = MSG_TRANSFER_FAILURE;
        break;      
      default:
        ret = MSG_OK;
        RETHROW;
        break;
        xbt_die("Unhandled SIMIX network exception");
    }
    xbt_ex_free(e);
    MSG_RETURN(ret);        
  }

  /*t_simdata->refcount--;*/
  MSG_RETURN (MSG_OK);
}
