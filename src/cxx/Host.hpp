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
#include <MsgException.hpp>

// namespace SimGrid::Msg
namespace SimGrid
{
	namespace Msg
	{
		// Declaration of the class SimGrid::Msg::Host.
		class Host // final class.
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
				
				/*! \brief Host::getName() - This method return the name of the Msg host object.
				 *
				 * \return			The name of the host object.
				 */
				const char* getName(void) const;
				
				/*! \brief Host::setData() - Set the user data of an host object.
				 *
				 * \param data		The user data to set.
				 */
				void setData(void* data);
				
				/*! \brief Host::getData() - Get the user data of a host object.
				 *
				 * \return			The user data of the host object.
				 */
				void* getData(void) const;
				
				/*! \brief Host::hasData() - Test if an host object has some data.
				 *
				 * \return			This method returns true if the host object has some user data.
				 *					Otherwise the method returns false.
				 */
				bool hasData(void) const;
				
				/*! \brief Host::getRunningTaskNumber() - returns the number of tasks currently running on a host.
				 *
				 * \return			The number of task currently running of the host object.
				 *
	 			 * \remark			The external load is not taken in account.
	 			 */
				int getRunningTaskNumber(void) const;
				
				/*! \brief Host::getSpeed() - returns the speed of the processor of a host,
				 * regardless of the current load of the machine.
				 *
				 * \return			The speed of the processor of the host in flops.
				 */ 
				double getSpeed(void) const;
				
				/*! \brief Host::isAvailable - tests if an host is availabled.
				 * 
				 * \return			Is the host is availabled the method returns
				 *					true. Otherwise the method returns false.
				 */ 
				bool isAvailble(void) const;
				
				/* ! \brief Host::put() - put a task on the given channel of a host .
				 *
				 * \param channel	The channel where to put the task.
				 * \param rTask		A refercence to the task object containing the native task to
				 *					put on the channel specified by the parameter channel.
				 *
				 * \return			If successful the task is puted on the specified channel. Otherwise
				 *					the method throws one of the exceptions described below.
				 *
				 * \exception		[MsgException] 				if an internal error occurs.
				 *					[InvalidParameterException]	if the value of the channel specified as
				 *												parameter is negative.
				 */
				void put(int channel, const Task& rTask)
				throw(MsgException, InvalidParameterException);
				
				/* ! \brief Host::put() - put a task on the given channel of a host object (waiting at most timeout seconds).
				 *
				 * \param channel	The channel where to put the task.
				 * \param rTask		A refercence to the task object containing the native task to
				 *					put on the channel specified by the parameter channel.
				 * \param timeout	The timeout in seconds.
				 *
				 * \return			If successful the task is puted on the specified channel. Otherwise
				 *					the method throws one of the exceptions described below.
				 *
				 * \exception		[MsgException] 				if an internal error occurs.
				 *					[InvalidParameterException]	if the value of the channel specified as
				 *												parameter is negative or if the timeout value
				 *												is less than zero and différent of -1.
				 *
				 * \remark			To specify no timeout set the timeout value with -1.0.
				 */
				void put(int channel, Task task, double timeout) 
				throw(MsgException, InvalidParameterException);
				
				/* ! \brief Host::putBounded() - put a task on the given channel of a host object (capping the emission rate to maxrate).
				 *
				 * \param channel	The channel where to put the task.
				 * \param rTask		A refercence to the task object containing the native task to
				 *					put on the channel specified by the parameter channel.
				 * \param maxRate	The maximum rate.
				 *
				 * \return			If successful the task is puted on the specified channel. Otherwise
				 *					the method throws one of the exceptions described below.
				 *
				 * \exception		[MsgException] 				if an internal error occurs.
				 *					[InvalidParameterException]	if the value of the channel specified as
				 *												parameter is negative or if the maxRate parameter value
				 *												is less than zero and différent of -1.0.
				 *
				 * \remark			To specify no rate set the maxRate parameter value with -1.0.
				 */
				void putBounded(int channel, const Task& rTask, double maxRate) 
				throw(MsgException, InvalidParameterException);
				
				/* ! brief Host::send() - sends the given task to mailbox identified by the default alias.
				 *
				 * \param rTask		A reference to the task object containing the native msg task to send.
				 *
				 * \return			If successful the task is sended to the default mailbox. Otherwise the
				 *					method throws one of the exceptions described below.
				 *
				 * \exception		[BadAllocException]			if there is not enough memory to allocate
				 *												the default alias variable.
				 *					[MsgException]				if an internal error occurs.
				 */
				void send(const Task& rTask) 
				throw(BadAllocException, MsgException);
				
