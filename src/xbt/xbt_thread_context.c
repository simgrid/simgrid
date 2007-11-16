
#include "xbt/function_types.h"
#include "xbt/xbt_context_factory.h"
#include "xbt/xbt_thread_context.h"

static xbt_context_t 
xbt_thread_context_factory_create_context(const char* name, xbt_main_func_t code, void_f_pvoid_t startup_func, void* startup_arg, void_f_pvoid_t cleanup_func, void* cleanup_arg, int argc, char** argv);


static int
xbt_thread_context_factory_create_master_context(xbt_context_t* maestro);

static int
xbt_thread_context_factory_finalize(xbt_context_factory_t* factory);

static void 
xbt_thread_context_free(xbt_context_t context);

static void 
xbt_thread_context_kill(xbt_context_t context);

static void 
xbt_thread_context_schedule(xbt_context_t context);

static void 
xbt_thread_context_yield(void);

static void 
xbt_thread_context_start(xbt_context_t context);

static void 
xbt_thread_context_stop(int exit_code);

static void 
xbt_thread_context_swap(xbt_context_t context);

static void
xbt_thread_context_schedule(xbt_context_t context);

static void
xbt_thread_context_yield(void);

static void
xbt_thread_context_suspend(xbt_context_t context);

static void
xbt_thread_context_resume(xbt_context_t context);

static void* 
xbt_thread_context_wrapper(void* param);

int
xbt_thread_context_factory_init(xbt_context_factory_t* factory)
{
	*factory = xbt_new0(s_xbt_context_factory_t,1);
	
	(*factory)->create_context = xbt_thread_context_factory_create_context;
	(*factory)->finalize = xbt_thread_context_factory_finalize;
	(*factory)->create_maestro_context = xbt_thread_context_factory_create_master_context;
	(*factory)->name = "thread_context_factory";

	return 0;
}

static int
xbt_thread_context_factory_create_master_context(xbt_context_t* maestro)
{
	*maestro = (xbt_context_t)xbt_new0(s_xbt_thread_context_t, 1);
    return 0;
}

static int
xbt_thread_context_factory_finalize(xbt_context_factory_t* factory)
{
	free(*factory);
	*factory = NULL;
	return 0;
}

static xbt_context_t 
xbt_thread_context_factory_create_context(const char* name, xbt_main_func_t code, void_f_pvoid_t startup_func, void* startup_arg, void_f_pvoid_t cleanup_func, void* cleanup_arg, int argc, char** argv)
{
	xbt_thread_context_t context = xbt_new0(s_xbt_thread_context_t, 1);
	
	context->code = code;
	context->name = xbt_strdup(name);
	context->begin = xbt_os_sem_init(0);
	context->end = xbt_os_sem_init(0);
	context->iwannadie = 0;           /* useless but makes valgrind happy */
	context->argc = argc;
	context->argv = argv;
	context->startup_func = startup_func;
	context->startup_arg = startup_arg;
	context->cleanup_func = cleanup_func;
	context->cleanup_arg = cleanup_arg;
	
	context->free = xbt_thread_context_free;			
	context->kill = xbt_thread_context_kill;			
	context->schedule = xbt_thread_context_schedule;
	context->yield = xbt_thread_context_yield;			
	context->start = xbt_thread_context_start;			
	context->stop = xbt_thread_context_stop;			
	
	return (xbt_context_t)context;
}

static void 
xbt_thread_context_free(xbt_context_t context)
{
	if(context)
	{
		xbt_thread_context_t thread_context = (xbt_thread_context_t)context;
		
		free(thread_context->name);
		
		if(thread_context->argv)
		{
			int i;
			
			for(i = 0; i < thread_context->argc; i++)
				if(thread_context->argv[i])
					free(thread_context->argv[i]);
					
			free(thread_context->argv);
		}
		
		/* wait about the thread terminason */
		xbt_os_thread_join(thread_context->thread, NULL);
		
		/* destroy the synchronisation objects */
		xbt_os_sem_destroy(thread_context->begin);
		xbt_os_sem_destroy(thread_context->end);
		
		/* finally destroy the context */
		free(context);
	}
}

