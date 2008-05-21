#include <Process.hpp>

namespace SimGrid
{
	namespace Msg
	{
		Process* Process::currentProcess = NULL;
		
		// Default constructor.
		Process::Process()
		{
			this->nativeProcess = NULL;
		}
		
		Process::Process(const char* hostName, const char* name)
		throw(InvalidArgumentException, HostNotFoundException)
		{
			// check the parameters
			
			if(!name)
				throw NullPointerException("name");
				
			if(!hostName)
				throw NullPointerException("hostName");
			
			Host host = Host::getByName(hostName);
				
			create(host, name, 0, NULL);	
		}
		
		Process::Process(const Host& rHost, const char* name)
		throw(NullPointerException)
		{
			if(!name)
				throw NullPointerException("name");
				
			create(rHost, name, 0, NULL);	
		}
		
		Process::Process(const Host& rHost, const char* name, int argc, char** argv)
		throw(NullPointerException, InvalidArgumentException, LogicException)
		{
			
			// check the parameters
			
			if(!name)
				throw NullPointerException("name");
				
			if(!hostName)
				throw NullPointerException("hostName");
				
			if(argc < 0)
				throw InvalidArgument("argc (must be positive)");
				
			if(!argc && argv)
				throw LogicException("argv is not NULL but argc is zero");
			
			if(argc && !argv)
				throw LogicException("argv is NULL but argc is not zero");
			
			create(rHost, name, argc, argv);	
		}
		
		Process::Process(const char* hostname, const char* name, int argc, char** argv)
		throw(NullPointerException, InvalidArgumentException, LogicException, HostNotFoundException)
		{
			// check the parameters
			
			if(!name)
				throw NullPointerException("name");
				
			if(!hostName)
				throw NullPointerException("hostName");
				
			if(argc < 0)
				throw InvalidArgument("argc (must be positive)");
				
			if(!argc && argv)
				throw LogicException("argv is not NULL but argc is zero");
			
			if(argc && !argv)
				throw LogicException("argv is NULL but argc is not zero");
				
			Host host = Host::getByName(hostname);
				
			create(host, name, argc, argv);	
		}
		
		int Process::killAll(int resetPID) 
		{
		    return MSG_process_killall(resetPID);
		}
		
		void Process::suspend(void)
		throw(MsgException)
		{
		    if(MSG_OK != MSG_process_suspend(nativeProcess)) 
		    	throw MsgException("MSG_process_suspend() failed");
		}
		
		void Process::resume(void) 
		throw(MsgException)
		{
			if(MSG_OK != MSG_process_resume(nativeProcess))
				throw MsgException("MSG_process_resume() failed");
		}
		
		bool Process::isSuspended(void)
		{
		   return (bool)MSG_process_is_suspended(nativeProcess);
		}  
		
		Host& Process::getHost(void) 
		{
		  m_host_t nativeHost = MSG_process_get_host(nativeProcess);
			
		  // return the reference to the Host object
		  return (*((Host*)nativeHost->data));
		}
		
		Process& Process::fromPID(int PID) 
		throw(ProcessNotFoundException, InvalidArgumentException, MsgException)
		{
			// check the parameters
			
			if(PID < 1)
				throw InvalidArgumentException("PID (the PID of the process to retrieve is not less than 1)");
				
			Process* process = NULL;
			m_process_t nativeProcess = MSG_process_from_PID(PID);
			
			if(!nativeProcess) 
				throw ProcessNotFoundException(PID);
			
			process = Process::fromNativeProcess(nativeProcess);
				
			if(!process) 
				throw MsgException("Process::fromNativeProcess() failed");
			
			return (*process);   
		} 
		
		int Process::getPID(void)
		{
		    return MSG_process_get_PID(nativeProcess);
		}
		
		int Process::getPPID(void)
		{
			return MSG_process_get_PPID(nativeProcess);
		}
		
		const char* Process::getName(void) const
		{
			return nativeProcess->name;
		}
		
