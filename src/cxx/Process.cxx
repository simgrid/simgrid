#include <Process.hpp>

namespace msg
{
	
Process* Process::currentProcess = NULL;

// Default constructor.
Process::Process()
{
	this->nativeProcess = NULL;
}

Process::Process(const char* hostname, const char* name)
throw(HostNotFoundException)
{
	Host host = Host::getByName(hostname);
		
	create(host, name, 0, NULL);	
}

Process::Process(const char* hostname, const char* name, int argc, char** argv)
throw(HostNotFoundException)
{
	Host host = Host::getByName(hostname);
		
	create(host, name, argc, argv);	
}

Process::Process(const Host& rHost, const char* name)
throw(HostNotFoundException)
{
	
	create(rHost, name, 0, NULL);	
}

Process::Process(const Host& rHost, const char* name, int argc, char** argv)
throw(HostNotFoundException)
{
	
	create(rHost, name, argc, argv);	
}

int Process::run(int argc, char** argv)
{
	Process* process =(Process*)argv[argc];
	
	return process->main(argc, argv);
}

void Process::create(const Host& rHost, const char* name, int argc, char** argv)
throw(HostNotFoundException)
{
	smx_process_t nativeCurrentProcess = NULL;
	nativeProcess = xbt_new0(s_smx_process_t, 1);
	smx_simdata_process_t simdata = xbt_new0(s_smx_simdata_process_t, 1);
	smx_host_t nativeHost = SIMIX_host_get_by_name(rHost.getName());
	
	argv = (char**)realloc(argc + 1, sizeo(char*));
	
	argv[argc] = (char*)this;
	
	
	// TODO throw HostNotFoundException if host is NULL
	
	// Simulator Data
	simdata->smx_host = nativeHost;
	simdata->mutex = NULL;
	simdata->cond = NULL;
	simdata->argc = argc;
	simdata->argv = argv;
	simdata->context = xbt_context_new(name, Process::run, NULL, NULL, simix_global->cleanup_process_function, nativeProcess, simdata->argc, simdata->argv);
	
	/* Process structure */
	nativeProcess->name = xbt_strdup(name);
	nativeProcess->simdata = simdata;
	
	// Set process data
	nativeProcess->data = NULL;
	
	// Set process properties
	simdata->properties = NULL;
	
	xbt_swag_insert(nativeProcess, nativeHost->simdata->process_list);
	
	/* fix current_process, about which xbt_context_start mocks around */
	nativeCurrentProcess = simix_global->current_process;
	xbt_context_start(nativeProcess->simdata->context);
	simix_global->current_process = nativeCurrentProcess;
	
	xbt_swag_insert(nativeProcess, simix_global->process_list);
	DEBUG2("Inserting %s(%s) in the to_run list", nativeProcess->name, nativeHost->name);
	xbt_swag_insert(nativeProcess, simix_global->process_to_run);
}


int Process::killAll(int resetPID) 
{
    return MSG_process_killall(resetPID);
}

void Process::suspend(void)
throw(NativeException)
{
    if(MSG_OK != MSG_process_suspend(nativeProcess)) 
    {
    	// TODO throw NativeException.
    }
}

void Process::resume(void) 
throw(NativeException)
{
	if(MSG_OK != MSG_process_resume(nativeProcess))
	{
		// TODO throw NativeException.
	}
}


bool Process::isSuspended(void)
{
   return (bool)MSG_process_is_suspended(nativeProcess);
}  


Host& Process::getHost(void) 
throw(NativeException) 
{
  m_host_t nativeHost = MSG_process_get_host(nativeProcess);
	
  if(!nativeHost->data) 
  {
    // TODO throw NativeException.
    return NULL;
  }

  // return the reference to the Host object
  return (*((Host*)nativeHost->data));
} 

Process& Process::fromPID(int PID) 
throw(ProcessNotFoundException, NativeException)
{
	Process* process = NULL;
	m_process_t nativeProcess = MSG_process_from_PID(PID);
	
	
	if(!process) 
	{
		throw ProcessNotFoundException;
		return NULL;
	}
	
	process = Process::fromNativeProcess(nativeProcess);
		
	if(!process) 
	{
		// 	TODO throw NativeException
		return NULL;
	}
	
	return (*process);   
}

// TODO implement this method
Process& Process::fromNativeProcess(m_process_t nativeProcess)
{
	
}

int Process::getPID(void)
{
    return MSG_process_get_PID(nativeProcess);
}

int Process::getPPID(void)
{
	return MSG_process_get_PPID(nativeProcess);
}


Process& Process::currentProcess(void)
throw(NativeException)
{
	Process* currentProcess = NULL;
    m_process_t currentNativeProcess = MSG_process_self();


  	if(!currentNativeProcess) 
  	{
    	// TODO throw NativeException
  	}
  	
  	currentProcess = Process::fromNativeProcess(currentNativeProcess);
		
	if(!currentProcess) 
	{
		// 	TODO throw NativeException
		return NULL;
	}
	
	return (*currentProcess);  
}

int Process::currentProcessPID(void)
{
	 return MSG_process_self_PID();
}


int Process::currentProcessPPID(void)
{
	return MSG_process_self_PPID();
}

void Process::migrate(const Host& rHost)
throw(NativeException)
{
	if(MSG_OK != MSG_process_change_host(nativeProcess, rHost.nativeHost))
	{
		// TODO throw NativeException
	}
	
}

void Process::sleep(double seconds)
throw(NativeException)
{
	if(MSG_OK != MSG_process_sleep(seconds))
	{
		// TODO throw NativeException
	}
   	
}

void Process::putTask(const Host& rHost, int channel, const Task& rTask)
throw( NativeException)
{
	if(MSG_OK != MSG_task_put_with_timeout(rTask.nativeTask, rHost.nativeHost, channel, -1.0))
	{
		// TODO throw NativeException
	}
}

void Process::putTask(const Host& rHost, int channel, const Task& rTask, double timeout) 
throw(NativeException)
{
	if(MSG_OK != MSG_task_put_with_timeout(rTask.nativeTask, rHost.nativeHost, channel, timeout))
	{
		// TODO throw NativeException
	}
}

Task& Process::getTask(int channel) 
throw(NativeException)
{
	m_task_t nativeTask = NULL;
	
	
	if (MSG_OK != MSG_task_get_ext(&nativeTask, channel, -1.0, NULL)) 
	{
		// TODO throw NativeException
		return NULL;
	}
	
	return (*((Task*)(nativeTask->data)));
}

Task& Process::getTask(int channel, double timeout) 
throw(NativeException)
{
	m_task_t nativeTask = NULL;
	
	
	if (MSG_OK != MSG_task_get_ext(&nativeTask, channel, timeout, NULL)) 
	{
		// TODO throw NativeException
		return NULL;
	}
	
	return (*((Task*)(nativeTask->data)));
}

Task& Process::getTask(int channel, const Host& rHost) 
throw(NativeException)
{
	m_task_t nativeTask = NULL;
	
	
	if (MSG_OK != MSG_task_get_ext(&nativeTask, channel, -1.0, rHost.nativeHost)) 
	{
		// TODO throw NativeException
		return NULL;
	}
	
	return (*((Task*)(nativeTask->data)));
}

Task& Process::getTask(int channel, double timeout, const Host& rHost)
throw(NativeException)
{
	m_task_t nativeTask = NULL;
	
	
	if (MSG_OK != MSG_task_get_ext(&nativeTask, channel, timeout, rHost.nativeHost)) 
	{
		// TODO throw NativeException
		return NULL;
	}
	
	return (*((Task*)(nativeTask->data)));
}

void Process::sendTask(const char* alias, const Task& rTask, double timeout) 
throw(NativeException)
{
	if(MSG_OK != MSG_task_send_with_timeout(rTask.nativeTask, alias ,timeout))
	{
		// TODO throw NativeException
	}
		
}

void Process::sendTask(const char* alias, const Task& rTask) 
throw(NativeException)
{
	if(MSG_OK != MSG_task_send_with_timeout(rTask.nativeTask, alias ,-1.0))
	{
		// TODO throw NativeException
	}
}

void Process::sendTask(const Task& rTask) 
throw(NativeException)
{
	MSG_error_t rv;
	
	char* alias = (char*)calloc(strlen(Host::currentHost().getName() + strlen(nativeProcess->name) + 2);
	sprintf(alias,"%s:%s", Host::currentHost().getName() ,nativeProcess->name);
	
	rv = MSG_task_send_with_timeout(rTask.nativeTask, alias ,-1.0);
	
	free(alias);
	
	if(MSG_OK != rv)
	{
		// TODO throw NativeException
	}
}

void Process::sendTask(const Task& rTask, double timeout) 
throw(NativeException)
{
	MSG_error_t rv;
	
	char* alias = (char*)calloc(strlen(Host::currentHost().getName() + strlen(nativeProcess->name) + 2);
	sprintf(alias,"%s:%s", Host::currentHost().getName() ,nativeProcess->name);
	
	rv = MSG_task_send_with_timeout(rTask.nativeTask, alias ,timeout);
	
	free(alias);
	
	if(MSG_OK != rv)
	{
		// TODO throw NativeException
	}
}

Task& Process::receiveTask(const char* alias) 
throw(NativeException)
{
	
	m_task_t nativeTask = NULL;
	
	if (MSG_OK !=  MSG_task_receive_ext(&nativeTask,alias, -1.0, NULL)) 
	{
		// TODO throw NativeException
		return NULL;
	}

	return (*((Task*)nativeTask->data));
}


Task& Process::receiveTask(void) 
throw(NativeException)
{
	m_task_t nativeTask = NULL;
	
	char* alias = (char*)calloc(strlen(Host::currentHost().getName() + strlen(nativeProcess->name) + 2);
	sprintf(alias,"%s:%s", Host::currentHost().getName() ,nativeProcess->name);
	
	MSG_error_t rv = MSG_task_receive_ext(&nativeTask, alias, -1.0, NULL);
	
	free(alias);
	
	if(MSG_OK !=  rv) 
	{
		//TODO throw NativeException
		return NULL;
	}

	return (*((Task*)nativeTask->data));
}


Task& Process::receiveTask(const char* alias, double timeout) 
throw(NativeException)
{
	m_task_t nativeTask = NULL;
	
	if(MSG_OK !=  MSG_task_receive_ext(&nativeTask, alias, timeout, NULL)) 
	{
		//TODO throw NativeException
		return NULL;
	}

	return (*((Task*)nativeTask->data));
}


Task& Process::receiveTask(double timeout) 
throw(NativeException)
{
	m_task_t nativeTask = NULL;
	
	char* alias = (char*)calloc(strlen(Host::currentHost().getName() + strlen(nativeProcess->name) + 2);
	sprintf(alias,"%s:%s", Host::currentHost().getName() ,nativeProcess->name);
	
	MSG_error_t rv = MSG_task_receive_ext(&nativeTask, alias, timeout, NULL);
	
	free(alias);
	
	if(MSG_OK !=  rv) 
	{
		//TODO throw NativeException
		return NULL;
	}

	return (*((Task*)nativeTask->data));
}


Task& Process::receiveTask(const char* alias, double timeout, const Host& rHost) 
throw(NativeException)
{
	m_task_t nativeTask = NULL;
	
	if(MSG_OK !=  MSG_task_receive_ext(&nativeTask, alias, timeout, rHost.nativeHost)) 
	{
		//TODO throw NativeException
		return NULL;
	}

	return (*((Task*)nativeTask->data));
}


Task& Process::receiveTask(double timeout, const Host& rHost) 
throw(NativeException)
{
	m_task_t nativeTask = NULL;
	
	char* alias = (char*)calloc(strlen(Host::currentHost().getName() + strlen(nativeProcess->name) + 2);
	sprintf(alias,"%s:%s", Host::currentHost().getName() ,nativeProcess->name);
	
	MSG_error_t rv = MSG_task_receive_ext(&nativeTask, alias, timeout, rHost.nativeHost);
	
	free(alias);
	
	if(MSG_OK !=  rv) 
	{
		//TODO throw NativeException
		return NULL;
	}

	return (*((Task*)nativeTask->data));
}


Task& Process::receiveTask(const char* alias, const Host& rHost) 
throw(NativeException)
{
	
	m_task_t nativeTask = NULL;
	

	if(MSG_OK !=   MSG_task_receive_ext(&nativeTask, alias, -1.0, rHost.nativeHost)) 
	{
		//TODO throw NativeException
		return NULL;
	}

	return (*((Task*)nativeTask->data));
}

Task& Process::receiveTask(const Host& rHost) 
throw(NativeException)
{
	m_task_t nativeTask = NULL;
	
	char* alias = (char*)calloc(strlen(Host::currentHost().getName() + strlen(nativeProcess->name) + 2);
	sprintf(alias,"%s:%s", Host::currentHost().getName() ,nativeProcess->name);
	
	MSG_error_t rv = MSG_task_receive_ext(&nativeTask, alias, -1.0, rHost.nativeHost);
	
	free(alias);
	
	if(MSG_OK !=  rv) 
	{
		//TODO throw NativeException
		return NULL;
	}

	return (*((Task*)nativeTask->data));
}

const char* Process::getName(void) const
{
	return nativeProcess->name;
}


}