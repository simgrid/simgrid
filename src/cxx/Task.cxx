#include <Task.hpp>

namespace SimGrid
{
	namespace Msg
	{
	
		Task::Task()
		{
			nativeTask = NULL;
		}
		
		
		Task::Task(const Task& rTask)
		{
		}
		
		
		Task::~Task()
		throw(NativeException)
		{
			if(NULL != nativeTask)
				if(MSG_OK != MSG_task_destroy(nativeTask))
					throw MsgException("MSG_task_destroy() failed");
		}
		
		
		Task::Task(const char* name, double computeDuration, double messageSize)
		throw(InvalidArgumentException, NullPointerException)
		{
			 
			if(computeDuration < 0) 
				throw InvalidArgumentException("computeDuration");
			
			if(messageSize < 0) 
				throw InvalidArgumentException("messageSize");
			
			if(!name) 
				throw NullPointerException("name");
			
			// create the task
			nativeTask = MSG_task_create(name, computeDuration, messageSize, NULL);
			
			nativeTask->data = (void*)this;
		}
		
		Task::Task(const char* name, Host* hosts, double* computeDurations, double* messageSizes, int hostCount)
		throw(NullPointerException, InvalidArgumentException)
		{
			
			// check the parameters
			
			if(!name) 
				throw NullPointerException("name");
			
			if(!hosts) 
				throw NullPointerException("hosts");
			
			if(!computeDurations) 
				throw NullPointerException("computeDurations");
				
			if(!messageSizes) 
				throw NullPointerException("messageSizes");
				
			if(!hostCount)
				throw InvalidArgumentException("hostCount (must not be zero)");
				
			
			m_host_t* nativeHosts;
			double* durations;
			double* sizes;
			
			
			nativeHosts = xbt_new0(m_host_t, hostCount);
			durations = xbt_new0(double,hostCount);
			sizes = xbt_new0(double, hostCount * hostCount);
			
			
			for(int index = 0; index < hostCount; index++) 
			{
				
				nativeHosts[index] = hosts[index].nativeHost;
				durations[index] = computeDurations[index];
			}
			
			for(int index = 0; index < hostCount*hostCount; index++) 
				sizes[index] = messageSizes[index];
			
			
			nativeTask = MSG_parallel_task_create(name, hostCount, nativeHosts, durations, sizes,NULL);
			
			
			
			task->data = (void*)this;
			
		}
		
		const char* Task::getName(void) const
		{
			return nativeTask->name;
		}
		
		Process& Task::getSender(void) const
		{
			m_proccess_t nativeProcess = MSG_task_get_sender(nativeTask);
			
			return (*((Process*)(nativeProcess->data)));
		}
		
		Host& Task::getSource(void) const 
		{
			m_host_t nativeHost = MSG_task_get_source(nativeTask);
			
			return (*((Host*)(nativeHost->data)));	
		}
		
		double Task::getComputeDuration(void) const
		{
			return MSG_task_get_compute_duration(nativeTask);
		}
		
		double Task::getRemainingDuration(void) const
		{
			return MSG_task_get_remaining_computation(nativeTask);
		}
		
		void Task::setPriority(double priority)
		throw(InvalidArgumentException)
		{
			// check the parameters
			
			if(priority < 0.0)
				throw InvalidArgumentException("priority");
				
			MSG_task_set_priority(nativeTask, priority);
		}
		
		Task& Task::get(int channel) 
		throw(InvalidArgumentException, MsgException)
		{
			// check the parameters
			
			if(channel < 0)
				throw InvalidArgumentException("channel (must not be negative)");
				
			m_task_t nativeTask = NULL;
			
			if(MSG_OK != MSG_task_get_ext(&nativeTask, channel , -1.0, NULL)) 
				throw MsgException("MSG_task_get_ext() failed");
			
			return (*((Task*)(nativeTask->data)));
		}
		
		Task& Task::get(int channel, const Host& rHost) 
		throw(InvalidArgumentException, MsgException)
		{
			// check the parameters
			
			if(channel < 0)
				throw InvalidArgumentException("channel (must not be negative)");
				
			m_task_t nativeTask = NULL;
			
			
			if(MSG_OK != MSG_task_get_ext(&nativeTask, channel , -1.0, rHost.nativeHost)) 
				throw MsgException("MSG_task_get_ext() failed");
			
			return (*((Task*)(nativeTask->data)));
		}
		
		Task& Task::get(int channel, double timeout, const Host& rHost) 
		throw(InvalidArgumentException, MsgException)
		{
			// check the parameters
			
			if(channel < 0)
				throw InvalidArgumentException("channel (must not be negative)");
			
			if(timeout < 0 && timeout !=-1.0)
				throw InvalidArgumentException("timeout (must not be negative and different thant -1.0)");	
				
			m_task_t nativeTask = NULL;
			
			
			if(MSG_OK != MSG_task_get_ext(&nativeTask, channel , timeout, rHost.nativeHost)) 
				throw MsgException("MSG_task_get_ext() failed");
			
			return (*((Task*)(nativeTask->data)));
		}
		
