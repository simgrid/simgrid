#ifndef SMX_MAILBOX_H
#define SMX_MAILBOX_H

#include "xbt/fifo.h"
#include "simix/private.h"
#include "msg/datatypes.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_ALIAS_NAME	((size_t)260)

/* this structure represents a mailbox */
typedef struct s_msg_mailbox
{
	char* alias;			/* the key of the mailbox in the dictionary				*/
	xbt_fifo_t tasks;		/* the list of the tasks in the mailbox					*/
	smx_cond_t cond;		/* the condition on the mailbox							*/
	char* hostname;			/* the name of the host containing the mailbox			*/
}s_msg_mailbox_t, * msg_mailbox_t;

/*
 * Initialization of the mailbox module.
 */
void
MSG_mailbox_mod_init(void);

/*
 * Terminaison of the mailbox module.
 */
void
MSG_mailbox_mod_exit(void);

/*! \brief MSG_get_mailboxes - get the dictionary containing all the mailboxes.
 * 
 * The function MSG_get_mailboxes returns the dictionary containing all the mailboxes
 * of the simulation. A mailbox allow different processes to communicate using msg tasks.
 * It is identified by an alias which is a key of the dictionary containing all of them.
 * 
 * \return	The dictionary containing all the mailboxes of the simulation.
 */
xbt_dict_t
MSG_get_mailboxes(void);

/*! \brief MSG_mailbox_new - create a new mailbox.
 *
 * The function MSG_mailbox_new creates a new mailbox identified by the key specified
 * by the parameter alias.
 *
 * \param alias		The alias of the mailbox to create.
 * 
 * \return			The newly created mailbox.
 */
msg_mailbox_t
MSG_mailbox_new(const char *alias);

/* \brief MSG_mailbox_destroy - destroy a mailbox.
 *
 * The function MSG_mailbox_destroy removes a mailbox from the dictionary containing
 * all the mailbox of the simulation an release it from the memory. This function is
 * different to the MSG_mailbox_free which only release a mailbox from the memory but
 does not remove it from the dictionary.
 *
 * \param mailbox	The mailbox to destroy.
 *
 * \see				MSG_mailbox_free.
 */
void
MSG_mailbox_destroy(msg_mailbox_t* mailbox);

/* \brief MSG_mailbox_free - release a mailbox from the memory.
 *
 * The function MSG_mailbox_free release a mailbox from the memory but does
 * not remove it from the dictionary.
 *
 * \param mailbox	The mailbox to release.
 *
 * \see				MSG_mailbox_destroy.
 */
void
MSG_mailbox_free(void* mailbox);

/* \brief MSG_mailbox_get_by_alias - get a mailbox from its alias.
 *
 * The function MSG_mailbox_get_by_alias returns the mailbox associated with
 * the key specified by the parameter alias. If the mailbox does not exists,
 * the function create it.
 *
 * \param alias		The alias of the mailbox to return.
 *
 * \return			The mailbox associated with the alias specified as parameter
 *					or a new mailbox if the key does not match.
 */
msg_mailbox_t
MSG_mailbox_get_by_alias(const char* alias);

/*! \brief MSG_mailbox_get_alias - get the alias associated with the mailbox.
 *
 * The function MSG_mailbox_get_alias returns the alias of the mailbox specified
 * by the parameter mailbox.
 *
 * \param mailbox	The mailbox to get the alias.
 *
 * \return			The alias of the mailbox specified by the parameter mailbox.
 */
const char*
MSG_mailbox_get_alias(msg_mailbox_t mailbox);

/*! \brief MSG_mailbox_get_cond - get the simix condition of a mailbox.
 *
 * The function MSG_mailbox_get_cond returns the condition of the mailbox specified
 * by the parameter mailbox.
 *
 * \param mailbox	The mailbox to get the condition.
 *
 * \return			The simix condition of the mailbox specified by the parameter mailbox.
 */
smx_cond_t
MSG_mailbox_get_cond(msg_mailbox_t mailbox);

/*! \brief MSG_mailbox_set_cond - set the simix condition of a mailbox.
 *
 * The function MSG_mailbox_set_cond set the condition of the mailbox specified
 * by the parameter mailbox.
 *
 * \param mailbox	The mailbox to set the condition.
 * \param cond		The new simix condition of the mailbox.
 *
 */
