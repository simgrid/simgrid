#ifndef _XBT_CONTEXT_PRIVATE_H
#define _XBT_CONTEXT_PRIVATE_H

#include "xbt/sysdep.h"
#include "xbt/xbt_context.h"

SG_BEGIN_DECL()

/* the following function pointers describe the interface that all context concepts must implement */

typedef void (*xbt_pfn_context_free_t)(xbt_context_t);		/* pointer type to the function used to destroy the specified context	*/
typedef void (*xbt_pfn_context_kill_t)(xbt_context_t);		/* pointer type to the function used to kill the specified context		*/
typedef void (*xbt_pfn_context_schedule_t)(xbt_context_t);	/* pointer type to the function used to resume the specified context	*/
typedef void (*xbt_pfn_context_yield_t)(void);				/* pointer type to the function used to yield the specified context		*/
typedef void (*xbt_pfn_context_start_t)(xbt_context_t);		/* pointer type to the function used to start the specified context		*/
typedef void (*xbt_pfn_context_stop_t)(int);				/* pointer type to the function used to stop the current context		*/

/* each context concept must use this macro in its declaration */
#define XBT_CTX_BASE_T \
	s_xbt_swag_hookup_t hookup; \
	char *name; \
	void_f_pvoid_t cleanup_func; \
	void *cleanup_arg; \
	ex_ctx_t *exception; \
	int iwannadie; \
	xbt_main_func_t code; \
	int argc; \
	char **argv; \
	void_f_pvoid_t startup_func; \
	void *startup_arg; \
	xbt_pfn_context_free_t free; \
	xbt_pfn_context_kill_t kill; \
	xbt_pfn_context_schedule_t schedule; \
	xbt_pfn_context_yield_t yield; \
	xbt_pfn_context_start_t start; \
	xbt_pfn_context_stop_t stop				

/* all other contexts derive from this structure */
typedef struct s_xbt_context
{
	XBT_CTX_BASE_T;
}s_xbt_context_t;

SG_END_DECL()
	

#ifdef CONTEXT_THREADS
#include "xbt_thread_context.h"	/* thread based context declarations 		*/
#elif !defined(WIN32)
#include "xbt_ucontext.h"		/* ucontext based context declarations		*/
#else
#error ERROR [__FILE__, line __LINE__]: no context based implementation specified.
#endif

#include "xbt_jcontext.h"		/* java thread based context declarations	*/	

#endif /* !_XBT_CONTEXT_PRIVATE_H */
