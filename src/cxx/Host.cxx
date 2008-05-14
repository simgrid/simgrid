#include <Host.hpp>

namespace msg
{
// Default constructor.
Host::Host()
{
	nativeHost = NULL;
	data = NULL;
}

// Copy constructor.
Host::Host(const Host& rHost)
{
}

// Destructor.
Host::~Host()
{
	nativeHost = NULL;
}

// Operations

Host& Host::getByName(const char* hostName)
throw(HostNotFoundException)
{
	m_host_t nativeHost;	// native host.
	Host* host = NULL;		// wrapper host.
	
	/* get the host by name	(the hosts are created during the grid resolution) */
	nativeHost = MSG_get_host_by_name(name);
	DEBUG2("MSG gave %p as native host (simdata=%p)",nativeHost,nativeHost->simdata);
	
	if(!nativeHost) 
	{// invalid host name
		// TODO throw HostNotFoundException
		return NULL;
	}
	
	if(!nativeHost->data) 
	{ // native host not associated yet with  its wrapper
	
		// instanciate a new wrapper 
		host = new Host();
	
		host->nativeHost = nativeHost; 
	
		// the native host data field is set with its wrapper returned 
		
		nativeHost->data = (void*)host;
	}
	
	// return the reference to cxx wrapper
	return *((Host*)nativeHost->data);				
}

int Host::getNumber(void)
{
	return MSG_get_host_number();
}

Host& Host::currentHost(void)
{
	Host* host = NULL;
	m_host_t nativeHost = MSG_host_self();
	
	if(!nativeHost->data) 
	{
		// the native host not yet associated with its wrapper
	
		// instanciate a new wrapper
		host = new Host();
	
		host->nativeHost = nativeHost;
	
		nativeHost->data = (void*)host;
	} 
	else 
	{
		host = (Host*)nativeHost->data;
	}
	
	return *host;
}

int Host::all(Host** hosts)  
{
	int count = xbt_fifo_size(msg_global->host);
	
	*hosts = new Host[count];
	int index;
 	m_host_t nativeHost;
 	Host* host;

 	m_host_t* table = (m_host_t *)xbt_fifo_to_array(msg_global->host);
	
	for(index = 0; index < count; index++) 
	{
		nativeHost = table[index];
		host = (Host*)(nativeHost->data);
	
		if(!host) 
		{
			host = new Host();
			
			host->nativeHost = nativeHost;
			nativeHost->data = (void*)host;
		}
		
		hosts[index] = host;
  }

  return count;  
}


const char* Host::getName(void) const
{
	return nativeHost->name;
}


void Host::setData(void* data)
{
	this->data = data;
}

// Getters/setters

void* Host::getData(void) const
{
	return this->data;
}

int Host::getRunningTaskNumber(void) const
{
 	return MSG_get_host_msgload(nativeHost); 
}

double Host::getSpeed(void) const
{
	return MSG_get_host_speed(nativeHost->simdata->smx_host);
}

bool Host::hasData(void) const
{
	return (NULL != this->data);
}

bool Host::isAvailable(void) const
{
	return (bool)SIMIX_host_get_state(nativeHost->simdata->smx_host);
}

void Host::put(int channel, const Task& rTask) 
throw(NativeException)
{
	if(MSG_OK != MSG_task_put_with_timeout(rTask.nativeTask, nativeHost, channel , -1.0))
	{
		// TODO throw NativeException
	}
} 

void Host::put(int channel, const Task& rTask, double timeout) 
throw(NativeException) 
{
    if(MSG_OK != MSG_task_put_with_timeout(rTask.nativeTask, nativeHost, channel , timeout))
	{
		// TODO throw NativeException
	}
}

void Host::putBounded(int channel, const Task& rTask, double maxRate) 
throw(NativeException)
{
    MsgNative.hostPutBounded(this, channel, task, maxrate);
    
	if(MSG_OK != MSG_task_put_bounded(rTask.nativeTask, nativeHost, channel, maxRate))
	{
		// TODO throw NativeException
	}
}




 

void Host::send(const Task& rTask) 
throw(NativeException)  
{	
	MSG_error_t rv;
	
	char* alias = (char*)calloc(strlen(this->getName() + strlen(Process::currentProcess().getName()) + 2);
	sprintf(alias,"%s:%s", this->getName(),Process::currentProcess().getName());
		
		
	rv = MSG_task_send_with_timeout(rTask.nativeTask, alias, -1.0);
	
	free(alias);
	
	if(MSG_OK != rv)
	{
		// TODO throw NativeException
	}
} 

void Host::send(const char* alias, const Task& rTask) 
throw(NativeException) 
{
	
	if(MSG_OK != MSG_task_send_with_timeout(rTask.nativeTask, alias, -1.0))
	{
		// TODO throw NativeException
	}
}

void Host::send(const Task& rTask, double timeout) 
throw(NativeException) 
{
	MSG_error_t rv;
	
	char* alias = (char*)calloc(strlen(this->getName() + strlen(Process::currentProcess().getName()) + 2);
	sprintf(alias,"%s:%s", this->getName(),Process::currentProcess().getName());
		
		
	rv = MSG_task_send_with_timeout(rTask.nativeTask, alias, timeout);
	
	free(alias);
	
	if(MSG_OK != rv)
	{
		// TODO throw NativeException
	}
}

void Host::send(const char* alias, const Task& rTask, double timeout) 
throw(NativeException) 
{
	if(MSG_OK != MSG_task_send_with_timeout(rTask.nativeTask, alias, timeout))
	{
		// TODO throw NativeException
	}
}


void Host::sendBounded(const Task& rTask, double maxRate) 
throw(NativeException) 
{
	
	MSG_error_t rv;
	
	char* alias = (char*)calloc(strlen(this->getName() + strlen(Process::currentProcess().getName()) + 2);
	sprintf(alias,"%s:%s", this->getName(),Process::currentProcess().getName());
		
	rv = MSG_task_send_bounded(rTask.nativeTask, alias, maxRate);
	
	free(alias);
	
	if(MSG_OK != rv)
	{
		// TODO throw NativeException
	}
}  

void Host::sendBounded(const char* alias, const Task& rTask, double maxRate) 
throw(NativeException) 
{
	if(MSG_OK != MSG_task_send_bounded(rTask.nativeTask, alias, maxRate))
	{
		// TODO throw NativeException
	}
} 

	
}