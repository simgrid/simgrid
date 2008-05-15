/*
 * Host.hpp
 *
 * Copyright 2006,2007 Martin Quinson, Malek Cherier           
 * All right reserved. 
 *
 * This program is free software; you can redistribute 
 * it and/or modify it under the terms of the license 
 *(GNU LGPL) which comes with this package. 
 *
 */  
#ifndef MSG_HOST_HPP
#define MSG_HOST_HPP


/*! \brief Host class declaration.
 *
 * An host instance represents a location (any possible place) where a process may run. 
 * Thus it is represented as a physical resource with computing capabilities, some 
 * mailboxes to enable running process to communicate with remote ones, and some private 
 * data that can be only accessed by local process. An instance of this class is always 
 * binded with the corresponding native host. All the native hosts are automaticaly created 
 * during the call of the static method Msg::createEnvironment(). This method takes as parameter
 * the platform file which describes all elements of the platform (host, link, root..).
 * You never need to create an host your self.
 *
 * The best way to get an host is to call the static method 
 * Host.getByName() which returns a reference.
 *
 * For example to get the instance of the host. If your platform
 * file description contains an host named "Jacquelin" :
 *
 * \verbatim
using namespace SimGrid.Msg;

Host jacquelin;

try 
{ 
	jacquelin = Host::getByName("Jacquelin");
}
catch(HostNotFoundException e) 
{
	cerr << e.toString();
}
...
\endverbatim
 *
 */

#ifndef __cplusplus
	#error Host.hpp requires C++ compilation (use a .cxx suffix)
#endif

#include <InvalidArgumentException.hpp>
#include <BadAllocException.hpp>
#include <HostNotFoundException.hpp>

// namespace SimGrid::Msg
namespace SimGrid
{
	namespace Msg
	{
		// Declaration of the class SimGrid::Msg::Host.
		class Host
		{
			// Desable the default constructor.
			// The best way to get an host instance is to use the static method Host::getByName().
			
			private :
				
			// Default constructor (desabled).
				Host();
			
			public:	
				
			// Copy constructor (desabled).
				Host(const Host& rHost);
				
			// Destructor (desable).
				virtual ~Host();
			
			// Operations
			
				/*! \brief Host::getByName() - returns an host by its name
				 *
				 * This static method returns a reference to the host instance associated 
				 * with a native host of your platform. This is the best way to get a host.
				 *
				 * \param hostName	The name of the host.
				 *
				 * \return			If successful the method returns a reference to the instance
				 *					associated with the native host having the name specified
				 *					as parameter of your platform. Otherwise the method throws
				 *					one of the exceptions detailed below.
				 *
				 * \exception		[HostNotFoundException]		if no host with the specified name
				 *												was found.
				 *					[InvalidParameterException]	if the hostName parameter is invalid (NULL).
				 *					[BadAllocException]			if there is not enough memory to allocate the host.
				 */ 
				static Host& getByName(const char* hostName)
				throw(HostNotFoundException, InvalidParameterException, BadAllocException);
				
				/*! \brief Host::getNumber() - returns the number of the installed hosts.
				 *
				 * \return			The number of hosts installed.
				 */
				static int getNumber(void);
				
				
				/*! \brief Host::currentHost() - This static method returns the location on which the current 
				 * process is executed.
				 *
				 * \return			The host of the current process.
				 *
				 * \see				Process::currentProcess().
				 */
				static Host& currentHost(void)
				throw(InvalidParameterException, BadAllocException);
				
				/*! \brief Host::all() - This static method retrieves all of the hosts of the installed platform.
				 *
				 * \param hosts		A pointer to array of Host pointers that receives all the hosts of the platform.
				 *
				 * \param len		A pointer to the length of the table of pointers.
				 *
				 * \return			If successful the hosts table is filled and
				 *					the parameter len is set with the number of hosts of the platform.
				 *					Otherwise the method throw one of the exception described below.
				 *
				 * \exception		[InvalidParameterException]	if the parameter hosts is invalid or
				 *												if the parameter len is negative or
				 *												less than the number of hosts installed
				 *												on the current platform.
				 *					[BadAllocException]			If the method can't allocate memory to fill
				 *												the table of hosts.
				 *
				 *
				 * \remark			To get the number of hosts installed on your platform use the static method
				 *					Host::getNumber().
				 *
				 * \see				Host::getNumber().
				 *
				 *\verbatim
				 * // This example show how to use this method to get the list of hosts installed on your platform.
				 *
				 *	using namespace SimGrid::Msg;
				 *	using <iostream>
				 *
				 *	// (1) get the number of hosts.
				 *	int number = Host::getNumber();
				 *	
				 *	// (2) allocate the array that receives the list of hosts.
				 *	HostPtr* ar = new HostPtr[number];	// HostPtr is defined as (typedef Host* HostPtr at the end of the
				 *										// declaration of this class.
				 *
				 *	// (3) call the method
				 *	try
				 *	{
				 *		Host::all(&ar, &number);
				 *	}
				 *	catch(BadAllocException e)
				 *	{
				 *		cerr << e.toString() << endl;
				 *		...
				 *	}
				 *	catch(InvalidArgumentException e)
				 *	{
				 *		cerr << e.toString() << endl;
				 *		..
				 *	} 
				 *
				 *	// (4) use the table of host (for example print all the name of all the hosts);
				 *	
				 *	for(int i = 0; i < number ; i++)
				 *		cout << ar[i]->getName() << endl;
				 *
				 *	...
				 *		
				 *	// (5) release the allocate table
				 *	
				 *	delete[] ar;
				 *
				 */ 
				static void all(Host*** hosts /*in|out*/, int* len /*in|out*/);
				
				
				const char* getName(void) const;
				
				void setData(void* data);
				
				// Getters/setters
				
				void* getData(void) const;
				
				int Host::getRunningTaskNumber(void) const;
				
				double getSpeed(void) const;
				
				bool hasData(void) const;
				
				bool isAvailble(void) const;
				
				void put(int channel, const Task& rTask)
				throw(NativeException);
				
				void put(int channel, Task task, double timeout) 
				throw(NativeException);
				
				void Host::putBounded(int channel, const Task& rTask, double maxRate) 
				throw(NativeException);
				
				
				void send(const Task& rTask) 
				throw(NativeException);
				
				void send(const char* alias, const Task& rTask) 
				throw(NativeException);
				
				void send(const Task& rTask, double timeout) 
				throw(NativeException);
				
				void send(const char* alias, const Task& rTask, double timeout) 
				throw(NativeException);
				
				void sendBounded(const Task& rTask, double maxRate) 
				throw(NativeException);
				
				void sendBounded(const char* alias, const Task& rTask, double maxRate) 
				throw(NativeException);
				
			 
			
			
			protected:
			// Attributes.
			
				/**
				 * This attribute represents the msg native host object. 
				 * It is set automaticatly during the call of the static 
				 * method Host::getByName().
				 *
				 * @see				Host::getByName().
				 */ 
				m_host_t nativeHost;
			
			private:
				/**
				 * User host data. 
				 */ 
				void* data;
		};
		
		typedef Host* HostPtr;
	} // namespace Msg
} // namespace SimGrid

#endif // !MSG_HOST_HPP