void
MSG_mailbox_set_cond(msg_mailbox_t mailbox, smx_cond_t cond);

/*! \brief MSG_mailbox_get_hostname - get the name of the host owned a mailbox.
 *
 * The function MSG_mailbox_get_hostname returns name of the host owned the mailbox specified
 * by the parameter mailbox.
 *
 * \param mailbox	The mailbox to get the name of the host.
 *
 * \return			The name of the host owned the mailbox specified by the parameter mailbox.
 */
const char*
MSG_mailbox_get_hostname(msg_mailbox_t mailbox);

/*! \brief MSG_mailbox_set_hostname - set the name of the host owned a mailbox.
 *
 * The function MSG_mailbox_set_hostname sets the name of the host owned the mailbox specified
 * by the parameter mailbox.
 *
 * \param mailbox	The mailbox to set the name of the host.
 * \param hostname	The name of the owner of the mailbox.
 *
 */
void
MSG_mailbox_set_hostname(msg_mailbox_t mailbox, const char* hostname);


/*! \brief MSG_mailbox_is_empty - test if a mailbox is empty.
 *
 * The function MSG_mailbox_is_empty tests if a mailbox is empty (contains no msg task). 
 *
 * \param mailbox	The mailbox to get test.
 *
 * \return			The function returns 1 if the mailbox is empty. Otherwise the function
 *					returns 0.
 */
int
MSG_mailbox_is_empty(msg_mailbox_t mailbox);

/*! \brief MSG_mailbox_put - put a task in a mailbox.
 *
 * The MSG_mailbox_put puts a task in a specified mailbox.
 *
 * \param mailbox	The mailbox where put the task.
 * \param task		The task to put in the mailbox.
 */
void
MSG_mailbox_put(msg_mailbox_t mailbox, m_task_t task);

/*! \brief MSG_mailbox_remove - remove a task from a mailbox.
 *
 * The MSG_mailbox_remove removes a task from a specified mailbox.
 *
 * \param mailbox	The mailbox concerned by this operation.
 * \param task		The task to remove from the mailbox.
 */
void
MSG_mailbox_remove(msg_mailbox_t mailbox, m_task_t task);

/*! \brief MSG_mailbox_get_head - get the task at the head of a mailbox.
 *
 * The MSG_mailbox_get_head returns the task at the head of the mailbox.
 * This function does not remove the task from the mailbox (contrary to
 * the function MSG_mailbox_pop_head).
 *
 * \param mailbox	The mailbox concerned by the operation.
 *
 * \return			The task at the head of the mailbox.
 */
m_task_t
MSG_mailbox_get_head(msg_mailbox_t mailbox);

/*! \brief MSG_mailbox_pop_head - get the task at the head of a mailbox
 * and remove it from it.
 *
 * The MSG_mailbox_pop_head returns the task at the head of the mailbox
 * and remove it from it.
 *
 * \param mailbox	The mailbox concerned by the operation.
 *
 * \return			The task at the head of the mailbox.
 */
m_task_t
MSG_mailbox_pop_head(msg_mailbox_t mailbox);

/*! \brief MSG_mailbox_get_first_host_task - get the first msg task
 * of a specified mailbox, sended by a process of a specified host.
 *
 * \param mailbox	The mailbox concerned by the operation.
 * \param host		The msg host of the process that has sended the
 *					task.
 *
 * \return			The first task in the mailbox specified by the
 *					parameter mailbox and sended by a process located
 *					on the host specified by the parameter host.
 */
m_task_t
MSG_mailbox_get_first_host_task(msg_mailbox_t mailbox, m_host_t host);

/*! \brief MSG_mailbox_get_count_host_tasks - get the number of msg tasks
 * of a specified mailbox, sended by all the process of a specified host.
 *
 * \param mailbox	The mailbox concerned by the operation.
 * \param host		The msg host containing the processes that have sended the
 *					tasks.
 *
 * \return			The number of tasks in the mailbox specified by the
 *					parameter mailbox and sended by all the processes located
 *					on the host specified by the parameter host.
 */
int
MSG_mailbox_get_count_host_tasks(msg_mailbox_t mailbox, m_host_t host);


#ifdef __cplusplus
}
#endif

#endif /* !SMX_MAILBOX_H */