		Process& Process::currentProcess(void)
		throw(MsgException)
		{
			Process* currentProcess = NULL;
		    m_process_t currentNativeProcess = MSG_process_self();
		
		
		  	if(!currentNativeProcess) 
		  		throw MsgException("MSG_process_self() failed");
		  	
		  	currentProcess = Process::fromNativeProcess(currentNativeProcess);
				
			if(!currentProcess) 
				throw MsgException("Process::fromNativeProcess() failed");
			
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
		throw(MsgException)
		{
			if(MSG_OK != MSG_process_change_host(nativeProcess, rHost.nativeHost))
				throw MsgException("MSG_process_change_host()");
			
		}
		
		void Process::sleep(double seconds)
		throw(throw(InvalidArgumentException, MsgException))
		{
			// check the parameters.
			if(seconds <= 0)
				throw InvalidArgumentException("seconds (must not be less or equals to zero");
				
			if(MSG_OK != MSG_process_sleep(seconds))
				throw MsgException("MSG_process_sleep()");
		   	
		}
		
		void Process::putTask(const Host& rHost, int channel, const Task& rTask)
		throw(InvalidArgumentException, MsgException)
		{
			// check the parameters
			
			if(channel < 0)
				throw InvalidArgumentException("channel (must not be negative)");
				
			if(MSG_OK != MSG_task_put_with_timeout(rTask.nativeTask, rHost.nativeHost, channel, -1.0))
				throw MsgException("MSG_task_put_with_timeout()");
		}
		
		void Process::putTask(const Host& rHost, int channel, const Task& rTask, double timeout) 
		throw(InvalidArgumentException, MsgException)
		{
			// check the parameters
			if(channel < 0)
				throw InvalidArgumentException("channel (must not be negative)");
				
			if(timeout < 0 && timeout != -1.0)
				throw InvalidArgumentException("timeout (must not be less than zero an different of -1.0)");
				
			if(MSG_OK != MSG_task_put_with_timeout(rTask.nativeTask, rHost.nativeHost, channel, timeout))
				throw MsgException("MSG_task_put_with_timeout() failed");
		}
		
		Task& Process::getTask(int channel) 
		throw(InvalidArgumentException, MsgException)
		{
			// check the parameters
			
			if(channel < 0)
				throw InvalidArgumentException("channel (must not be negative)");
			
			m_task_t nativeTask = NULL;
			
			if (MSG_OK != MSG_task_get_ext(&nativeTask, channel, -1.0, NULL)) 
				throw MsgException("MSG_task_get_ext() failed");
			
			return (*((Task*)(nativeTask->data)));
		}
		
		Task& Process::getTask(int channel, double timeout) 
		throw(InvalidArgumentException, MsgException)
		{
			// check the parameters
			if(channel < 0)
				throw InvalidArgumentException("channel (must not be negative)");
				
			if(timeout < 0 && timeout != -1.0)
				throw InvalidArgumentException("timeout (must not be less than zero an different of -1.0)");
			
			m_task_t nativeTask = NULL;
			
			if (MSG_OK != MSG_task_get_ext(&nativeTask, channel, timeout, NULL)) 
				throw MsgException("MSG_task_get_ext() failed");
			
			return (*((Task*)(nativeTask->data)));
		}
		
		Task& Process::getTask(int channel, const Host& rHost) 
		throw(InvalidArgumentException, MsgException)
		{
			// check the parameters
			if(channel < 0)
				throw InvalidArgumentException("channel (must not be negative)");
				
			m_task_t nativeTask = NULL;
			
			if (MSG_OK != MSG_task_get_ext(&nativeTask, channel, -1.0, rHost.nativeHost)) 
				throw MsgException("MSG_task_get_ext() failed");
			
			return (*((Task*)(nativeTask->data)));
		}
		
		Task& Process::getTask(int channel, double timeout, const Host& rHost)
		throw(InvalidArgumentException, MsgException)
		{
			// check the parameters
			if(channel < 0)
				throw InvalidArgumentException("channel (must not be negative)");
				
			if(timeout < 0 && timeout != -1.0)
				throw InvalidArgumentException("timeout (must not be less than zero an different of -1.0)");
			
			m_task_t nativeTask = NULL;	
			
			if (MSG_OK != MSG_task_get_ext(&nativeTask, channel, timeout, rHost.nativeHost)) 
				throw MsgException("MSG_task_get_ext() failed");
			
			return (*((Task*)(nativeTask->data)));
		}
		
		void Process::sendTask(const char* alias, const Task& rTask, double timeout) 
		throw(NullPointerException, InvalidArgumentException, MsgException)
		{
			// check the parameters
			
			if(!alias)
				throw NullPointerException("alias");
			
			if(timeout < 0 && timeout !=-1.0)
				throw InvalidArgumentException("timeout (the timeout value must not be negative and different than -1.0)");
			
			if(MSG_OK != MSG_task_send_with_timeout(rTask.nativeTask, alias ,timeout))
				throw MsgException("MSG_task_send_with_timeout()");
				
		}
		
		void Process::sendTask(const char* alias, const Task& rTask) 
		throw(NullPointerException, MsgException)
		{
			// check the parameters
			
			if(!alias)
				throw NullPointerException("alias");
				
			if(MSG_OK != MSG_task_send_with_timeout(rTask.nativeTask, alias ,-1.0))
				throw MsgException("MSG_task_send_with_timeout()");
		}
		
		void Process::sendTask(const Task& rTask) 
		throw(BadAllocException, MsgException)
		{
			char* alias = (char*)calloc(strlen(Host::currentHost().getName() + strlen(nativeProcess->name) + 2);
				
			if(!alias)
				throw BadAllocException("alias");
				
			sprintf(alias,"%s:%s", Host::currentHost().getName() ,nativeProcess->name);
			
			MSG_error_t rv = MSG_task_send_with_timeout(rTask.nativeTask, alias ,-1.0);
			
			free(alias);
			
			if(MSG_OK != rv)
				throw MsgException("MSG_task_send_with_timeout()");
		}
		
		void Process::sendTask(const Task& rTask, double timeout) 
		throw(BadAllocException, InvalidArgumentException, MsgException)
		{
			// check the parameters
			
			if(timeout < 0 && timeout !=-1.0)
				throw InvalidArgumentException("timeout (the timeout value must not be negative and different than -1.0)");
			
			char* alias = (char*)calloc(strlen(Host::currentHost().getName() + strlen(nativeProcess->name) + 2);
				
			if(!alias)
				throw BadAllocException("alias");
				
			sprintf(alias,"%s:%s", Host::currentHost().getName() ,nativeProcess->name);
			
			MSG_error_t rv = MSG_task_send_with_timeout(rTask.nativeTask, alias ,timeout);
			
			free(alias);
			
			if(MSG_OK != rv)
				throw MsgException("MSG_task_send_with_timeout()");	
		}
		
		Task& Process::receiveTask(const char* alias) 
		throw(NullPointerException, MsgException)
		{
			// check the parameters
			
			if(!alias)
				throw NullPointerException(alias);
				
			m_task_t nativeTask = NULL;
			
			if (MSG_OK !=  MSG_task_receive_ext(&nativeTask,alias, -1.0, NULL)) 
				throw MsgException("MSG_task_receive_ext() failed");
		
			return (*((Task*)nativeTask->data));
		}
		
		
		Task& Process::receiveTask(void) 
		throw(BadAllocException, MsgException)
		{
			
			char* alias = (char*)calloc(strlen(Host::currentHost().getName() + strlen(nativeProcess->name) + 2);
			
			if(!alias)
				throw BadAllocException("alias");	
			
			sprintf(alias,"%s:%s", Host::currentHost().getName() ,nativeProcess->name);
			
			m_task_t nativeTask = NULL;
			
			MSG_error_t rv = MSG_task_receive_ext(&nativeTask, alias, -1.0, NULL);
			
			free(alias);
			
			if(MSG_OK !=  rv) 
				throw MsgException("MSG_task_receive_ext() failed");	
		
			return (*((Task*)nativeTask->data));
		}
		
		
		Task& Process::receiveTask(const char* alias, double timeout) 
		throw(NullPointerException, InvalidArgumentException, MsgException)
		{
			// check the parameters
			
			if(!alias)
				throw NullPointerException("alias");
				
			if(timeout < 0 && timeout !=-1.0)
				throw InvalidArgumentException("timeout (the timeout value must not be negative and different than -1.0)");
				
			m_task_t nativeTask = NULL;
			
			if(MSG_OK !=  MSG_task_receive_ext(&nativeTask, alias, timeout, NULL)) 
				throw MsgException("MSG_task_receive_ext() failed");		
		
			return (*((Task*)nativeTask->data));
		}
		
		
		Task& Process::receiveTask(double timeout) 
		throw(InvalidArgumentException, BadAllocException, MsgException)
		{
			// check the parameters
			
			if(timeout < 0 && timeout !=-1.0)
				throw InvalidArgumentException("timeout (the timeout value must not be negative and different than -1.0)");
				
			
			char* alias = (char*)calloc(strlen(Host::currentHost().getName() + strlen(nativeProcess->name) + 2);
			
			if(!alias)
				throw BadAllocException("alias");
			
			sprintf(alias,"%s:%s", Host::currentHost().getName() ,nativeProcess->name);
			
			m_task_t nativeTask = NULL;
			
			MSG_error_t rv = MSG_task_receive_ext(&nativeTask, alias, timeout, NULL);
			
			free(alias);
			
			if(MSG_OK !=  rv) 
				throw MsgException("MSG_task_receive_ext() failed");	
		
			return (*((Task*)nativeTask->data));
		}
		
		
		Task& Process::receiveTask(const char* alias, double timeout, const Host& rHost) 
		throw(NullPointerException, InvalidArgumentException, MsgException)
		{
			// check the parameters
			
			if(!alias)
				throw NullPointerException("alias");
			
			if(timeout < 0 && timeout !=-1.0)
				throw InvalidArgumentException("timeout (the timeout value must not be negative and different than -1.0)");
				
			m_task_t nativeTask = NULL;
			
			if(MSG_OK !=  MSG_task_receive_ext(&nativeTask, alias, timeout, rHost.nativeHost)) 
				throw MsgException("MSG_task_receive_ext() failed");
		
			return (*((Task*)nativeTask->data));
		}
		
		
		Task& Process::receiveTask(double timeout, const Host& rHost) 
		throw(BadAllocException, InvalidArgumentException, MsgException)
		{
			// check the parameters
			
			if(timeout < 0 && timeout !=-1.0)
				throw InvalidArgumentException("timeout (the timeout value must not be negative and different than -1.0)");
			
			char* alias = (char*)calloc(strlen(Host::currentHost().getName() + strlen(nativeProcess->name) + 2);
			
			if(!alias)
				throw BadAllocException("alias");
				
			sprintf(alias,"%s:%s", Host::currentHost().getName() ,nativeProcess->name);
			
			m_task_t nativeTask = NULL;
			
			MSG_error_t rv = MSG_task_receive_ext(&nativeTask, alias, timeout, rHost.nativeHost);
			
			free(alias);
			
			if(MSG_OK !=  rv) 
				throw MsgException("MSG_task_receive_ext() failed");
		
			return (*((Task*)nativeTask->data));
		}
		
		
		Task& Process::receiveTask(const char* alias, const Host& rHost) 
		throw(NullPointerException, MsgException)
		{
			
			// check the parameters
			
			if(!alias)
				throw NullPointerException("alias");
			
			m_task_t nativeTask = NULL;
			
			if(MSG_OK !=   MSG_task_receive_ext(&nativeTask, alias, -1.0, rHost.nativeHost)) 
				throw MsgException("MSG_task_receive_ext() failed");
		
			return (*((Task*)nativeTask->data));
		}
		
		Task& Process::receiveTask(const Host& rHost) 
		throw(BadAllocException, MsgException)
		{
			char* alias = (char*)calloc(strlen(Host::currentHost().getName() + strlen(nativeProcess->name) + 2);
			
			if(!alias)
				throw BadAllocException("alias");
			
			sprintf(alias,"%s:%s", Host::currentHost().getName() ,nativeProcess->name);
			
			m_task_t nativeTask = NULL;
			
			MSG_error_t rv = MSG_task_receive_ext(&nativeTask, alias, -1.0, rHost.nativeHost);
			
			free(alias);
			
			if(MSG_OK !=  rv) 
				throw MsgException("MSG_task_receive_ext() failed");
		
			return (*((Task*)nativeTask->data));
		}
		
		void Process::create(const Host& rHost, const char* name, int argc, char** argv)
		throw(HostNotFoundException)
		{
			smx_process_t nativeCurrentProcess = NULL;
			nativeProcess = xbt_new0(s_smx_process_t, 1);
			smx_simdata_process_t simdata = xbt_new0(s_smx_simdata_process_t, 1);
			smx_host_t nativeHost = SIMIX_host_get_by_name(rHost.getName());
			
			throw HostNotFoundException(rHost.getName());
			
			argv = (char**)realloc(argc + 1, sizeo(char*));
			
			argv[argc] = (char*)this;
			
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
		
		Process& Process::fromNativeProcess(m_process_t nativeProcess)
		{
			return (*((Process*)(nativeProcess->simdata->arg[nativeProcess->argc])));
		}
		
		int Process::run(int argc, char** argv)
		{
			Process* process =(Process*)argv[argc];
			
			return process->main(argc, argv);
		}
		
	} // namespace Msg

} // namespace SimGrid