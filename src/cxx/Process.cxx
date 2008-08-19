/*
 * Process.cxx
 *
 * Copyright 2006,2007 Martin Quinson, Malek Cherier           
 * All right reserved. 
 *
 * This program is free software; you can redistribute 
 * it and/or modify it under the terms of the license 
 *(GNU LGPL) which comes with this package. 
 *
 */
 
 /* Process member functions implementation.
  */  

#include <Process.hpp>


#include <ApplicationHandler.hpp>
#include <Host.hpp>
#include <Task.hpp>

#include <stdlib.h>
#include <stdio.h>

#include <msg/msg.h>
#include <msg/private.h>
#include <msg/mailbox.h>



namespace SimGrid
{
	namespace Msg
	{

		MSG_IMPLEMENT_DYNAMIC(Process, Object)

		// Default constructor.
		Process::Process()
		{
			this->nativeProcess = NULL;
		}
		
		Process::Process(const char* hostName, const char* name)
		throw(NullPointerException, HostNotFoundException, BadAllocException)
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
				
			if(argc < 0)
				throw InvalidArgumentException("argc (must be positive)");
				
			if(!argc && argv)
				throw LogicException("argv is not NULL but argc is zero");
			
			if(argc && !argv)
				throw LogicException("argv is NULL but argc is not zero");
			
			create(rHost, name, argc, argv);	
		}
		
