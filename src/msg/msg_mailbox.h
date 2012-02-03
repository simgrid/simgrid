/* Copyright (c) 2008, 2009, 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
  * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SMX_MAILBOX_H
#define SMX_MAILBOX_H

#include "xbt/fifo.h"
#include "simix/simix.h"
#include "msg/datatypes.h"


SG_BEGIN_DECL()
#define MAX_ALIAS_NAME	((size_t)260)

/*! \brief MSG_mailbox_new - create a new mailbox.
 *
 * The function MSG_mailbox_new creates a new mailbox identified by the key specified
 * by the parameter alias and add it in the global dictionary.
 *
 * \param alias		The alias of the mailbox to create.
 *
 * \return		The newly created mailbox.
 */
XBT_PUBLIC(msg_mailbox_t)
    MSG_mailbox_new(const char *alias);

/* \brief MSG_mailbox_free - release a mailbox from the memory.
 *
 * The function MSG_mailbox_free release a mailbox from the memory but does
 * not remove it from the dictionary.
 *
 * \param mailbox	The mailbox to release.
 *
 * \see			MSG_mailbox_destroy.
 */
void MSG_mailbox_free(void *mailbox);

/* \brief MSG_mailbox_get_by_alias - get a mailbox from its alias.
 *
 * The function MSG_mailbox_get_by_alias returns the mailbox associated with
 * the key specified by the parameter alias. If the mailbox does not exists,
 * the function create it.
 *
 * \param alias		The alias of the mailbox to return.
 *
 * \return	The mailbox associated with the alias specified as parameter
 *		or a new mailbox if the key does not match.
 */
XBT_PUBLIC(msg_mailbox_t)
    MSG_mailbox_get_by_alias(const char *alias);

/*! \brief MSG_mailbox_is_empty - test if a mailbox is empty.
 *
 * The function MSG_mailbox_is_empty tests if a mailbox is empty
 * (contains no msg task).
 *
 * \param mailbox	The mailbox to get test.
 *
 * \return	The function returns 1 if the mailbox is empty. Otherwise the function
 *		returns 0.
 */
XBT_PUBLIC(int) MSG_mailbox_is_empty(msg_mailbox_t mailbox);

/*! \brief MSG_mailbox_get_head - get the task at the head of a mailbox.
 *
 * The MSG_mailbox_get_head returns the task at the head of the mailbox.
 * This function does not remove the task from the mailbox (contrary to
 * the function MSG_mailbox_pop_head).
 *
 * \param mailbox	The mailbox concerned by the operation.
 *
 * \return		The task at the head of the mailbox.
 */
XBT_PUBLIC(m_task_t)
    MSG_mailbox_get_head(msg_mailbox_t mailbox);

/*! \brief MSG_mailbox_get_count_host_waiting_tasks - Return the number of tasks
   waiting to be received in a mailbox and sent by a host.
 *
 * \param mailbox	The mailbox concerned by the operation.
 * \param host		The msg host containing the processes that have sended the
 *			tasks.
 *
 * \return		The number of tasks in the mailbox specified by the
 *			parameter mailbox and sended by all the processes located
 *			on the host specified by the parameter host.
 */
XBT_PUBLIC(int)
MSG_mailbox_get_count_host_waiting_tasks(msg_mailbox_t mailbox,
                                         m_host_t host);

#ifdef MSG_USE_DEPRECATED
/* \brief MSG_mailbox_get_by_channel - get a mailbox of the specified host from its channel.
 *
 * The function MSG_mailbox_get_by_channel returns the mailbox of the
 * specified host from the channel specified by the parameter
 * channel. If the mailbox does not exists, the function fails.
 *
 * \param host      The host containing he mailbox to get.
 * \param channel   The channel used to identify the mailbox.
 *
 * \return The mailbox of the specified host associated the channel
 *         specified as parameter.
 *
 */
XBT_PUBLIC(msg_mailbox_t)
    MSG_mailbox_get_by_channel(m_host_t host, m_channel_t channel);
#endif

SG_END_DECL()
#endif                          /* !SMX_MAILBOX_H */
