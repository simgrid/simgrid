#include "msg_mailbox.h"
#include "msg/private.h"

static xbt_dict_t 
msg_mailboxes = NULL;

xbt_dict_t
MSG_get_mailboxes(void)
{
	return msg_mailboxes;
}

void
MSG_mailbox_mod_init(void)
{
	msg_mailboxes = xbt_dict_new();	
}

void
MSG_mailbox_mod_exit(void)
{
	xbt_dict_free(&msg_mailboxes);
}

msg_mailbox_t
MSG_mailbox_new(const char *alias)
{
	msg_mailbox_t mailbox = xbt_new0(s_msg_mailbox_t,1);
	
	mailbox->tasks = xbt_fifo_new();
	mailbox->cond = NULL;
	mailbox->alias = xbt_strdup(alias);
	mailbox->hostname = NULL;
	
	/* add the mbox in the dictionary */
	xbt_dict_set(msg_mailboxes, alias, mailbox, MSG_mailbox_free);
	
	return mailbox;
}

void
MSG_mailbox_destroy(msg_mailbox_t* mailbox)
{
	xbt_dict_remove(msg_mailboxes,(*mailbox)->alias);

	if(NULL != ((*mailbox)->hostname))
		free((*mailbox)->hostname);

	
	free((*mailbox)->alias);
	
	free(*mailbox);
	
	*mailbox = NULL;	
}

void
MSG_mailbox_put(msg_mailbox_t mailbox, m_task_t task)
{
	xbt_fifo_push(mailbox->tasks, task);
}

smx_cond_t
MSG_mailbox_get_cond(msg_mailbox_t mailbox)
{
	return mailbox->cond;
}

void
MSG_mailbox_remove(msg_mailbox_t mailbox, m_task_t task)
{
	xbt_fifo_remove(mailbox->tasks,task);
}

int
MSG_mailbox_is_empty(msg_mailbox_t mailbox)
{
	return (NULL == xbt_fifo_get_first_item(mailbox->tasks));
}

m_task_t
MSG_mailbox_pop_head(msg_mailbox_t mailbox)
{
	return (m_task_t)xbt_fifo_shift(mailbox->tasks);
}

m_task_t
MSG_mailbox_get_head(msg_mailbox_t mailbox)
{
	xbt_fifo_item_t item;
	
	if(NULL == (item = xbt_fifo_get_first_item(mailbox->tasks)))
		return NULL;
		
	return (m_task_t)xbt_fifo_get_item_content(item);
}

m_task_t
MSG_mailbox_get_first_host_task(msg_mailbox_t mailbox, m_host_t host)
{
	m_task_t task = NULL;
	xbt_fifo_item_t item = NULL;
	
	xbt_fifo_foreach(mailbox->tasks, item, task, m_task_t) 
	{
		if (task->simdata->source == host)
			break;
	}
	
	if(item) 
		xbt_fifo_remove_item(mailbox->tasks, item);
		
	return task;
}

int
MSG_mailbox_get_count_host_tasks(msg_mailbox_t mailbox, m_host_t host)
{
	m_task_t task = NULL;
	xbt_fifo_item_t item = NULL;
	int count = 0;
	
	xbt_fifo_foreach(mailbox->tasks, item, task, m_task_t) 
	{
		if (task->simdata->source == host)
			count++;
	}
		
	return count;
}

void
MSG_mailbox_set_cond(msg_mailbox_t mailbox, smx_cond_t cond)
{
	mailbox->cond = cond;
}

const char*
MSG_mailbox_get_alias(msg_mailbox_t mailbox)
{
	return mailbox->alias;
}

const char*
MSG_mailbox_get_hostname(msg_mailbox_t mailbox)
{
	return mailbox->hostname;
}

void
MSG_mailbox_set_hostname(msg_mailbox_t mailbox, const char* hostname)
{
	mailbox->hostname = xbt_strdup(hostname);
}

void
MSG_mailbox_free(void* mailbox)
{
	msg_mailbox_t __mailbox = (msg_mailbox_t)mailbox;

	if(NULL != (__mailbox->hostname))
		free(__mailbox->hostname);

	free(__mailbox->alias);
	
	free(__mailbox);
}

msg_mailbox_t
MSG_mailbox_get_by_alias(const char* alias)
{
	xbt_ex_t e;
	int found = 1;
	msg_mailbox_t mailbox;

	TRY 
	{
		mailbox = xbt_dict_get(msg_mailboxes,alias);
	} 
	CATCH(e) 
	{
		if (e.category == not_found_error) 
		{
			found = 0;
			xbt_ex_free(e);
		} 
		else 
		{
			RETHROW;
		}
	}

	if(!found)
	{
		mailbox = MSG_mailbox_new(alias);
		MSG_mailbox_set_hostname(mailbox,MSG_host_self()->name);
	}
	
	return mailbox;	
}