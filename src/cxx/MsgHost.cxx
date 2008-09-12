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

#include <MsgTask.hpp>
#include <MsgProcess.hpp>

#include <MsgHost.hpp>

#include <stdlib.h>
#include <stdio.h>

#include <msg/msg.h>
#include <msg/private.h>

#include <xbt/fifo.h>

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
		throw(HostNotFoundException, NullPointerException, BadAllocException)
		{
			// check the parameters
			if(!hostName)
				throw NullPointerException("hostName");
				
			m_host_t nativeHost = NULL;	// native host.
			Host* host = NULL;			// wrapper host.
			
			if(!(nativeHost = MSG_get_host_by_name(hostName))) 
				throw HostNotFoundException(hostName);
			
			if(!nativeHost->data) 
			{ // native host not associated yet with  its wrapper
			
				// instanciate a new wrapper 
				if(!(host = new Host()))
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
		throw(InvalidArgumentException, BadAllocException) 
		{
			// check the parameters
			if(!hosts)
				throw InvalidArgumentException("hosts");
				
			if(len < 0)
				throw InvalidArgumentException("len parameter must be positive");
			
			int count = xbt_fifo_size(msg_global->host);
			
			if(*len < count)
				throw InvalidArgumentException("len parameter must be more than the number of installed host\n (use Host::getNumber() to get the number of hosts)");
			
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
					if(!(host = new Host()))
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
			return MSG_get_host_speed(nativeHost);
		}
		
		bool Host::hasData(void) const
		{
			return (NULL != this->data);
		}
		
		int Host::isAvailable(void) const
		{
			return SIMIX_host_get_state(nativeHost->simdata->smx_host);
		}
		
		void Host::put(int channel, Task* task) 
		throw(MsgException, InvalidArgumentException)
		{
			// checks the parameters
			if(channel < 0)
				throw InvalidArgumentException("channel (must be more or equal to zero)");
				
			if(MSG_OK != MSG_task_put_with_timeout(task->nativeTask, nativeHost, channel , -1.0))
				throw MsgException("MSG_task_put_with_timeout() failed");
		} 

		void Host::put(int channel, Task* task, double timeout) 
		throw(MsgException, InvalidArgumentException) 
		{
			// checks the parameters
			if(channel < 0)
				throw InvalidArgumentException("channel (must be more or equal to zero)");
				
			if(timeout < 0.0 && timeout != -1.0)
				throw InvalidArgumentException("timeout (must be more or equal to zero or equal to -1.0)");	
				
				
		    if(MSG_OK != MSG_task_put_with_timeout(task->nativeTask, nativeHost, channel , timeout))
				throw MsgException("MSG_task_put_with_timeout() failed");
		}
		
		void Host::putBounded(int channel, Task* task, double maxRate) 
		throw(MsgException, InvalidArgumentException)
		{
		    // checks the parameters
			if(channel < 0)
				throw InvalidArgumentException("channel (must be more or equal to zero)");
				
			if(maxRate < 0.0 && maxRate != -1.0)
				throw InvalidArgumentException("maxRate (must be more or equal to zero or equal to -1.0)");	
		    
			if(MSG_OK != MSG_task_put_bounded(task->nativeTask, nativeHost, channel, maxRate))
				throw MsgException("MSG_task_put_bounded() failed");
		}
		
		void Host::send(Task* task) 
		throw(MsgException, BadAllocException)  
		{	
			MSG_error_t rv;
			
			char* alias = (char*)calloc(strlen(this->getName())+ strlen(Process::currentProcess().getName()) + 2, sizeof(char));
				
			if(!alias)
				throw BadAllocException("alias");
				
			sprintf(alias,"%s:%s", this->getName(),Process::currentProcess().getName());
				
			rv = MSG_task_send_with_timeout(task->nativeTask, alias, -1.0);
			
			free(alias);
			
			if(MSG_OK != rv)
				throw MsgException("MSG_task_send_with_timeout() failed");
		} 
		
		void Host::send(const char* alias, Task* task) 
		throw(InvalidArgumentException, MsgException) 
		{
			// check the parameters
			if(!alias)
				throw InvalidArgumentException("alias (must not be NULL)");
			
			if(MSG_OK != MSG_task_send_with_timeout(task->nativeTask, alias, -1.0))
				throw MsgException("MSG_task_send_with_timeout() failed");
		}
		
		void Host::send(Task* task, double timeout) 
		throw(InvalidArgumentException, BadAllocException, MsgException) 
		{
			// check the parameters
			if(timeout < 0 && timeout != -1.0)
				throw InvalidArgumentException("timeout (must be positive or equal to zero or equal to -1.0)");
			
			MSG_error_t rv;
			
			char* alias = (char*)calloc(strlen(this->getName()) + strlen(Process::currentProcess().getName()) + 2, sizeof(char));
				
			if(!alias)
				throw BadAllocException("alias");
				
			sprintf(alias,"%s:%s", this->getName(),Process::currentProcess().getName());
				
				
			rv = MSG_task_send_with_timeout(task->nativeTask, alias, timeout);
			
			free(alias);
			
			if(MSG_OK != rv)
				throw MsgException("MSG_task_send_with_timeout() failed");
		}
		
		void Host::send(const char* alias, Task* task, double timeout) 
		throw(InvalidArgumentException, MsgException) 
		{
			// check the parameter
			
			if(!alias)
				throw InvalidArgumentException("alias (must not be NULL)");
				
			if(timeout < 0 && timeout != -1.0)
				throw InvalidArgumentException("timeout (must be positive or equal to zero or equal to -1.0)");
					
			if(MSG_OK != MSG_task_send_with_timeout(task->nativeTask, alias, timeout))
				throw MsgException("MSG_task_send_with_timeout() failed");
		}
		
		
		void Host::sendBounded(Task* task, double maxRate) 
		throw(InvalidArgumentException, BadAllocException, MsgException) 
		{
			if(maxRate < 0 && maxRate != -1.0)
				throw InvalidArgumentException("maxRate (must be positive or equal to zero or equal to -1.0)");
			
			MSG_error_t rv;
			
			char* alias = (char*)calloc(strlen(this->getName()) + strlen(Process::currentProcess().getName()) + 2, sizeof(char));
			
			if(!alias)
				throw BadAllocException("alias");
				
			sprintf(alias,"%s:%s", this->getName(),Process::currentProcess().getName());
				
			rv = MSG_task_send_bounded(task->nativeTask, alias, maxRate);
			
			free(alias);
			
			if(MSG_OK != rv)
				throw MsgException("MSG_task_send_bounded() failed");
		}  
		
		void Host::sendBounded(const char* alias, Task* task, double maxRate) 
		throw(InvalidArgumentException, MsgException) 
		{
			// check the parameters
			if(!alias)
				throw InvalidArgumentException("alias (must not be NULL)");
			
			if(maxRate < 0 && maxRate != -1)
				throw InvalidArgumentException("maxRate (must be positive or equal to zero or equal to -1.0)");
			
			if(MSG_OK != MSG_task_send_bounded(task->nativeTask, alias, maxRate))
				throw MsgException("MSG_task_send_bounded() failed");
			
		}
	} // namspace Msg
} // namespace SimGrid

