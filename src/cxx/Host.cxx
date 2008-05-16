/*
 * Host.cxx
 *
 * Copyright 2006,2007 Martin Quinson, Malek Cherier           
 * All right reserved. 
 *
 * This program is free software; you can redistribute 
 * it and/or modify it under the terms of the license 
 *(GNU LGPL) which comes with this package. 
 *
 */
 
 /* Host class member functions implementation.
  */  
#include <Host.hpp>

namespace SimGrid
{
	namespace Msg
	{
		Host::Host()
		{
			nativeHost = NULL;
			data = NULL;
		}
		
		Host::Host(const Host& rHost)
		{
			this->nativeHost = rHost.nativeHost;
			this->data = rHost.getData();	
		}
		
		Host::~Host()
		{
			// NOTHING TODO
		}
		
		
		Host& Host::getByName(const char* hostName)
		throw(HostNotFoundException, InvalidParameterException, BadAllocException)
		{
			// check the parameters
			if(!hostName)
				throw InvalidParmeterException("hostName");
				
			m_host_t nativeHost = NULL;	// native host.
			Host* host = NULL;			// wrapper host.
			
			if(!(nativeHost = MSG_get_host_by_name(hostName))) 
				throw HostNotFoundException(hostName);
			
			if(!nativeHost->data) 
			{ // native host not associated yet with  its wrapper
			
				// instanciate a new wrapper 
				if(!(host = new Host())
					throw BadAllocException(hostName);
				
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
		
		void Host::all(Host*** hosts, int* len) 
		throw(InvalidParameterException, BadAllocException) 
		{
			/* check the parameters */
			if(!hosts)
				throw InvalidParameterException("hosts");
				
			if(len < 0)
				throw InvalidParameterException("len parameter must be positive");
			
			int count = xbt_fifo_size(msg_global->host);
			
			if(*len < count)
				throw InvalidParameterException("len parameter must be more than the number of installed host\n (use Host::getNumber() to get the number of hosts)");
			
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
					if(!(host = new Host())
					{
						// release all allocated memory.
						for(int i = 0; i < index; i++)
							delete (*(hosts)[i]);
						
						throw BadAllocException("to fill the table of the hosts installed on your platform");
					}
					
					host->nativeHost = nativeHost;
					nativeHost->data = (void*)host;
				}
				
				(*hosts)[index] = host;
		  }
		
		  *len = count;  
		}
		
		const char* Host::getName(void) const
		{
			return nativeHost->name;
		}
		
		void Host::setData(void* data)
		{
			this->data = data;
		}
		
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
		throw(MsgException, InvalidParameterException)
		{
			// checks the parameters
			if(channel < 0)
				throw InvalidParameterException("channel (must be more or equal to zero)");
				
			if(MSG_OK != MSG_task_put_with_timeout(rTask.nativeTask, nativeHost, channel , -1.0))
				throw MsgException("MSG_task_put_with_timeout() failed");
		} 
		
		void Host::put(int channel, const Task& rTask, double timeout) 
		throw(MsgException, InvalidParameterException) 
		{
			// checks the parameters
			if(channel < 0)
				throw InvalidParameterException("channel (must be more or equal to zero)");
				
			if(timeout < 0.0 && timeout != -1.0)
				throw InvalidParameterException("timeout (must be more or equal to zero or equal to -1.0)");	
				
				
		    if(MSG_OK != MSG_task_put_with_timeout(rTask.nativeTask, nativeHost, channel , timeout))
				throw MsgException("MSG_task_put_with_timeout() failed");
		}
		
		void Host::putBounded(int channel, const Task& rTask, double maxRate) 
		throw(MsgException, InvalidParameterException)
		{
		    // checks the parameters
			if(channel < 0)
				throw InvalidParameterException("channel (must be more or equal to zero)");
				
			if(maxRate < 0.0 && maxRate != -1.0)
				throw InvalidParameterException("maxRate (must be more or equal to zero or equal to -1.0)");	
		    
			if(MSG_OK != MSG_task_put_bounded(rTask.nativeTask, nativeHost, channel, maxRate))
				throw MsgException("MSG_task_put_bounded() failed");
		}
		
		void Host::send(const Task& rTask) 
		throw(NativeException)  
		{	
			MSG_error_t rv;
			
			char* alias = (char*)calloc(strlen(this->getName() + strlen(Process::currentProcess().getName()) + 2);
				
			if(!alias)
				throw BadAllocException("alias");
				
			sprintf(alias,"%s:%s", this->getName(),Process::currentProcess().getName());
				
			rv = MSG_task_send_with_timeout(rTask.nativeTask, alias, -1.0);
			
			free(alias);
			
			if(MSG_OK != rv)
				throw MsgException("MSG_task_send_with_timeout() failed");
		} 
		
		void Host::send(const char* alias, const Task& rTask) 
		throw(InvalidParameterException, MsgException) 
		{
			// check the parameters
			if(!alias)
				throw InvalidParmeterException("alias (must not be NULL)");
			
			if(MSG_OK != MSG_task_send_with_timeout(rTask.nativeTask, alias, -1.0))
				throw MsgException("MSG_task_send_with_timeout() failed");
		}
		
		void Host::send(const Task& rTask, double timeout) 
		throw(InvalidParameterException, BadAllocException, MsgException) 
		{
			// check the parameters
			if(timeout < 0 && timeout != -1.0)
				throw InvalidParameterException("timeout (must be positive or equal to zero or equal to -1.0)");
			MSG_error_t rv;
			
			char* alias = (char*)calloc(strlen(this->getName() + strlen(Process::currentProcess().getName()) + 2);
				
			if(!alias)
				throw BadAllocException("alias");
				
			sprintf(alias,"%s:%s", this->getName(),Process::currentProcess().getName());
				
				
			rv = MSG_task_send_with_timeout(rTask.nativeTask, alias, timeout);
			
			free(alias);
			
			if(MSG_OK != rv)
				throw MsgException("MSG_task_send_with_timeout() failed");
		}
		
		void Host::send(const char* alias, const Task& rTask, double timeout) 
		throw(InvalidParameterException, MsgException) 
		{
			// check the parameter
			
			if(!alias)
				throw InvalidParmeterException("alias (must not be NULL)");
				
			if(timeout < 0 && timeout != -1.0)
				throw InvalidParameterException("timeout (must be positive or equal to zero or equal to -1.0)");
					
			if(MSG_OK != MSG_task_send_with_timeout(rTask.nativeTask, alias, timeout))
				throw MsgException("MSG_task_send_with_timeout() failed");
		}
		
		
		void Host::sendBounded(const Task& rTask, double maxRate) 
		throw(InvalidParameterException, BadAllocException, MsgException) 
		{
			if(maxRate < 0 && maxRate != -1.0)
				throw InvalidParameterException("maxRate (must be positive or equal to zero or equal to -1.0)");
			
			MSG_error_t rv;
			
			char* alias = (char*)calloc(strlen(this->getName() + strlen(Process::currentProcess().getName()) + 2);
			
			if(!alias)
				throw BadAllocException("alias");
				
			sprintf(alias,"%s:%s", this->getName(),Process::currentProcess().getName());
				
			rv = MSG_task_send_bounded(rTask.nativeTask, alias, maxRate);
			
			free(alias);
			
			if(MSG_OK != rv)
				throw MsgException("MSG_task_send_bounded() failed");
		}  
		
		void Host::sendBounded(const char* alias, const Task& rTask, double maxRate) 
		throw(InvalidParameterException, MsgException) 
		{
			// check the parameters
			if(!alias)
				throw InvalidParameterException("alias (must not be NULL)");
			
			if(maxRate < 0 && maxRate != -1)
				throw InvalidParameterException("maxRate (must be positive or equal to zero or equal to -1.0)");
			
			if(MSG_OK != MSG_task_send_bounded(rTask.nativeTask, alias, maxRate))
				throw MsgException("MSG_task_send_bounded() failed");
			
		} 
	} // namspace Msg
} // namespace SimGrid