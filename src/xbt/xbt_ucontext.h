#ifndef _XBT_UCONTEXT_H
#define _XBT_UCONTEXT_H

#include "ucontext_stack.h"		/* loads context system definitions															*/
#include <ucontext.h>			/* context relative declarations															*/				
#define STACK_SIZE 128*1024		/* lower this if you want to reduce the memory consumption									*/

#ifndef _XBT_CONTEXT_FACTORY_T_DEFINED
typedef struct s_xbt_context_factory* xbt_context_factory_t;
#define _XBT_CONTEXT_FACTORY_T_DEFINED
#endif /* !_XBT_CONTEXT_FACTORY_T_DEFINED */

typedef struct s_xbt_ucontext
{
	XBT_CTX_BASE_T;
   	ucontext_t uc;					/* the thread that execute the code															*/
	char stack[STACK_SIZE];			/* the thread stack size																	*/
	struct s_xbt_ucontext* prev;		/* the previous thread																	*/
}s_xbt_ucontext_t,* xbt_ucontext_t;


int
xbt_ucontext_factory_init(xbt_context_factory_t* factory);

SG_END_DECL()

#endif /* !_XBT_UCONTEXT_H */