		Process::Process(const char* hostName, const char* name, int argc, char** argv)
		throw(NullPointerException, InvalidArgumentException, LogicException, HostNotFoundException, BadAllocException)
		{
			// check the parameters
			
			if(!name)
				throw NullPointerException("name");
				
			if(!hostName)
				throw NullPointerException("hostName");
				
			if(argc < 0)
				throw InvalidArgumentException("argc (must be positive)");
				
			if(!argc && argv)
				throw LogicException("argv is not NULL but argc is zero");
			
			if(argc && !argv)
				throw LogicException("argv is NULL but argc is not zero");
				
			Host host = Host::getByName(hostName);
				
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
		
		int Process::isSuspended(void)
		{
		   return MSG_process_is_suspended(nativeProcess);
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
				throw InvalidArgumentException("PID (the PID of the process to retrieve is less than 1)");
				
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
			if(MSG_OK != MSG_process_change_host(rHost.nativeHost))
				throw MsgException("MSG_process_change_host()");
			
		}
		
		void Process::sleep(double seconds)
		throw(InvalidArgumentException, MsgException)
		{
			// check the parameters.
			if(seconds <= 0)
				throw InvalidArgumentException("seconds (must not be less or equals to zero");
				
			if(MSG_OK != MSG_process_sleep(seconds))
				throw MsgException("MSG_process_sleep()");
		   	
		}
		
		void Process::putTask(const Host& rHost, int channel, Task* task)
		throw(InvalidArgumentException, MsgException)
		{
			// check the parameters
			
			if(channel < 0)
				throw InvalidArgumentException("channel (must not be negative)");
				
			if(MSG_OK != MSG_task_put_with_timeout(task->nativeTask, rHost.nativeHost, channel, -1.0))
				throw MsgException("MSG_task_put_with_timeout()");
		}
		
		void Process::putTask(const Host& rHost, int channel, Task* task, double timeout) 
		throw(InvalidArgumentException, MsgException)
		{
			// check the parameters
			if(channel < 0)
				throw InvalidArgumentException("channel (must not be negative)");
				
			if(timeout < 0 && timeout != -1.0)
				throw InvalidArgumentException("timeout (must not be less than zero and different of -1.0)");
				
			if(MSG_OK != MSG_task_put_with_timeout(task->nativeTask, rHost.nativeHost, channel, timeout))
				throw MsgException("MSG_task_put_with_timeout() failed");
		}
		
		Task* Process::getTask(int channel) 
		throw(InvalidArgumentException, MsgException)
		{
			// check the parameters
			
			if(channel < 0)
				throw InvalidArgumentException("channel (must not be negative)");
			
			m_task_t nativeTask = NULL;
			
			if (MSG_OK != MSG_task_get_ext(&nativeTask, channel, -1.0, NULL)) 
				throw MsgException("MSG_task_get_ext() failed");
			
			return (Task*)(nativeTask->data);
		}
		
		Task* Process::getTask(int channel, double timeout) 
		throw(InvalidArgumentException, MsgException)
		{
			// check the parameters
			if(channel < 0)
				throw InvalidArgumentException("channel (must not be negative)");
				
			if(timeout < 0 && timeout != -1.0)
				throw InvalidArgumentException("timeout (must not be less than zero and different of -1.0)");
			
			m_task_t nativeTask = NULL;
			
			if (MSG_OK != MSG_task_get_ext(&nativeTask, channel, timeout, NULL)) 
				throw MsgException("MSG_task_get_ext() failed");
			
			return (Task*)(nativeTask->data);
		}
		
		Task* Process::getTask(int channel, const Host& rHost) 
		throw(InvalidArgumentException, MsgException)
		{
			// check the parameters
			if(channel < 0)
				throw InvalidArgumentException("channel (must not be negative)");
				
			m_task_t nativeTask = NULL;
			
			if (MSG_OK != MSG_task_get_ext(&nativeTask, channel, -1.0, rHost.nativeHost)) 
				throw MsgException("MSG_task_get_ext() failed");
			
			return (Task*)(nativeTask->data);
		}
		
		Task* Process::getTask(int channel, double timeout, const Host& rHost)
		throw(InvalidArgumentException, MsgException)
		{
			// check the parameters
			if(channel < 0)
				throw InvalidArgumentException("channel (must not be negative)");
				
			if(timeout < 0 && timeout != -1.0)
				throw InvalidArgumentException("timeout (must not be less than zero and different of -1.0)");
			
			m_task_t nativeTask = NULL;	
			
			if (MSG_OK != MSG_task_get_ext(&nativeTask, channel, timeout, rHost.nativeHost)) 
				throw MsgException("MSG_task_get_ext() failed");
			
			return (Task*)(nativeTask->data);
		}
		
		void Process::sendTask(const char* alias, Task* task, double timeout) 
		throw(NullPointerException, InvalidArgumentException, MsgException)
		{
			// check the parameters
			
			if(!alias)
				throw NullPointerException("alias");
			
			if(timeout < 0 && timeout !=-1.0)
				throw InvalidArgumentException("timeout (the timeout value must not be negative and different than -1.0)");
			
			if(MSG_OK != MSG_task_send_with_timeout(task->nativeTask, alias ,timeout))
				throw MsgException("MSG_task_send_with_timeout()");
				
		}
		
		void Process::sendTask(const char* alias, Task* task) 
		throw(NullPointerException, MsgException)
		{
			// check the parameters
			
			if(!alias)
				throw NullPointerException("alias");
				
			if(MSG_OK != MSG_task_send_with_timeout(task->nativeTask, alias ,-1.0))
				throw MsgException("MSG_task_send_with_timeout()");
		}
		
		void Process::sendTask(Task* task) 
		throw(BadAllocException, MsgException)
		{
			char* alias = (char*)calloc( strlen(this->getHost().getName()) + strlen(nativeProcess->name) + 2, sizeof(char));
				
			if(!alias)
				throw BadAllocException("alias");
				
			sprintf(alias,"%s:%s", Host::currentHost().getName() ,nativeProcess->name);
			
			MSG_error_t rv = MSG_task_send_with_timeout(task->nativeTask, alias ,-1.0);
			
			free(alias);
			
			if(MSG_OK != rv)
				throw MsgException("MSG_task_send_with_timeout()");
		}
		
		void Process::sendTask(Task* task, double timeout) 
		throw(BadAllocException, InvalidArgumentException, MsgException)
		{
			// check the parameters
			
			if(timeout < 0 && timeout !=-1.0)
				throw InvalidArgumentException("timeout (the timeout value must not be negative and different than -1.0)");
			
			char* alias = (char*)calloc( strlen(this->getHost().getName()) + strlen(nativeProcess->name) + 2, sizeof(char));
				
			if(!alias)
				throw BadAllocException("alias");
				
			sprintf(alias,"%s:%s", Host::currentHost().getName() ,nativeProcess->name);
			
			MSG_error_t rv = MSG_task_send_with_timeout(task->nativeTask, alias ,timeout);
			
			free(alias);
			
			if(MSG_OK != rv)
				throw MsgException("MSG_task_send_with_timeout()");	
		}
		
		Task* Process::receiveTask(const char* alias) 
		throw(NullPointerException, MsgException)
		{
			// check the parameters
			
			if(!alias)
				throw NullPointerException(alias);
				
			m_task_t nativeTask = NULL;
			
			if (MSG_OK !=  MSG_task_receive_ext(&nativeTask,alias, -1.0, NULL)) 
				throw MsgException("MSG_task_receive_ext() failed");
		
			return (Task*)(nativeTask->data);
		}
		
		
		Task* Process::receiveTask(void) 
		throw(BadAllocException, MsgException)
		{
			
			char* alias = (char*)calloc( strlen(this->getHost().getName()) + strlen(nativeProcess->name) + 2, sizeof(char));
			
			if(!alias)
				throw BadAllocException("alias");	
			
			sprintf(alias,"%s:%s", Host::currentHost().getName() ,nativeProcess->name);
			
			m_task_t nativeTask = NULL;
			
			MSG_error_t rv = MSG_task_receive_ext(&nativeTask, alias, -1.0, NULL);
			
			free(alias);
			
			if(MSG_OK !=  rv) 
				throw MsgException("MSG_task_receive_ext() failed");	
		
			return (Task*)(nativeTask->data);
		}
		
		
		Task* Process::receiveTask(const char* alias, double timeout) 
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
		
			return (Task*)(nativeTask->data);
		}
		
		
		Task* Process::receiveTask(double timeout) 
		throw(InvalidArgumentException, BadAllocException, MsgException)
		{
			// check the parameters
			
			if(timeout < 0 && timeout !=-1.0)
				throw InvalidArgumentException("timeout (the timeout value must not be negative and different than -1.0)");
				
			
			char* alias = (char*)calloc( strlen(this->getHost().getName()) + strlen(nativeProcess->name) + 2, sizeof(char));
			
			if(!alias)
				throw BadAllocException("alias");
			
			sprintf(alias,"%s:%s", Host::currentHost().getName() ,nativeProcess->name);
			
			m_task_t nativeTask = NULL;
			
			MSG_error_t rv = MSG_task_receive_ext(&nativeTask, alias, timeout, NULL);
			
			free(alias);
			
			if(MSG_OK !=  rv) 
				throw MsgException("MSG_task_receive_ext() failed");	
		
			return (Task*)(nativeTask->data);
		}
		
		
		Task* Process::receiveTask(const char* alias, double timeout, const Host& rHost) 
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
		
