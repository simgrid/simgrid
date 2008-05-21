/*
 * Process.hpp
 *
 * Copyright 2006,2007 Martin Quinson, Malek Cherier           
 * All right reserved. 
 *
 * This program is free software; you can redistribute 
 * it and/or modify it under the terms of the license 
 *(GNU LGPL) which comes with this package. 
 *
 */  
 
#ifndef MSG_PROCESS_HPP
#define MSG_PROCESS_HPP

// Compilation C++ recquise
#ifndef __cplusplus
	#error Process.hpp requires C++ compilation (use a .cxx suffix)
#endif


namespace SimGrid
{
	namespace Msg
	{
		// SimGrid::Msg::Process class declaration.
		class Process
		{
			friend ApplicationHandler;
			
			private;
			
				// Disable the default constructor.
				Process();
			
			public:
			
				/*! \brief  Constructs a process from the name of the host and its name.
				 *
				 * \param hostName		The host name of the process to create.
				 * \param name			The name of the process to create.
				 *
				 * \exception			If the constructor failed, it throws one of the exceptions described
				 *						below:
				 *						
				 *						[NullPointerException]		if the name of the process is NULL or if the
				 *													name of the host is NULL.
				 * 						[HostNotFoundException]		if the host is not found.
				 */	
				Process(const char* hostName, const char* name)
				throw(NullPointerException, HostNotFoundException);
			
				/*! \brief Constructs a process from a reference to an host object and its name.
				 *
				 * \param rHost			A reference to the host object representing the native
				 *						MSG host where to create the process.
				 * \param name			The name of the process to create.
				 *
				 * \exception			If the constructor failed, it throws the exception described
				 *						below:
				 *
				 * 						[NullPointerException]		if the name of process is NULL.
				 */ 
				Process(const Host& rHost, const char* name)
				throw(NullPointerException);
				
				/*! brief Construct a proces from reference to an host object and the name of the process.
				 * This constuctor takes also the list of the arguments of the process.
				 *
				 * \param host			A reference to the host where to create the process.
				 * \param name			The name of the process to create.
				 * \param argc			The number of the arguments of the process to create.
				 * \param argv			The list of arguments of the process to create.
				 *
				 * \exception			If the constructor failed, it throws one of the exceptions described
				 *						below:
				 *
				 *						[NullPointerException]		if the name of the host is NULL or
				 *													if the name of the process is NULL.
				 *						[InvalidArgumentException]  if the value of the parameter argv is
				 *													negative.
				 *						[LogicException]			if the parameter argv is NULL and the 
				 *													parameter argc is different than zero
				 *													if the parameter argv is not NULL and 
				 *													the parameter argc is zero.
				 */
				Process(const Host& rHost, const char* name, int argc, char** argv)
				throw(NullPointerException, InvalidArgumentException, LogicException);
				
				/*! brief Constructs a proces from the name of a host and the name of the process.
				 * This constuctor takes also the list of the arguments of the process.
				 *
				 * \param hostName		The name of the host where to create the process.
				 * \param name			The name of the process to create.
				 * \param argc			The number of the arguments of the process to create.
				 * \param argv			The list of arguments of the process to create.
				 *
				 * \exception			If the constructor failed, it throws one of the exceptions described
				 *						below:
				 *						
				 *						[NullPointerException]		if the name of the process or if the name
				 *													of the host is NULL.
				 *						[InvalidArgumentException]  if the value of the parameter argv is
				 *													negative.
				 *						[LogicException]			if the parameter argv is NULL and the 
				 *													parameter argc is different than zero
				 *													if the parameter argv is not NULL and 
				 *													the parameter argc is zero.
				 *						[HostNotFoundException]		if the specified host is no found.
				 */
				Process(const char* hostName, const char* name, int argc, char** argv)
				throw(NullPointerException, HostNotFoundException);
			
				/*! \brief Process::killAll() - kill all the running processes of the simulation.
				 *
				 * \param resetPID		Should we reset the PID numbers. A negative number means no reset
	     		 *						and a positive number will be used to set the PID of the next newly
	     		 *						created process.
	     		 *
	     		 * \return				The static method returns the PID of the next created process.
	     		 */			
				static int killAll(int resetPID);
			
