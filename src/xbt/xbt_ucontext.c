
#include "ucontext_stack.h"
#include "xbt/ex_interface.h"
#include "xbt/xbt_context_factory.h"
#include "xbt/xbt_ucontext.h"

/* callback: context fetching */
static ex_ctx_t*
xbt_jcontext_ex_ctx(void);

/* callback: termination */
static void 
xbt_jcontext_ex_terminate(xbt_ex_t *e);

static xbt_context_t 
xbt_ucontext_factory_create_context(const char* name, xbt_main_func_t code, void_f_pvoid_t startup_func, void* startup_arg, void_f_pvoid_t cleanup_func, void* cleanup_arg, int argc, char** argv);

static int
xbt_ucontext_factory_finalize(xbt_context_factory_t* factory);

static int 
xbt_ucontext_factory_create_maestro_context(xbt_context_t* maestro);

static void 
xbt_ucontext_free(xbt_context_t context);

static void 
xbt_ucontext_kill(xbt_context_t context);

static void 
xbt_ucontext_schedule(xbt_context_t context);

static void 
xbt_ucontext_yield(void);

static void 
xbt_ucontext_start(xbt_context_t context);

static void 
xbt_ucontext_stop(int exit_code);

static void 
xbt_ucontext_swap(xbt_context_t context);

static void
xbt_ucontext_schedule(xbt_context_t context);

static void
xbt_ucontext_yield(void);

static void
xbt_ucontext_suspend(xbt_context_t context);

static void
xbt_ucontext_resume(xbt_context_t context);

static void* 
xbt_ucontext_wrapper(void* param);

/* callback: context fetching */
static ex_ctx_t*
xbt_ucontext_ex_ctx(void) 
{
	return current_context->exception;
}

/* callback: termination */
static void 
xbt_ucontext_ex_terminate(xbt_ex_t *e) 
{
	xbt_ex_display(e);
	abort();
}


int
xbt_ucontext_factory_init(xbt_context_factory_t* factory)
{
	/* context exception */
	*factory = xbt_new0(s_xbt_context_factory_t,1);
	
	(*factory)->create_context = xbt_ucontext_factory_create_context;
	(*factory)->finalize = xbt_ucontext_factory_finalize;
	(*factory)->create_maestro_context = xbt_ucontext_factory_create_maestro_context;
	(*factory)->name = "ucontext_context_factory";
	
	/* context exception handlers */
	__xbt_ex_ctx       = xbt_ucontext_ex_ctx;
    __xbt_ex_terminate = xbt_ucontext_ex_terminate;	
    
	return 0;
}

static int 
xbt_ucontext_factory_create_maestro_context(xbt_context_t* maestro)
{
		
	xbt_ucontext_t context = xbt_new0(s_xbt_ucontext_t, 1);
	
	context->exception = xbt_new(ex_ctx_t,1);
    XBT_CTX_INITIALIZE(context->exception);
    
    *maestro = (xbt_context_t)context;
    
    return 0;
    
}


static int
xbt_ucontext_factory_finalize(xbt_context_factory_t* factory)
{
	free(maestro_context->exception);
	free(*factory);
	*factory = NULL;
	return 0;
}

static xbt_context_t 
xbt_ucontext_factory_create_context(const char* name, xbt_main_func_t code, void_f_pvoid_t startup_func, void* startup_arg, void_f_pvoid_t cleanup_func, void* cleanup_arg, int argc, char** argv)
{
	xbt_ucontext_t context = xbt_new0(s_xbt_ucontext_t, 1);
	
	context->code = code;
	context->name = xbt_strdup(name);
	
	xbt_assert2(getcontext(&(context->uc)) == 0,"Error in context saving: %d (%s)", errno, strerror(errno));
	context->uc.uc_link = NULL;
	context->uc.uc_stack.ss_sp = pth_skaddr_makecontext(context->stack, STACK_SIZE);
	context->uc.uc_stack.ss_size = pth_sksize_makecontext(context->stack, STACK_SIZE);
	
	context->exception = xbt_new(ex_ctx_t, 1);
	XBT_CTX_INITIALIZE(context->exception);
	context->iwannadie = 0;           /* useless but makes valgrind happy */
	context->argc = argc;
	context->argv = argv;
	context->startup_func = startup_func;
	context->startup_arg = startup_arg;
	context->cleanup_func = cleanup_func;
	context->cleanup_arg = cleanup_arg;
	
	
	context->free = xbt_ucontext_free;			
	context->kill = xbt_ucontext_kill;			
	context->schedule = xbt_ucontext_schedule;
	context->yield = xbt_ucontext_yield;			
	context->start = xbt_ucontext_start;			
	context->stop = xbt_ucontext_stop;			
	
	return (xbt_context_t)context;
}