			return (Task*)(nativeTask->data);
		}
		
		
		Task* Process::receiveTask(double timeout, const Host& rHost) 
		throw(BadAllocException, InvalidArgumentException, MsgException)
		{
			// check the parameters
			
			if(timeout < 0 && timeout !=-1.0)
				throw InvalidArgumentException("timeout (the timeout value must not be negative and different than -1.0)");
			
			char* alias = (char*)calloc( strlen(this->getHost().getName()) + strlen(nativeProcess->name) + 2, sizeof(char));
			
			if(!alias)
				throw BadAllocException("alias");
				
			sprintf(alias,"%s:%s", Host::currentHost().getName() ,nativeProcess->name);
			
			m_task_t nativeTask = NULL;
			
			MSG_error_t rv = MSG_task_receive_ext(&nativeTask, alias, timeout, rHost.nativeHost);
			
			free(alias);
			
			if(MSG_OK !=  rv) 
				throw MsgException("MSG_task_receive_ext() failed");
		
			return (Task*)(nativeTask->data);
		}
		
		
		Task* Process::receiveTask(const char* alias, const Host& rHost) 
		throw(NullPointerException, MsgException)
		{
			
			// check the parameters
			
			if(!alias)
				throw NullPointerException("alias");
			
			m_task_t nativeTask = NULL;
			
			if(MSG_OK !=   MSG_task_receive_ext(&nativeTask, alias, -1.0, rHost.nativeHost)) 
				throw MsgException("MSG_task_receive_ext() failed");
		
			return (Task*)(nativeTask->data);
		}
		
		Task* Process::receiveTask(const Host& rHost) 
		throw(BadAllocException, MsgException)
		{
			char* alias = (char*)calloc( strlen(this->getHost().getName()) + strlen(nativeProcess->name) + 2, sizeof(char));
			
			if(!alias)
				throw BadAllocException("alias");
			
			sprintf(alias,"%s:%s", Host::currentHost().getName() ,nativeProcess->name);
			
			m_task_t nativeTask = NULL;
			
			MSG_error_t rv = MSG_task_receive_ext(&nativeTask, alias, -1.0, rHost.nativeHost);
			
			free(alias);
			
			if(MSG_OK !=  rv) 
				throw MsgException("MSG_task_receive_ext() failed");
		
			return (Task*)(nativeTask->data);
		}

		void Process::create(const Host& rHost, const char* name, int argc, char** argv)
		throw(InvalidArgumentException)
		{
			char alias[MAX_ALIAS_NAME + 1] = {0};
			msg_mailbox_t mailbox;
			
			
			// try to retrieve the host where to create the process from its name
			m_host_t nativeHost = rHost.nativeHost;
			
			if(!nativeHost)
				throw InvalidArgumentException("rHost"); 
			
			/* allocate the data of the simulation */
			this->nativeProcess = xbt_new0(s_m_process_t,1);
			this->nativeProcess->simdata = xbt_new0(s_simdata_process_t,1);
			this->nativeProcess->name = _strdup(name);
			this->nativeProcess->simdata->m_host = nativeHost;
			this->nativeProcess->simdata->PID = msg_global->PID++;
			
			// realloc the list of the arguments to add the pointer to this process instance at the end
			if(argc)
			{
				argv = (char**)realloc(argv , (argc + 2) * sizeof(char*));
			}
			else
			{
				argv = (char**)calloc(2 ,sizeof(char*));
			}
			
			// add the pointer to this instance at the end of the list of the arguments of the process
			// so the static method Process::run() (passed as argument of the MSG function xbt_context_new())
			// can retrieve the concerned process object by the run
			// so Process::run() can call the method main() of the good process
			// for more detail see Process::run() method
			argv[argc] = NULL;
			argv[argc + 1] = (char*)this;

			this->nativeProcess->simdata->argc = argc;
			this->nativeProcess->simdata->argv = argv;
			
			this->nativeProcess->simdata->s_process = SIMIX_process_create(
									this->nativeProcess->name,
				   					Process::run, 
				   					(void*)this->nativeProcess,
				   					nativeHost->name, 
				   					argc,
				   					argv, 
				   					NULL);
			
			if (SIMIX_process_self()) 
			{/* someone created me */
				this->nativeProcess->simdata->PPID = MSG_process_get_PID((m_process_t)SIMIX_process_self()->data);
			} 
			else 
			{
				this->nativeProcess->simdata->PPID = -1;
			}
			
			this->nativeProcess->simdata->last_errno = MSG_OK;
			
			/* add the process to the list of the processes of the simulation */
			xbt_fifo_unshift(msg_global->process_list, this->nativeProcess);
			
			sprintf(alias,"%s:%s",(this->nativeProcess->simdata->m_host->simdata->smx_host)->name,this->nativeProcess->name);
			
			mailbox = MSG_mailbox_new(alias);
			
			MSG_mailbox_set_hostname(mailbox, this->nativeProcess->simdata->m_host->simdata->smx_host->name);	
		}
		
		Process* Process::fromNativeProcess(m_process_t nativeProcess)
		{
			return ((Process*)(nativeProcess->simdata->argv[nativeProcess->simdata->argc + 1]));
			
		}
		
		int Process::run(int argc, char** argv)
		{
			
			// the last argument of the process is the pointer to the process to run
			// for more detail see Process::create() method
			return ((Process*)argv[argc + 1])->main(argc, argv);
			
		}

		int Process::main(int argc, char** argv)
		{
			throw LogicException("Process::main() not implemented");
		}

		/*void* Process::operator new(size_t size)
		{
			// TODO
		}

		void Process::operator delete(void* p)
		{
			// TODO
		}*/
		
	} // namespace Msg

} // namespace SimGrid