				/*! \brief Process::suspend() - Suspend an MSG process.
				 * 
				 * \excetpion			If this method failed, it throws one the exception described below:
				 *
				 *						[MsgException]	if an internal exception occurs during the operation.
				 */
				void suspend(void)
				throw(MsgException);
				
				
			
				/*! \brief Process::resume() - Resume the MSG process.
				 *
				 * \exception			If this method failed, it throws the exception described below:
				 *
				 *						[MsgException] if an internal exception occurs during the operation.
				 */
				void resume(void) 
				throw(MsgException);
			
				/*! \brief Process::isSuspend() - Tests if a process is suspended.
				 *
				 * \return				This method returns true is the process is suspended.
				 *						Otherwise the method returns false.
				 */
				bool isSuspended(void);
			
				/*! \brief Process::getHost() - Retrieves the host of a process object.
				 *
				 * \return				The method returns a reference to the
				 *						host of the process.
				 *
				 */
				Host& getHost(void); 
			
				/*! \brief Process::fromPID() - Retrieves a process from its PID.
				 *
				 * \param PID			The PID of the process to retrieve.
				 *
				 * \return				If successful the method returns a reference to
				 *						to process. Otherwise, the method throws one of
				 *						the exceptions described below:
				 *
				 * \exception			[ProcessNotFoundException]	if the process is not found (no process with this PID).
				 *
				 *						[InvalidArgumentException]	if the parameter PID is less than 1.
				 *
				 *						[MsgException]				if a native error occurs during the operation.
				 */
				static Process& fromPID(int PID)
				throw(ProcessNotFoundException, InvalidArgumentException, MsgException); 
			
				/*! \brief Process::getPID() - Gets the PID of a process object.
				 *
				 * \return				This method returns the PID of a process object.
				 */
				int getPID(void);
				
				/*! \brief Process::getPPID() - Gets the parent PID of a process object.
				 *
				 * \return				This method returns the parent PID of a process object.
				 */
				int getPPID(void);
				
				/*! \brief Process::getName() - Gets the name of a process object.
				 *
				 * \return				This method returns the name of a process object.
				 */
				const char* getName(void) const;
			
				/*! \brief Process::currentProcess() - Retrieves the current process.
				 *
				 * \return				This method returns a reference to the current process. Otherwise
				 *						the method throws the excepction described below:
				 *
				 * \exception			[MsgException]	if an internal exception occurs.
				 */
				static Process& currentProcess(void)
				throw (MsgException);
			
				/*! \brief Process::currentProcessPID() - Retrieves the PID of the current process.
				 *
				 * \return				This method returns the PID of the current process.
				 */
				static int currentProcessPID(void);
				
				/*! \brief Process::currentProcessPPID() - Retrieves the parent PID of the current process.
				 *
				 * \return				This method returns the parent PID of the current process.
				 */
				static int currentProcessPPID(void);
			
				/*! \brief Process::migrate() - Migrate a process object to an another host.
				 *
				 * \param rHost			A reference to the host to migrate the process to.
				 *
				 * \return 				If successful the method migrate the process to the specified
				 *						host. Otherwise the method throws the exception described
				 *						below:
				 *
				 * \exception			[MsgException]	if an internal exception occurs.
				 */
				void migrate(const Host& rHost)
				throw(MsgException);
			
				/*! \brief Process::sleep() - Makes the current process sleep until time seconds have elapsed. 
				 *
				 * \param seconds 			The number of seconds to sleep.
				 *
				 * \execption				If this method failed, it throws one of the exceptions described
				 *							below: 
				 *
				 *							[InvalidArgumentException]	if the parameter seconds is
				 *							less or equals to zero.
				 *
				 *							[MsgException] if an internal exception occurs.
				 */
				static void sleep(double seconds)
				throw(InvalidArgumentException, MsgException);
			
				/*! \brief Process::putTask() - This method puts a task on a given channel of a given host.
				 *
				 * \exception			If this method failed, it throws one of the exceptions described
				 *						below:
				 *
				 *						[InvalidArgumentException] if the channel number is negative.
				 *
				 *						[MsgException]				if an internal exception occurs.
				 */
				void putTask(const Host& rHost, int channel, const Task& rTask)
				throw(InvalidArgumentException, MsgException);
			
