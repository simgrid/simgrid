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

  if (NULL != (_mailbox->hostname))
    free(_mailbox->hostname);

  xbt_fifo_free(_mailbox->tasks);
  free(_mailbox->alias);

  free(_mailbox);
}

void MSG_mailbox_put(msg_mailbox_t mailbox, m_task_t task)
{
  xbt_fifo_push(mailbox->tasks, task);
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

  if (NULL == (item = xbt_fifo_get_first_item(mailbox->tasks)))
    return NULL;

  return (m_task_t) xbt_fifo_get_item_content(item);
}


m_task_t
MSG_mailbox_get_first_host_task(msg_mailbox_t mailbox, m_host_t host)
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
MSG_mailbox_get_count_host_waiting_tasks(msg_mailbox_t mailbox,
					 m_host_t host)
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

msg_mailbox_t
MSG_mailbox_get_by_channel(m_host_t host, m_channel_t channel)
{
  xbt_assert0((host != NULL), "Invalid host");
  xbt_assert1((channel >= 0)
	      && (channel < msg_global->max_channel), "Invalid channel %d",
	      channel);

  return host->simdata->mailboxes[(size_t) channel];
}

MSG_error_t
MSG_mailbox_get_task_ext(msg_mailbox_t mailbox, m_task_t * task,
			 m_host_t host, double timeout)
{
  m_process_t process = MSG_process_self();
  m_task_t t = NULL;
  m_host_t h = NULL;
  simdata_task_t t_simdata = NULL;
  simdata_host_t h_simdata = NULL;
  int first_time = 1;

  smx_cond_t cond = NULL;	//conditional wait if the task isn't on the channel yet

  CHECK_HOST();

  /* Sanity check */
  xbt_assert0(task, "Null pointer for the task storage");

  if (*task)
    CRITICAL0
	("MSG_task_get() was asked to write in a non empty task struct.");

  /* Get the task */
  h = MSG_host_self();
  h_simdata = h->simdata;

  SIMIX_mutex_lock(h->simdata->mutex);

  if (MSG_mailbox_get_cond(mailbox)) {
    CRITICAL1("A process is already blocked on the channel %s",
	      MSG_mailbox_get_alias(mailbox));
    SIMIX_cond_display_info(MSG_mailbox_get_cond(mailbox));
    xbt_die("Go fix your code!");
  }

  while (1) {
    /* if the mailbox is empty (has no task */
    if (!MSG_mailbox_is_empty(mailbox)) {
      if (!host) {
	/* pop the head of the mailbox */
	t = MSG_mailbox_pop_head(mailbox);
	break;
      } else {
	/* get the first task of the host */
	if (NULL != (t = MSG_mailbox_get_first_host_task(mailbox, host)))
	  break;
      }
    }

    if (timeout > 0) {
      if (!first_time) {
	SIMIX_mutex_unlock(h->simdata->mutex);
	/* set the simix condition of the mailbox to NULL */
	MSG_mailbox_set_cond(mailbox, NULL);
	SIMIX_cond_destroy(cond);
	MSG_RETURN(MSG_TRANSFER_FAILURE);
      }
    }

    cond = SIMIX_cond_init();

    /* set the condition of the mailbox */
    MSG_mailbox_set_cond(mailbox, cond);

    if (timeout > 0)
      SIMIX_cond_wait_timeout(cond, h->simdata->mutex, timeout);
    else
      SIMIX_cond_wait(MSG_mailbox_get_cond(mailbox), h->simdata->mutex);


    if (SIMIX_host_get_state(h_simdata->smx_host) == 0)
      MSG_RETURN(MSG_HOST_FAILURE);

    first_time = 0;
  }

  SIMIX_mutex_unlock(h->simdata->mutex);

  DEBUG1("OK, got a task (%s)", t->name);
  /* clean conditional */
  if (cond) {
    SIMIX_cond_destroy(cond);

    MSG_mailbox_set_cond(mailbox, NULL);
  }

  t_simdata = t->simdata;
  t_simdata->receiver = process;
  *task = t;

  SIMIX_mutex_lock(t_simdata->mutex);

  /* Transfer */
  /* create SIMIX action to the communication */
  t_simdata->comm =
      SIMIX_action_communicate(t_simdata->sender->simdata->m_host->
			       simdata->smx_host,
			       process->simdata->m_host->simdata->smx_host,
			       t->name, t_simdata->message_size,
			       t_simdata->rate);

  /* if the process is suspend, create the action but stop its execution, it will be restart when the sender process resume */
  if (MSG_process_is_suspended(t_simdata->sender)) {
    DEBUG1("Process sender (%s) suspended", t_simdata->sender->name);
    SIMIX_action_set_priority(t_simdata->comm, 0);
  }

  process->simdata->waiting_task = t;
  SIMIX_register_action_to_condition(t_simdata->comm, t_simdata->cond);

  while (1) {
    SIMIX_cond_wait(t_simdata->cond, t_simdata->mutex);

    if (SIMIX_action_get_state(t_simdata->comm) != SURF_ACTION_RUNNING)
      break;
  }

  SIMIX_unregister_action_to_condition(t_simdata->comm, t_simdata->cond);
  process->simdata->waiting_task = NULL;

  /* the task has already finished and the pointer must be null */
  if (t->simdata->sender) {
    t->simdata->sender->simdata->waiting_task = NULL;
  }

  /* for this process, don't need to change in get function */
  t->simdata->receiver = NULL;
  SIMIX_mutex_unlock(t_simdata->mutex);


  if (SIMIX_action_get_state(t_simdata->comm) == SURF_ACTION_DONE) {
    SIMIX_action_destroy(t_simdata->comm);
    t_simdata->comm = NULL;
    t_simdata->using--;
    MSG_RETURN(MSG_OK);
  } else if (SIMIX_host_get_state(h_simdata->smx_host) == 0) {
    SIMIX_action_destroy(t_simdata->comm);
    t_simdata->comm = NULL;
    t_simdata->using--;
    MSG_RETURN(MSG_HOST_FAILURE);
  } else {
    SIMIX_action_destroy(t_simdata->comm);
    t_simdata->comm = NULL;
    t_simdata->using--;
    MSG_RETURN(MSG_TRANSFER_FAILURE);
  }
}