		bool static Task::probe(int channel)
		throw(InvalidArgumentException)
		{
			// check the parameters
			
			if(channel < 0)
				throw InvalidArgumentException("channel (must not be negative)");
				
			return (bool)MSG_task_Iprobe(channel);
		}
		
		int Task::probe(int channel, const Host& rHost)
		throw(InvalidArgumentException)
		{
			// check the parameters
			
			if(channel < 0)
				throw InvalidArgumentException("channel (must not be negative)");
				
			return MSG_task_probe_from_host(chan_id,rHost.nativeHost);
		}
		
		void Task::execute(void) 
		throw(MsgException)
		{
			if(MSG_OK != MSG_task_execute(nativeTask))
				throw MsgException("MSG_task_execute() failed");
		}
		
		void Task::cancel(void) 
		throw(MsgException)
		{
			if(MSG_OK != MSG_task_cancel(nativeTask))
				throw MsgException("MSG_task_cancel() failed");
		}
		
		void Task::send(void) 
		throw(BadAllocException, MsgException)
		{	
			char* alias = (char*)calloc(strlen(Process::currentProcess().getName() + strlen(Host::currentHost().getName()) + 2);
			
			if(!alias)
				throw BadAllocException("alias");
				
			sprintf(alias,"%s:%s", Process::currentProcess().getName(),Host::currentHost().getName());
				
			MSG_error_t rv = MSG_task_send_with_timeout(nativeTask, alias, -1.0);
		
			free(alias)
		
			if(MSG_OK != rv)
				throw MsgException("MSG_task_send_with_timeout() failed");
		}
		
		void Task::send(const char* alias) 
		throw(NullPointerException, MsgException)
		{
			// check the parameters
			
			if(!alias)
				throw NullPointerException("alias");
		
			if(MSG_OK != MSG_task_send_with_timeout(nativeTask, alias, -1.0))
				throw MsgException("MSG_task_send_with_timeout() failed");
		}
		
		void Task::send(double timeout) 
		throw(BadAllocationException, InvalidArgumentException, MsgException)
		{
			// check the parameters
			
			if(timeout < 0  && timeout != -1.0)
				throw InvalidArgumentException("timeout (must not be negative and different than -1.0");
						
			char* alias = (char*)calloc(strlen(Process::currentProcess().getName() + strlen(Host::currentHost().getName()) + 2);
			
			if(!alias)
				throw BadAllocException("alias");
				
			sprintf(alias,"%s:%s", Process::currentProcess().getName(),Host::currentHost().getName());
		
			MSG_error_t rv = MSG_task_send_with_timeout(nativeTask, alias, timeout);
		
			free(alias)
		
			if(MSG_OK != rv)
				throw MsgException("MSG_task_send_with_timeout() failed");
		}	
		
		void Task::send(const char* alias, double timeout) 
		throw(NullPointerException, InvalidArgumentException, MsgException)
		{
			// check the parameters
			
			if(!alias)
				throw NullPointerException("alias");
				
			if(timeout < 0  && timeout != -1.0)
				throw InvalidArgumentException("timeout (must not be negative and different than -1.0");
						
				
			if(MSG_OK != MSG_task_send_with_timeout(nativeTask, alias, timeout))
				throw MsgException("MSG_task_send_with_timeout() failed");
		}
		
		void Task::sendBounded(double maxRate) 
		throw(BadAllocException, InvalidArgumentException, MsgException)
		{
			// check the parameters
			
			if(maxRate < 0 && maxRate != -1.0)
				throw InvalidArgumentException("maxRate (must not be negative and different than -1.0");
					
			char* alias = (char*)calloc(strlen(Process::currentProcess().getName() + strlen(Host::currentHost().getName()) + 2);
			
			if(!alias)
				throw BadAllocException("alias");
			
			sprintf(alias,"%s:%s", Process::currentProcess().getName(),Host::currentHost().getName());
			
			MSG_error_t rv = MSG_task_send_bounded(nativeTask, alias, maxRate);
			
			free(alias);	
			
			if(MSG_OK != rv)
				throw MsgException("MSG_task_send_bounded() failed");
				
		}
		
		void Task::sendBounded(const char* alias, double maxRate) 
		throw(NullPointerException, InvalidArgumentException, MsgException)
		{
			// check the parameters
			
			if(maxRate < 0 && maxRate != -1.0)
				throw InvalidArgumentException("maxRate (must not be negative and different than -1.0");
							
			if(!alias)
				throw NullPointerException("alias");
			
			if(MSG_OK != MSG_task_send_bounded(nativeTask, alias, maxRate))
				throw MsgException("MSG_task_send_bounded() failed");
		}
		
		Task& Task::receive(void) 
		throw(throw(BadAllocException, MsgException))
		{
			char* alias = (char*)calloc(strlen(Process::currentProcess().getName() + strlen(Host::currentHost().getName()) + 2);
			
			if(!alias)
				throw BadAllocException("alias");
				
			sprintf(alias,"%s:%s", Process::currentProcess().getName(),Host::currentHost().getName());
				
			m_task_t nativeTask = NULL;
			
			MSG_error_t rv = MSG_task_receive_ext(&nativeTask, alias, -1.0, NULL);	
		
			free(alias);
			
			if(MSG_OK != rv) 
				throw MsgException("MSG_task_receive_ext() failed");
		
			return (*((Task*)nativeTask->data));
		}
		
		Task& Task::receive(const char* alias) 
		throw(NullPointerException, MsgException)
		{
			// check the parameters
			
			if(!alias)
				throw NullPointerException("alias");
				
			m_task_t nativeTask = NULL;
			
			if(MSG_OK != MSG_task_receive_ext(&nativeTask, alias, -1.0, NULL)) 
				throw MsgException("MSG_task_receive_ext() failed");
		
			return (*((Task*)nativeTask->data));
		}
		
		Task& Task::receive(const char* alias, double timeout) 
		throw(NullPointerException, InvalidArgumentException, MsgException)
		{
			// check the parameters
			
			if(!alias)
				throw NullPointerException("alias");
			
			if(timeout < 0 && alias != -1.0)
				throw InvalidArgumentException("timeout (must not be negative and differnt than -1.0)");
				
			m_task_t nativeTask = NULL;
			
			if(MSG_OK != MSG_task_receive_ext(&nativeTask, alias, timeout, NULL)) 
				throw MsgException("MSG_task_receive_ext() failed");
		
			return (*((Task*)nativeTask->data));
		}
		
		Task& Task::receive(const char* alias, const Host& rHost) 
		throw(NullPointerException, MsgException)
		{
			// check the parameters
			
			if(!alias)
				throw NullPointerException("alias");
			
			m_task_t nativeTask = NULL;
			
			if(MSG_OK != MSG_task_receive_ext(&nativeTask, alias, -1.0, rHost.nativeHost)) 
				throw MsgException("MSG_task_receive_ext() failed");
		
			return (*((Task*)nativeTask->data));
		}	
		
		Task& Task::receive(const char* alias, double timeout, const Host& rHost) 
		throw(NullPointerException, InvalidArgumentException, MsgException)
		{
			// check the parameters
			
			if(!alias)
				throw NullPointerException("alias");
			
			if(timeout < 0 && alias != -1.0)
				throw InvalidArgumentException("timeout (must not be negative and differnt than -1.0)");
				
			m_task_t nativeTask = NULL;
			
			
			if(MSG_OK != MSG_task_receive_ext(&nativeTask, alias, timeout, rHost.nativeHost)) 
				throw MsgException("MSG_task_receive_ext() failed");
		
			return (*((Task*)nativeTask->data));
		}
		
		bool Task::listen(void) 
		throw(BadAllocException)
		{
			char* alias = (char*)calloc(strlen(Process::currentProcess().getName() + strlen(Host::currentHost().getName()) + 2);
			
			if(!alias)
				throw BadAllocException("alias");
				
			sprintf(alias,"%s:%s", Process::currentProcess().getName(),Host::currentHost().getName());
				
			int rv = MSG_task_listen(alias);
			
			free(alias);
			
			return (bool)rv;
		}	
		
		bool Task::listen(const char* alias) 
		throw(NullPointerException)
		{
			// check the parameters
			
			if(!alias)
				throw NullPointerException("alias");
				
			return (bool)MSG_task_listen(alias);
		}
		
		int Task::listenFrom(void) 
		throw(BadAllocException)
		{
			char* alias = (char*)calloc(strlen(Process::currentProcess().getName() + strlen(Host::currentHost().getName()) + 2);
			
			if(!alias)
				throw BadAllocException("alias");
				
			sprintf(alias,"%s:%s", Process::currentProcess().getName(),Host::currentHost().getName());
			
			int rv = MSG_task_listen_from(alias);
			
			free(alias);
			
			return rv;
		}	
		
		int Task::listenFrom(const char* alias) 
		throw(NullPointerException)
		{
			if(!alias)
				throw NullPointerException("alias");
				
			return MSG_task_listen_from(alias);
			
		}
		
		int Task::listenFromHost(const Host& rHost) 
		throw(BadAllocException)
		{
			char* alias = (char*)calloc(strlen(Process::currentProcess().getName() + strlen(Host::currentHost().getName()) + 2);
			
			if(!alias)
				throw BadAllocException("alias");
			
			sprintf(alias,"%s:%s", Process::currentProcess().getName(),Host::currentHost().getName());
			
			int rv = MSG_task_listen_from_host(alias, rHost.nativeHost);
			
			free(alias);
			
			return rv;
		}
			
		int Task::listenFromHost(const char* alias, const Host& rHost) 
		throw(NullPointerException)
		{
			// check the parameters
			if(!alias)
				throw NullPointerException("alias");
				
			return MSG_task_listen_from_host(alias, rHost.nativeHost);
		}	
	} // namespace Msg																								
} // namespace SimGrid