static void 
xbt_thread_context_kill(xbt_context_t context)
{
	context->iwannadie = 1;
	xbt_thread_context_swap(context);
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
xbt_thread_context_schedule(xbt_context_t context)
{
	xbt_assert0((current_context == maestro_context),"You are not supposed to run this function here!");
	xbt_thread_context_swap(context);
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
xbt_thread_context_yield(void)
{
	xbt_assert0((current_context != maestro_context),"You are not supposed to run this function here!");
	xbt_thread_context_swap(current_context);
}

static void 
xbt_thread_context_start(xbt_context_t context)
{
	xbt_thread_context_t thread_context = (xbt_thread_context_t)context;
	
	/* create and start the process */
	thread_context->thread = xbt_os_thread_create(thread_context->name,xbt_thread_context_wrapper,thread_context);
	
	/* wait the starting of the newly created process */
	xbt_os_sem_acquire(thread_context->end);
}

static void 
xbt_thread_context_stop(int exit_code)
{
	if(current_context->cleanup_func) 
		((*current_context->cleanup_func))(current_context->cleanup_arg);
	
	xbt_swag_remove(current_context, context_living);
	xbt_swag_insert(current_context, context_to_destroy);	

	/* signal to the maestro that it has finished */
	xbt_os_sem_release(((xbt_thread_context_t)current_context)->end);
	
	/* exit*/
	xbt_os_thread_exit(NULL);	/* We should provide return value in case other wants it */
}

static void 
xbt_thread_context_swap(xbt_context_t context)
{
	if((current_context != maestro_context) && !context->iwannadie)
	{
		/* (0) it's not the scheduler and the process doesn't want to die, it just wants to yield */
		
		/* yield itself, resume the maestro */
		xbt_thread_context_suspend(context);
	}
	else
	{
		/* (1) the current process is the scheduler and the process doesn't want to die
		 *	<-> the maestro wants to schedule the process
		 *		-> the maestro schedules the process and waits
		 *
		 * (2) the current process is the scheduler and the process wants to die
		 *	<-> the maestro wants to kill the process (has called the function xbt_context_kill())
		 *		-> the maestro schedule the process and waits (xbt_os_sem_acquire(context->end))
		 *		-> if the process stops (xbt_context_stop())
		 *			-> the process resumes the maestro (xbt_os_sem_release(current_context->end)) and exit (xbt_os_thread_exit())
		 *		-> else the process call xbt_context_yield()
		 *			-> goto (3.1)
		 *
		 * (3) the current process is not the scheduler and the process wants to die
		 *		-> (3.1) if the current process is the process who wants to die
		 *			-> (resume not need) goto (4)
		 *		-> (3.2) else the current process is not the process who wants to die
		 *			<-> the current process wants to kill an other process
		 *				-> the current process resumes the process to die and waits
		 *				-> if the process to kill stops
		 *					-> it resumes the process who kill it and exit
		 *				-> else if the process to kill calls to xbt_context_yield()
		 *					-> goto (3.1)
		 */
		/* schedule the process associated with this context */
		xbt_thread_context_resume(context);
	
	}
	
	/* (4) the current process wants to die */
	if(current_context->iwannadie)
		xbt_thread_context_stop(1);
}

static void* 
xbt_thread_context_wrapper(void* param)
{
	xbt_thread_context_t context = (xbt_thread_context_t)param;
	
	/* signal its starting to the maestro and wait to start its job*/
	xbt_os_sem_release(context->end);		
 	xbt_os_sem_acquire(context->begin);
	
	if (context->startup_func)
		(*(context->startup_func))(context->startup_arg);
	
	
	xbt_thread_context_stop((context->code) (context->argc, context->argv));
	return NULL;
}

static void
xbt_thread_context_suspend(xbt_context_t context)
{
	/* save the current context */
	xbt_context_t self = current_context;
	
	/* update the current context to this context */
	current_context = context;
	
	xbt_os_sem_release(((xbt_thread_context_t)context)->end);	
	xbt_os_sem_acquire(((xbt_thread_context_t)context)->begin);
	
	/* restore the current context to the previously saved context */
	current_context = self;		
}

static void
xbt_thread_context_resume(xbt_context_t context)
{
	/* save the current context */
	xbt_context_t self = current_context;
	
	/* update the current context */
	current_context = context;
	
	xbt_os_sem_release(((xbt_thread_context_t)context)->begin);		
 	xbt_os_sem_acquire(((xbt_thread_context_t)context)->end);
 	
 	/* restore the current context to the previously saved context */
	current_context = self;
}



