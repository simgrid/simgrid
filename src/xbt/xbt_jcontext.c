

#include "xbt/function_types.h"
#include "xbt/ex_interface.h"
#include "xbt/xbt_context_factory.h"
#include "xbt/xbt_jcontext.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(jmsg,"MSG for Java(TM)");

/* callback: context fetching */
static ex_ctx_t*
xbt_jcontext_ex_ctx(void);

/* callback: termination */
static void 
xbt_jcontext_ex_terminate(xbt_ex_t *e);

static xbt_context_t 
xbt_jcontext_factory_create_context(const char* name, xbt_main_func_t code, void_f_pvoid_t startup_func, void* startup_arg, void_f_pvoid_t cleanup_func, void* cleanup_arg, int argc, char** argv);

static int 
xbt_jcontext_factory_create_maestro_context(xbt_context_t* maestro);

static int
xbt_jcontext_factory_finalize(xbt_context_factory_t* factory);

static void 
xbt_jcontext_free(xbt_context_t context);

static void 
xbt_jcontext_kill(xbt_context_t context);

static void 
xbt_jcontext_schedule(xbt_context_t context);

static void 
xbt_jcontext_yield(void);

static void 
xbt_jcontext_start(xbt_context_t context);

static void 
xbt_jcontext_stop(int exit_code);

static void 
xbt_jcontext_swap(xbt_context_t context);

static void
xbt_jcontext_schedule(xbt_context_t context);

static void
xbt_jcontext_yield(void);

static void
xbt_jcontext_suspend(xbt_context_t context);

static void
xbt_jcontext_resume(xbt_context_t context);


/* callback: context fetching */
static ex_ctx_t*
xbt_jcontext_ex_ctx(void) 
{
	return current_context->exception;
}

/* callback: termination */
static void 
xbt_jcontext_ex_terminate(xbt_ex_t *e) 
{
	xbt_ex_display(e);
	abort();
}

int
xbt_jcontext_factory_init(xbt_context_factory_t* factory)
{
	/* context exception handlers */
    __xbt_ex_ctx       = xbt_jcontext_ex_ctx;
    __xbt_ex_terminate = xbt_jcontext_ex_terminate;	
    
    /* instantiate the context factory */
	*factory = xbt_new0(s_xbt_context_factory_t,1);
	
	(*factory)->create_context = xbt_jcontext_factory_create_context;
	(*factory)->finalize = xbt_jcontext_factory_finalize;
	(*factory)->create_maestro_context = xbt_jcontext_factory_create_maestro_context;
	(*factory)->name = "jcontext_factory";
	
	return 0;
}

static int 
xbt_jcontext_factory_create_maestro_context(xbt_context_t* maestro)
{
	xbt_jcontext_t context = xbt_new0(s_xbt_jcontext_t, 1);
	
	context->exception = xbt_new(ex_ctx_t,1);
    XBT_CTX_INITIALIZE(context->exception);
    
    *maestro = (xbt_context_t)context;
    
    return 0;
}

static int
xbt_jcontext_factory_finalize(xbt_context_factory_t* factory)
{
	free(maestro_context->exception);
	free(*factory);
	*factory = NULL;

	return 0;
}

static xbt_context_t 
xbt_jcontext_factory_create_context(const char* name, xbt_main_func_t code, void_f_pvoid_t startup_func, void* startup_arg, void_f_pvoid_t cleanup_func, void* cleanup_arg, int argc, char** argv)
{
	xbt_jcontext_t context = xbt_new0(s_xbt_jcontext_t,1);

	context->name = xbt_strdup(name);

	context->cleanup_func = cleanup_func;
	context->cleanup_arg = cleanup_arg;

	context->exception = xbt_new(ex_ctx_t,1);
	XBT_CTX_INITIALIZE(context->exception);

	context->free = xbt_jcontext_free;			
	context->kill = xbt_jcontext_kill;			
	context->schedule = xbt_jcontext_schedule;
	context->yield = xbt_jcontext_yield;			
	context->start = xbt_jcontext_start;			
	context->stop = xbt_jcontext_stop;	
	context->jprocess = (jobject)startup_arg;
	context->jenv = get_current_thread_env();

	return (xbt_context_t)context;
}

static void 
xbt_jcontext_free(xbt_context_t context)
{
	if(context) 
	{
		xbt_jcontext_t jcontext = (xbt_jcontext_t)context;
		
		free(jcontext->name);
		
		if(jcontext->jprocess) 
		{
			jobject jprocess = jcontext->jprocess;
			jcontext->jprocess = NULL;

			/* if the java process is alive join it */
			if(jprocess_is_alive(jprocess,get_current_thread_env())) 
				jprocess_join(jprocess,get_current_thread_env());
		}

		if(jcontext->exception) 
			free(jcontext->exception);

		free(context);
		context = NULL;
  	}
}

static void 
xbt_jcontext_kill(xbt_context_t context)
{
	context->iwannadie = 1;
	xbt_jcontext_swap(context);
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
xbt_jcontext_schedule(xbt_context_t context)
{
	xbt_assert0((current_context == maestro_context),"You are not supposed to run this function here!");
	xbt_jcontext_swap(context);
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
xbt_jcontext_yield(void)
{
	xbt_assert0((current_context != maestro_context),"You are not supposed to run this function here!");
	jprocess_unschedule(current_context);
}

static void 
xbt_jcontext_start(xbt_context_t context)
{
	jprocess_start(((xbt_jcontext_t)context)->jprocess,get_current_thread_env());
}

static void 
xbt_jcontext_stop(int exit_code)
{
	jobject jprocess = NULL;
	xbt_jcontext_t jcontext;

	if(current_context->cleanup_func)
	    	(*(current_context->cleanup_func))(current_context->cleanup_arg);

	xbt_swag_remove(current_context, context_living);
	xbt_swag_insert(current_context, context_to_destroy);
	
	jcontext = (xbt_jcontext_t)current_context;
	
	if(jcontext->iwannadie)
	{
		/* The maestro call xbt_context_stop() with an exit code set to one */
		if(jcontext->jprocess) 
		{
			/* if the java process is alive schedule it */
			if(jprocess_is_alive(jcontext->jprocess,get_current_thread_env())) 
			{
				jprocess_schedule(current_context);
				jprocess = jcontext->jprocess;
				jcontext->jprocess = NULL;
				
				/* interrupt the java process */
				jprocess_exit(jprocess,get_current_thread_env());

			}
		}
	}
	else
	{
		/* the java process exits */
		jprocess = jcontext->jprocess;
  		jcontext->jprocess = NULL;
	}

	/* delete the global reference associated with the java process */
	jprocess_delete_global_ref(jprocess,get_current_thread_env());	
}

static void 
xbt_jcontext_swap(xbt_context_t context)
{
	if(context) 
	{
		xbt_context_t self = current_context;
		
		current_context = context;
		
		jprocess_schedule(context);
		
		current_context = self;
	}
	
	if(current_context->iwannadie)
		xbt_jcontext_stop(1);
}