static void 
xbt_ucontext_free(xbt_context_t context)
{
	if(context)
	{
		free(context->name);
		
		if(context->argv)
		{
			int i;
			
			for(i = 0; i < context->argc; i++)
				if(context->argv[i])
					free(context->argv[i]);
					
			free(context->argv);
		}
		
		if(context->exception) 
			free(context->exception);
		
		/* finally destroy the context */
		free(context);
	}
}

static void 
xbt_ucontext_kill(xbt_context_t context)
{
	context->iwannadie = 1;
	xbt_ucontext_swap(context);
}

/** 
 * \param context the winner
 *
 * Calling this function blocks the current context and schedule \a context.  
 * When \a context will call xbt_context_yield, it will return
 * to this function as if nothing had happened.
 * 
 * Only the maestro can call this function to run a given process.
 */
static void 
xbt_ucontext_schedule(xbt_context_t context)
{
	xbt_assert0((current_context == maestro_context),"You are not supposed to run this function here!");
	xbt_ucontext_swap(context);
}

/** 
 * Calling this function makes the current context yield. The context
 * that scheduled it returns from xbt_context_schedule as if nothing
 * had happened.
 * 
 * Only the processes can call this function, giving back the control
 * to the maestro
 */
static void 
xbt_ucontext_yield(void)
{
	xbt_assert0((current_context != maestro_context),"You are not supposed to run this function here!");
	xbt_ucontext_swap(current_context);
}

static void 
xbt_ucontext_start(xbt_context_t context)
{
	makecontext(&(((xbt_ucontext_t)context)->uc), (void (*)(void)) xbt_ucontext_wrapper, 1, context);
}

static void 
xbt_ucontext_stop(int exit_code)
{
	if(current_context->cleanup_func) 
		((*current_context->cleanup_func))(current_context->cleanup_arg);
	
	xbt_swag_remove(current_context, context_living);
	xbt_swag_insert(current_context, context_to_destroy);	
	
	xbt_ucontext_swap(current_context);
}

static void 
xbt_ucontext_swap(xbt_context_t context)
{
	xbt_assert0(current_context, "You have to call context_init() first.");
	xbt_assert0(context, "Invalid argument");
	
	if(((xbt_ucontext_t)context)->prev == NULL) 
		xbt_ucontext_resume(context);
	else 
		xbt_ucontext_suspend(context);
	
	if(current_context->iwannadie)
		xbt_ucontext_stop(1);
}

static void* 
xbt_ucontext_wrapper(void* param)
{
	if (current_context->startup_func)
		(*current_context->startup_func)(current_context->startup_arg);
	
	xbt_ucontext_stop((*(current_context->code))(current_context->argc, current_context->argv));
	return NULL;
}

static void
xbt_ucontext_suspend(xbt_context_t context)
{
	int rv;
	
	xbt_ucontext_t prev_context = ((xbt_ucontext_t)context)->prev;
	
	current_context = (xbt_context_t)(((xbt_ucontext_t)context)->prev);
	
	((xbt_ucontext_t)context)->prev = NULL;
	
	rv = swapcontext(&(((xbt_ucontext_t)context)->uc), &(prev_context->uc));
		
	xbt_assert0((rv == 0), "Context swapping failure");
}

static void
xbt_ucontext_resume(xbt_context_t context)
{
	int rv;
	
	((xbt_ucontext_t)context)->prev = (xbt_ucontext_t)current_context;
	
	current_context = context;
	
	rv = swapcontext(&(((xbt_ucontext_t)context)->prev->uc), &(((xbt_ucontext_t)context)->uc));
		
	xbt_assert0((rv == 0), "Context swapping failure");
}



