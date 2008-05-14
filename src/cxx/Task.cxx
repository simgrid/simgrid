#include <Task.hpp>

namespace msg
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
	{
		if(MSG_OK != MSG_task_destroy(nativeTask))
		{
			// TODO throw NativeException
		}	
	}
}


Task::Task(const char* name, double computeDuration, double messageSize)
{
	 
	if(computeDuration < 0) 
	{
		// TODO throw exception
		return;
	}
	
	if(messageSize < 0) 
	{
		// TODO throw exception
		return;
	}
	
	if(!name) 
	{
		// TODO throw exception
		return;
	}
	
	// create the task
	nativeTask = MSG_task_create(name, computeDuration, messageSize, NULL);
	
	nativeTask->data = (void*)this;
}

Task::Task(const char* name, Host* hosts, double* computeDurations, double* messageSizes)
{
	// TODO parallel task create	
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
throw(NativeException)
{
	m_host_t nativeHost = MSG_task_get_source(nativeTask);
	
	if(!nativeHost->data) 
	{
	// TODO throw the exception
	return NULL;
	}
	
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
{
	MSG_task_set_priority(nativeTask, priority);
}

Task& Task::get(int channel) 
throw(NativeException)
{
	m_task_t nativeTask = NULL;
	
	
	if(MSG_OK != MSG_task_get_ext(&nativeTask, channel , -1.0, NULL)) 
	{
		// TODO throw the NativeException
		return NULL;
	}
	
	return (*((Task*)(nativeTask->data)));
}

Task& Task::get(int channel, const Host& rHost) 
throw(NativeException)
{
	m_task_t nativeTask = NULL;
	
	
	if(MSG_OK != MSG_task_get_ext(&nativeTask, channel , -1.0, rHost.nativeHost)) 
	{
		// TODO throw the NativeException
		return NULL;
	}
	
	return (*((Task*)(nativeTask->data)));
}

Task& Task::get(int channel, double timeout, const Host& rHost) 
throw(NativeException)
{
	m_task_t nativeTask = NULL;
	
	
	if(MSG_OK != MSG_task_get_ext(&nativeTask, channel , timeout, rHost.nativeHost)) 
	{
		// TODO throw the NativeException
		return NULL;
	}
	
	return (*((Task*)(nativeTask->data)));
}

bool static Task::probe(int channel)
{
	return (bool)MSG_task_Iprobe(channel);
}

int Task::probe(int channel, const Host& rHost)
{
	return MSG_task_probe_from_host(chan_id,rHost.nativeHost);
}

void Task::execute(void) 
throw(NativeException)
{
	if(MSG_OK != MSG_task_execute(nativeTask))
	{
		// TODO throw NativeException
	}	
}

void Task::cancel(void) 
throw(NativeException)
{
	if(MSG_OK != MSG_task_cancel(nativeTask))
	{
		// TODO throw NativeException
	}	
}

void send(void) 
throw(NativeException)
{
}			

}