				/*! \brief Process::putTask() - This method puts a task on a given channel of a given host (waiting at most given time).
				 *
				 * \exception			If this method failed, it throws one of the exceptions described below:
				 *
				 *						[MsgException] 				if an internal error occurs.
				 *
				 *						[InvalidArgumentException]	if the value of the channel specified as
				 *													parameter is negative or if the timeout value
				 *													is less than zero and différent of -1.
				 *
				 * \remark				Set the timeout with -1.0 to disable it.
				 */
				void putTask(const Host& rHost, int channel, const Task& rTask, double timeout) 
				throw(InvalidArgumentException, MsgException);
			
				/*! \brief Process::getTask() - Retrieves a task from a channel number (waiting at most given time).
				 *
				 * \param channel		The number of the channel where to get the task.
				 *
				 * \return				If successful the method returns a reference to 
				 *						the getted task. Otherwise the method throws one
				 *						of the exceptions described below:
				 *
				 * \exception			[InvalidArgumentException]	if the channel number is negative.
				 *
				 *						[MsgException]				if an internal exception occurs.
				 */
				Task& getTask(int channel) 
				throw(InvalidArgumentException, MsgException);
			
				/*! \brief Process::taskGet() - Retrieves a task from a channel number (waiting at most given time).
				 *
				 * \param channel		The number of the channel where to get the task.
				 * \param timeout		The timeout value.
				 *
				 * \return				If successful the method returns a reference to 
				 *						the getted task. Otherwise the method throws one
				 *						of the exceptions described below:
				 *
				 * \exception			[InvalidArgumentException]	if the channel number is negative
				 *													or if the timeout value is less than
				 *													zero and different of -1.0.
				 *						[MsgException]				if an internal exception occurs.
				 */
				Task& getTask(int channel, double timeout) 
				throw(InvalidArgumentException, MsgException);
			
				/*! \brief Process::taskGet() - Retrieves a task from a channel number and a host.
				 *
				 * \param channel		The number of the channel where to get the task.
				 * \param host			The host of the channel to get the task.
				 *
				 * \return				If successful the method returns a reference to 
				 *						the getted task. Otherwise the method throws one
				 *						of the exceptions described below.
				 *
				 * \exception			[InvalidArgumentException]	if the channel number is negative.
				 *						[MsgException]				if an internal exception occurs.
				 */
				Task& getTask(int channel, const Host& rHost) 
				throw(InvalidArgumentException, MsgException);
			
				/*! \brief Process::taskGet() - Retrieves a task from a channel number and a host (waiting at most given time).
				 *
				 * \param channel		The number of the channel where to get the task.
				 * \param timeout		The timeout value.
				 * \param rHost			The host owning the channel.
				 *
				 * \return				If successful the method returns a reference to 
				 *						the getted task. Otherwise the method throws one
				 *						of the exceptions described below:
				 *
				 * \exception			[InvalidArgumentException]	if the channel number is negative or
				 *													if the timeout value is negative and different
				 *													of -1.0.
				 *						[MsgException]				if an internal exception occurs.
				 *
				 * \remark				Set the timeout with -1.0 to disable it.
				 */
				Task& getTask(int channel, double timeout, const Host& rHost)
				throw(InvalidArgumentException MsgException);
			
				/*! \brief Process::sendTask() - Sends the given task in the mailbox identified by the specified alias
				 * (waiting at most given time).
				 *
				 * \param alias				The alias of the mailbox where to send the task.
				 * \param rTask				A reference to the task object to send.
				 * \param timeout			The timeout value.
				 *
				 * \exception				If this method failed, it throws one of the exceptions described below:
				 *
				 *							[NullPointerException]		if the alias specified as parameter is NULL.
				 *	
				 *							[InvalidArgumentException]	if the timeout value is negative and different than
				 *														-1.0
				 *							[MsgException]				if an internal exception occurs.
				 *
				 * \remark					Set the timeout with -1.0 to disable it.
				 */
				void sendTask(const char* alias, const Task& rTask, double timeout) 
				throw(NullPointerException, InvalidArgumentException, MsgException);
			
				/*! \brief Process::sendTask() - Sends the given task in the mailbox identified by the specified alias.
				 *
				 * \param alias				The alias of the mailbox where to send the task.
				 * \param rTask				A reference to the task object to send.
				 *
				 * \exception				If this method failed, it throws one of the exceptions described below:
				 *
				 *							[NullPointerException]		if the alias parameter is NULL.
				 *
				 *							[MsgException]				if an internal exception occurs.
				 *
				 */
				void sendTask(const char* alias, const Task& rTask) 
				throw(NullPointerException, MsgException);
			
