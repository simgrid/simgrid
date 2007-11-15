#ifndef _XBT_THREAD_CONTEXT_H
#define _XBT_THREAD_CONTEXT_H

#include "portable.h"  		/* loads context system definitions															*/
#include "xbt/xbt_os_thread.h"		/* declaration of the xbt native semaphore and native thread								*/
#include "xbt/swag.h"


#ifndef _XBT_CONTEXT_PRIVATE_H
/*#include "xbt_context_private.h"*/
#endif /* _XBT_CONTEXT_PRIVATE_H */

SG_BEGIN_DECL()

#ifndef _XBT_CONTEXT_FACTORY_T_DEFINED
typedef struct s_xbt_context_factory* xbt_context_factory_t;
#define _XBT_CONTEXT_FACTORY_T_DEFINED
#endif /* !_XBT_CONTEXT_FACTORY_T_DEFINED */


typedef struct s_xbt_thread_context
{
	XBT_CTX_BASE_T;
   	xbt_os_thread_t thread;			/* a plain dumb thread (portable to posix or windows)										*/
	xbt_os_sem_t begin;				/* this semaphore is used to schedule/yield the process										*/
	xbt_os_sem_t end;				/* this semaphore is used to schedule/unschedule the process								*/
}s_xbt_thread_context_t,* xbt_thread_context_t;

int
xbt_thread_context_factory_init(xbt_context_factory_t* factory);

SG_END_DECL()

#endif /* !_XBT_THREAD_CONTEXT_H */