MSG_error_t
MSG_mailbox_put_with_timeout(msg_mailbox_t mailbox, m_task_t task,
			     double timeout)
{
  m_process_t process = MSG_process_self();
  const char *hostname;
  simdata_task_t task_simdata = NULL;
  m_host_t local_host = NULL;
  m_host_t remote_host = NULL;
  smx_cond_t cond = NULL;

  CHECK_HOST();

  task_simdata = task->simdata;
  task_simdata->sender = process;
  task_simdata->source = MSG_process_get_host(process);

  xbt_assert0(task_simdata->using == 1,
	      "This task is still being used somewhere else. You cannot send it now. Go fix your code!");

  task_simdata->comm = NULL;

  task_simdata->using++;
  local_host = ((simdata_process_t) process->simdata)->m_host;

  /* get the host name containing the mailbox */
  hostname = MSG_mailbox_get_hostname(mailbox);

  remote_host = MSG_get_host_by_name(hostname);

  if (NULL == remote_host)
    THROW1(not_found_error, 0, "Host %s not fount", hostname);


  DEBUG4
      ("Trying to send a task (%g kB) from %s to %s on the channel aliased by the alias %s",
       task->simdata->message_size / 1000, local_host->name,
       remote_host->name, MSG_mailbox_get_alias(mailbox));

  SIMIX_mutex_lock(remote_host->simdata->mutex);

  /* put the task in the mailbox */
  MSG_mailbox_put(mailbox, task);

  if (NULL != (cond = MSG_mailbox_get_cond(mailbox))) {
    DEBUG0("Somebody is listening. Let's wake him up!");
    SIMIX_cond_signal(cond);
  }



  SIMIX_mutex_unlock(remote_host->simdata->mutex);

  SIMIX_mutex_lock(task->simdata->mutex);

  process->simdata->waiting_task = task;

  if (timeout > 0) {
    xbt_ex_t e;
    double time;
    double time_elapsed;
    time = SIMIX_get_clock();

    TRY {
      /*verify if the action that ends is the correct. Call the wait_timeout with the new time. If the timeout occurs, an exception is raised */
      while (1) {
	time_elapsed = SIMIX_get_clock() - time;
	SIMIX_cond_wait_timeout(task->simdata->cond, task->simdata->mutex,
				timeout - time_elapsed);

	if ((task->simdata->comm != NULL)
	    && (SIMIX_action_get_state(task->simdata->comm) !=
		SURF_ACTION_RUNNING))
	  break;
      }
    }
    CATCH(e) {
      if (e.category == timeout_error) {
	xbt_ex_free(e);
	/* verify if the timeout happened and the communication didn't started yet */
	if (task->simdata->comm == NULL) {
	  process->simdata->waiting_task = NULL;

	  /* remove the task from the mailbox */
	  MSG_mailbox_remove(mailbox, task);

	  if (task->simdata->receiver) {
	    task->simdata->receiver->simdata->waiting_task = NULL;
	  }

	  task->simdata->sender = NULL;

	  SIMIX_mutex_unlock(task->simdata->mutex);
	  MSG_RETURN(MSG_TRANSFER_FAILURE);
	}
      } else {
	RETHROW;
      }
    }
  } else {
    while (1) {
      SIMIX_cond_wait(task->simdata->cond, task->simdata->mutex);

      if (SIMIX_action_get_state(task->simdata->comm) !=
	  SURF_ACTION_RUNNING)
	break;
    }
  }

  DEBUG1("Action terminated %s", task->name);
  process->simdata->waiting_task = NULL;

  /* the task has already finished and the pointer must be null */
  if (task->simdata->receiver) {
    task->simdata->receiver->simdata->waiting_task = NULL;
  }

  task->simdata->sender = NULL;
  SIMIX_mutex_unlock(task->simdata->mutex);


  if (SIMIX_action_get_state(task->simdata->comm) == SURF_ACTION_DONE) {
    MSG_RETURN(MSG_OK);
  } else if (SIMIX_host_get_state(local_host->simdata->smx_host) == 0) {
    MSG_RETURN(MSG_HOST_FAILURE);
  } else {
    MSG_RETURN(MSG_TRANSFER_FAILURE);
  }
}