				/*! \brief Process::sendTask() - Sends the given task in the mailbox identified by the default alias.
				 *
				 * \param rTask				A reference to the task object to send.
				 *
				 * \exception				If this method failed, it throws one of the exceptions described below:
				 *							
				 *							[BadAllocException]			if there is not enough memory to build the default alias.
				 *
				 *							[MsgException]				if an internal exception occurs.
				 *
				 */
				void sendTask(const Task& rTask) 
				throw(BadAllocException, MsgException);
			
				/*! \brief Process::sendTask() - Sends the given task in the mailbox identified by the default alias
				 * (waiting at most given time).
				 *
				 * \param rTask				A reference to the task object to send.
				 * \param timeout			The timeout value.
				 *
				 * \exception				If this method failed, it throws one of the exceptions described below:
				 *
				 *							[BadAllocException]			if there is not enough memory to build the default alias.
				 *
				 *							[InvalidArgumentException]	if the timeout value is negative and different than -1.0.
				 *
				 *							[MsgException]				if an internal exception occurs.
				 *
				 * \remark					set the timeout value with -1.0 to disable it.
				 */
				void sendTask(const Task& rTask, double timeout) 
				throw(BadAllocException, InvalidArgumentException, MsgException);
			
				/*! \brief Process::receiveTask() - Retrieves a task from the mailbox identified by the alias specified as
				 * parameter.
				 *
				 * \param alias				The alias of the mailbox where to retrieve the task.
				 *
				 * \return					If succcessful, the method returns a task of the mailbox. Otherwise, the method
				 *							throws one of the exceptions described below:
				 *
				 * \exception				[NullPointerException]	if the parameter alias is NULL.
				 *
				 *							[MsgException]			if an internal exception occurs.
				 */
				Task& receiveTask(const char* alias) 
				throw(NullPointerException, MsgException);
			
				/*! \brief Process::receiveTask() - Retrieves a task from the mailbox identified by the default alias.
				 *
				 * \return					If succcessful, the method returns a task of the mailbox. Otherwise, the method
				 *							throws one of the exceptions described below:
				 *
				 * \exception				[BadAllocException]		if there is not enough memory to build the alias.			
				 *							[MsgException]			if an internal exception occurs.
				 */
				Task& receiveTask(void) 
				throw(BadAllocException, MsgException);
			
				/*! \brief Process::receiveTask() - Retrieves a task from the mailbox identified by the alias specified as
				 * parameter(waiting at most given time).
				 *
				 * \param alias				The alias of the mailbox where to retrieve the task.
				 * \param timeout 			The timeout value.
				 *
				 * \return					If succcessful, the method returns a task of the mailbox. Otherwise, the method
				 *							throws one of the exceptions described below.
				 *
				 * \exception				[InvalidArgumentException]	if the timeout value is negative or different of -1.0.
				 *
				 *							[NullPointerException]		if the parameter alias is NULL.
				 *
				 *							[MsgException]				if an internal exception occurs.
				 */
				Task& receiveTask(const char* alias, double timeout) 
				throw(NullPointerException, InvalidArgumentException, MsgException);
			
				/*! \brief Process::receiveTask() - Retrieves a task from the mailbox identified by the default alias
				 * (waiting at most given time).
				 *
				 * \param timeout			The timeout value.
				 *
				 * \return					If succcessful, the method returns a task of the mailbox. Otherwise, the method
				 *							throws one of the exceptions described below:
				 *
				 * \exception				[InvalidArgumentException]	if the timeout value is negative or different than -1.0.
				 *
				 *							[BadAllocException]			if there is not enough memory to build the alias.
				 *			
				 *							[MsgException]				if an internal exception occurs.
				 */
				Task& receiveTask(double timeout) 
				throw(InvalidArgumentException, BadAllocException, MsgException);
			