				/* ! brief Host::send() - sends the given task to mailbox identified by the specified alias parameter.
				 *
				 * \param rTask		A reference to the task object containing the native msg task to send.
				 * \param alias		The alias of the mailbox where to send the task.
				 *
				 * \return			If successful the task is sended to the default mailbox. Otherwise the
				 *					method throws one of the exceptions described below.
				 *
				 * \exception		[InvalidParameterException]	if alias parameter is invalid (NULL).
				 *					[BadAllocException]			if there is not enough memory to allocate
				 *												the default alias variable.
				 *					[MsgException]				if an internal error occurs.
				 */
				void send(const char* alias, const Task& rTask) 
				throw(InvalidParameterException, BadAllocException, MsgException);
				
				/* ! brief Host::send() - sends the given task to mailbox identified by the default alias
				 *  (waiting at most timeout seconds).
				 *
				 * \param rTask		A reference to the task object containing the native msg task to send.
				 * \param timeout	The timeout value to wait for.
				 *
				 * \return			If successful the task is sended to the default mailbox. Otherwise the
				 *					method throws one of the exceptions described below.
				 *
				 * \exception		[BadAllocException]			if there is not enough memory to allocate
				 *												the default alias variable.
				 *					[InvalidParameterException]	if the timeout value is negative and different of
				 *												-1.0.			
				 *					[MsgException]				if an internal error occurs.
				 *
				 * \remark			To specify no timeout set the timeout value with -1.0 or use the previous 
				 *					version of this method.
				 *
				 */
				void send(const Task& rTask, double timeout) 
				throw(NativeException);
				
				/* ! brief Host::send() - sends the given task to mailbox identified by the parameter alias
				 *  (waiting at most timeout seconds).
				 *
				 * \param alias		The alias of the mailbox to send the task.
				 * \param rTask		A reference to the task object containing the native msg task to send.
				 * \param timeout	The timeout value to wait for.
				 *
				 * \return			If successful the task is sended to the default mailbox. Otherwise the
				 *					method throws one of the exceptions described below.
				 *
				 * \exception		[InvalidParameterException]	if the timeout value is negative and different of
				 *												-1.0 or if the alias parameter is invalid (NULL).			
				 *					[MsgException]				if an internal error occurs.
				 *
				 * \remark			To specify no timeout set the timeout value with -1.0 or use the previous 
				 *					version of this method.
				 *
				 */
				void send(const char* alias, const Task& rTask, double timeout) 
				throw(InvalidParameterException, MsgException);
				
				/* ! brief Host::sendBounded() - sends the given task to mailbox associated to the default alias
				 *  (capping the emission rate to maxRate).
				 *
				 * \param rTask		A reference to the task object containing the native msg task to send.
				 * \param maxRate	The maximum rate value.
				 *
				 * \return			If successful the task is sended to the default mailbox. Otherwise the
				 *					method throws one of the exceptions described below.
				 *
				 * \exception		[InvalidParameterException]	if the maximum rate value is negative and different of
				 *												-1.0.			
				 *					[MsgException]				if an internal error occurs.
				 *					[BadAllocException]			if there is not enough memory to allocate
				 *												the default alias variable.
				 *
				 * \remark			To specify no rate set its value with -1.0.
				 *
				 */
				void sendBounded(const Task& rTask, double maxRate) 
				throw(InvalidParameterException, BadAllocException, MsgException);
				
				/* ! brief Host::sendBounded() - sends the given task to mailbox identified by the parameter alias
				 *  (capping the emission rate to maxRate).
				 *
				 * \param alias		The alias of the mailbox where to send the task.
				 * \param rTask		A reference to the task object containing the native msg task to send.
				 * \param maxRate	The maximum rate value.
				 *
				 * \return			If successful the task is sended to the default mailbox. Otherwise the
				 *					method throws one of the exceptions described below.
				 *
				 * \exception		[InvalidParameterException]	if the maximum rate value is negative and different of
				 *												-1.0 or if the alias parameter is invalid (NULL).			
				 *					[MsgException]				if an internal error occurs.
				 *
				 * \remark			To specify no rate set its value with -1.0.
				 *
				 */
				void sendBounded(const char* alias, const Task& rTask, double maxRate) 
				throw(InvalidParameterException, MsgException);
			
			protected:
			// Attributes.
			
				/**
				 * This attribute represents the msg native host object. 
				 * It is set automaticatly during the call of the static 
				 * method Host::getByName().
				 *
				 * \see				Host::getByName().
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