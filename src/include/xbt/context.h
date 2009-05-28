#ifndef XBT_CONTEXT_H
#define XBT_CONTEXT_H

#include "xbt/misc.h"				/* XBT_PUBLIC(), SG_BEGIN_DECL() and SG_END_DECL() definitions	*/
#include "xbt/function_types.h"		/* function pointer types declarations							*/	
#include "xbt_modinter.h"			/* xbt_context_init() and xbt_context_exit() declarations		*/
     SG_BEGIN_DECL()  typedef struct s_xbt_context *xbt_context_t;
XBT_PUBLIC(xbt_context_t) 
xbt_context_new(const char *name, xbt_main_func_t code,
                void_f_pvoid_t startup_func, void *startup_arg,
                void_f_pvoid_t cleanup_func, void *cleanup_arg, int argc,
                char *argv[]);
XBT_PUBLIC(void)  xbt_context_kill(xbt_context_t context);
XBT_PUBLIC(void)  xbt_context_start(xbt_context_t context);
XBT_PUBLIC(void)  xbt_context_yield(void);
XBT_PUBLIC(void)  xbt_context_schedule(xbt_context_t context);
     void xbt_context_empty_trash(void);
     void xbt_context_stop(int exit_code);
     void xbt_context_free(xbt_context_t context);
SG_END_DECL()  
#endif  /* !XBT_CONTEXT_H */