				/*! \brief Process::receiveTask() - Retrieves a task from the mailbox identified by the alias specified as
				 * parameter located on a given host (waiting at most given time).
				 *
				 * \param alias				The alias of the mailbox where to retrieve the task.
				 * \param timeout 			The timeout value.
				 * \param rHost				A reference to the host object owning the mailbox.
				 *
				 * \return					If succcessful, the method returns a task of the mailbox. Otherwise, the method
				 *							throws one of the exceptions described below:
				 *
				 * \exception				[InvalidArgumentException]	if the timeout value is negative or different of -1.0.
				 *
				 *							[NullPointerException]		if the parameter alias is NULL.
				 *
				 *							[MsgException]				if an internal exception occurs.
				 */
				Task& receiveTask(const char* alias, double timeout, const Host& rHost) 
				throw(NullPointerException, InvalidArgumentException, MsgException);
			
				/*! \brief Process::receiveTask() - Retrieves a task from the mailbox identified by default alias
				 *  and located on a given host (waiting at most given time).
				 *
				 * \param timeout 			The timeout value.
				 * \param rHost				A reference to the host object owning the mailbox.
				 *
				 * \return					If succcessful, the method returns a task of the mailbox. Otherwise, the method
				 *							throws one of the exceptions described below:
				 *
				 * \exception				[InvalidArgumentException]	if the timeout value is negative and different than -1.0.
				 *
				 *							[BadAllocException]			if there is not enough memory to build the default alias.
				 *
				 *							[MsgException]				if an internal exception occurs.
				 */
				Task& Process::receiveTask(double timeout, const Host& rHost) 
				throw(BadAllocException, InvalidArgumentException, MsgException);
			
				/*! \brief Process::receiveTask() - Retrieves a task from the mailbox identified by the alias
				 *  specified as parameter and located on a given host.
				 *
				 * \param alias 		The alias using to identify the mailbox from which to get the task.
				 * \param rHost			A reference to the host object owning the mailbox.
				 *
				 * \return				If succcessful, the method returns a task of the mailbox. Otherwise, the method
				 *						throws one of the exceptions described below:
				 *
				 * \exception			[NullPointerException]			if the parameter alias is NULL.
				 *
				 *						[MsgException]					if an internal exception occurs.
				 */
				Task& receiveTask(const char* alias, const Host& rHost) 
				throw(NullPointerException, MsgException);
			
				/*! \brief Process::receiveTask() - Retrieves a task from the mailbox identified by default alias
				 *  and located on a given host.
				 *
				 * \param rHost			A reference to the host object owning the mailbox.
				 *
				 * \return				If succcessful, the method returns a task of the mailbox. Otherwise, the method
				 *						throws one of the exceptions described below:
				 *
				 * \exception			[BadAllocException]				if there is not enough memory to build the alias.
				 *
				 *						[MsgException]					if an internal exception occurs.
				 */
				Task& receiveTask(const Host& rHost) 
				throw(BadAllocException, MsgException);
			
				
			private:
			
				/* Process::create() - Creates a process on a given host.
				 *
				 * param rHost			A reference to a host object where to create the process.
				 * param name			The name of the process to create.
				 * param argc			The number of argument to pass to the main function of the process.
				 * param argv			The list of the arguments of the main function of the process.
				 *
				 * exception			If this method failed, it throws one of the exceptions described below:
				 *
				 *						[HostNotFoundException]	if the host specified as parameter doesn't exist.
				 */
				void create(const Host& rHost, const char* name, int argc, char** argv) 
				throw(HostNotFoundException);
				
				/* Process::fromNativeProcess() - Retrieves the process wrapper associated with a native process.
				 *
				 * \param nativeProcess	The native process to get the wrapper.
				 *
				 * \return				The wrapper associated with the native process specified as parameter.
				 */
				static Process& fromNativeProcess(m_process_t nativeProcess);
			
			
			public:
			
				/* Process::run() - used to set the field code of the context of the process.
				 */ 
				static int run(int argc, char** argv);
				
				/*! \brief Process::main() - This virtual pure function is the main function of the process.
				 * You must override this function for each new class of process that you create.
				 *
				 * \param argc			The number of parameters of the main function.
				 * \param argv			The list of the parameter of the main function.
				 *
				 * \return				The exit code of the main function.
				 */
				virtual int main(int argc, char** argv) = 0;
				
			private:
				
				// Attributes.
				
				m_process_t nativeProcess;	// pointer to the native msg process.
			
		};
	
	} //namepace Msg

} namespace SimGrid