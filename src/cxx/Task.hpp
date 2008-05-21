/*
 * Task.hpp
 *
 * This file contains the declaration of the wrapper class of the native MSG task type.
 *
 * Copyright 2006,2007 Martin Quinson, Malek Cherier           
 * All right reserved. 
 *
 * This program is free software; you can redistribute 
 * it and/or modify it under the terms of the license 
 *(GNU LGPL) which comes with this package. 
 *
 */  
 
#ifndef MSG_TASK_HPP
#define MSG_TASK_HPP

// Compilation C++ recquise
#ifndef __cplusplus
	#error Process.hpp requires C++ compilation (use a .cxx suffix)
#endif

namespace SimGrid
{
	namespace Msg
	{
		// SimGrid::Msg::Task wrapper class declaration.
		class Task
		{
			protected:
				// Default constructor.
				Task();
				
			public:
				/*! \brief Copy constructor.
				 */
				Task(const Task& rTask);
				
				/*! \brief Destructor.
				 *
				 * \exception				If the destructor failed, it throw the exception described below:
				 *
				 *							[MsgException]				if a native exception occurs.
				 */
				virtual ~Task()
				throw(MsgException);
				
				/*! \brief Constructs an new task with the specified processing amount and amount
	 			 * of data needed.
				 *
				 * \param name				Task's name
				 * \param computeDuration	A value of the processing amount (in flop) needed to process the task. 
				 *							If 0, then it cannot be executed with the execute() method.
				 *							This value has to be >= 0.
				 * \param messageSize		A value of amount of data (in bytes) needed to transfert this task.
				 *							If 0, then it cannot be transfered with the get() and put() methods.
				 *							This value has to be >= 0.
				 *
				 * \exception				If this method failed, it throws one of the exceptions described below:
				 *
				 *							[InvalidArgumentException]	if the parameter computeDuration or messageSize
				 *														is negative.
				 *							[NullPointerException]		if the parameter name is NULL.
				 *
				 *							[MsgException]				if a internal exception occurs.
				 */
				Task(const char* name, double computeDuration, double messageSize)
				throw (InvalidArgumentException, NullPointerException);
				
				/*! \Constructs an new parallel task with the specified processing amount and amount for each host
			     * implied.
			     *
			     * \param name				The name of the parallel task.
			     * \param hosts				The list of hosts implied by the parallel task.
			     * \param computeDurations	The amount of operations to be performed by each host of \a hosts.
			     * \param messageSizes		A matrix describing the amount of data to exchange between hosts.
			     * \hostCount				The number of hosts implied in the parallel task.
			     * 
			     * \exception				If this method fails, it throws one of the exceptions described below:
			     *
			     *							[NullPointerException]		if the parameter name is NULL or
			     *														if the parameter computeDurations is NULL or
			     *														if the parameter messageSizes is NULL
			     *
			     *							[InvalidArgumentException]	if the parameter hostCount is negative or zero.							
			     */
				Task(const char* name, Host* hosts, double* computeDurations, double* messageSizes, int hostCount)
				throw(NullPointerException, InvalidArgumentException);
			
			
				/*! \brief Task::getName() - Gets the names of the task.
				 *
				 * \return 					The name of the task.
				 */
				const char* getName(void) const;
			
				/*! \brief Task::getSender() - Gets the sender of the task.
				 *
				 * \return					A reference to the sender of the task.
				 */
				Process& getSender(void) const;
				
				/*! \brief Task::getSource() - Gets the source of the task.
				 * 
				 * \return 					A reference to the source of the task.
				 */
				Host& getSource(void) const;
				
				/*! \brief Task::getComputeDuration() - Get the computing amount of the task.
				 *
				 * \return					The computing amount of the task.
				 */
			
				double getComputeDuration(void) const;
				
				/*! \brief Task::getRemainingDuration() - Gets the remaining computation.
				 *
				 * \return					The remining computation of the task.
				 */
				double getRemainingDuration(void) const;
			
				/*! \brief Task::setPrirority() -  Sets the priority of the computation of the task.
    			 * The priority doesn't affect the transfert rate. For example a
    			 * priority of 2 will make the task receive two times more cpu than
    			 * the other ones.
    			 *
    			 * \param priority			The new priority of the task.
    			 *
    			 * execption				If this method fails, it throws the exception described below:
    			 *
    			 *							[InvalidArgumentException]	if the parameter priority is negative.
    			 */
				void setPriority(double priority)
				throw(InvalidArgumentException);
				
				/*! \brief Task::get() - Gets a task from the specified channel of the host of the current process.
				 *
				 * \param channel			The channel number to get the task.
				 *
				 * \return					If successful the method returns the task. Otherwise the method throws one
				 *							of the exceptions described below:
				 *
				 * \exception				[InvalidArgumentException]	if the channel parameter is negative.
				 *					
				 *							[MsgException]				if an internal excpetion occurs.
				 */
				static Task& get(int channel) 
				throw(InvalidArgumentException, MsgException); 
				
				/*! \brief	Task::get() - Gets a task from the given channel number of the given host.	
				 *
				 * \param channel			The channel number.
				 * \param rHost				A reference of the host owning the channel.
				 *
				 * \return					If successful, the method returns the task. Otherwise the method
				 *							throw one of the exceptions described below:
				 *
				 * \exception				[InvalidArgumentException]	if the channel number is negative.
				 *
				 *							[MsgException]				if an internal exception occurs.
				 */
				static Task& get(int channel, const Host& rHost) 
				throw(InvalidArgumentException, MsgException);
				
				/*! \brief Task::get() - Gets a task from the specified channel of the specified host
				 *  (waiting at most given time).
				 *
				 * \param channel			The channel number.
				 * \param timeout			The timeout value.
				 * \param rHost				A reference to the host owning the channel.
				 *
				 * \return					If successful, the method returns the task. Otherwise the method returns
				 *							one of the exceptions described below:
				 *
				 * \exception				[InvalidArgumentException]	if the channel number is negative or
				 *														if the timeout value is negative and 
				 *														different than -1.0.
				 *							[MsgException]				if an internal exception occurs.
				 */
				static Task& get(int channel, double timeout, const Host& rHost) 
				throw(InvalidArgumentException, MsgException);  
				
				/*! \brief Task::probe() - Probes whether there is a waiting task on the given channel of local host.
				 *
				 * \param channel			The channel number.
				 *
				 * \return 					If there is a waiting task on the channel the method returns true. Otherwise
				 *							the method returns false.
				 *
				 * \exception				If this method fails, it throws the exception described below:
				 *
				 *							[InvalidArgumentException]	if the parameter channel is negative.
				 */
				static bool probe(int channel)
				throw(InvalidArgumentException);
				
				/*! \brief Task::probe() - Counts tasks waiting on the given channel of local host and sent by given host.
				 *
				 * \param channel			The channel id.
				 * \param rHost				A reference to the host that has sent the task.
				 *
				 * \return					The number of tasks.
				 *
				 * \exception				[InvalidArgumentException]	if the parameter channel is negative.
				 */
				static int probe(int channel, const Host& rHost)
				throw(InvalidArgumentException);
				
				/*! \brief Task::execute() - This method executes a task on the location on which the
   				 * process is running.
   				 *
   				 * \exception				If this method fails, it returns the exception described below:
   				 *
   				 *							[MsgException]				if an internal exception occurs.
   				 */
				void execute(void) 
				throw(MsgException);
				
				/*! \brief Task::cancel() - This method cancels a task.
   				 *
   				 * \exception				If this method fails, it returns the exception described below:
   				 *
   				 *							[MsgException]				if an internal exception occurs.
   				 */
				void cancel(void) 
				throw(MsgException);
			
				/*! \brief Task::send() - Send the task on the mailbox identified by the default alias.
				 *
				 * \exception				If this method failed, it returns one of the exceptions described
				 *							below:
				 *
				 *							[BadAllocException]			if there is not enough memory to build the default alias.
				 *
				 *							[MsgException]				if an internal exception occurs.
				 */
				void send(void) 
				throw(BadAllocException, MsgException);
				
				/*! \brief Task::send() - Send the task on the mailbox identified by the given alias.
				 *
				 * \param alias				The alias of the mailbox where to send the task.
				 *
				 * \exception				If this method failed, it returns one of the exceptions described
				 *							below:
				 *
				 *							[NullPointerException]		if there parameter alias is NULL.
				 *
				 *							[MsgException]				if an internal exception occurs.
				 */ 
				void send(const char* alias) 
				throw(NullPointerException, MsgException);
				
				/*! \brief Task::send() - Send the task on the mailbox identified by the default alias
				 * (waiting at most given time).
				 *
				 * \param timeout			The timeout value.
				 *
				 * \exception				If this method failed, it returns one of the exceptions described
				 *							below:
				 *
				 *							[BadAllocException]			if there is not enough memory to build the default alias.
				 *
				 *							[InvalidArgumentException]	if the timeout value is negative or different -1.0.
				 *
				 *							[MsgException]				if an internal exception occurs.
				 */
				void send(double timeout) 
				throw(BadAllocationException, InvalidArgumentException, MsgException);
				
				/*! \brief Task::send() - Send the task on the mailbox identified by a given alias
				 * (waiting at most given time).
				 *
				 * \param alias				The alias of the mailbox where to send the task.
				 * \param timeout			The timeout value.
				 *
				 * \exception				If this method failed, it returns one of the exceptions described
				 *							below:
				 *
				 *							[NullPointerException]		if alias parameter is NULL.
				 *
				 *							[InvalidArgumentException]	if the timeout value is negative or different -1.0.
				 *
				 *							[MsgException]				if an internal exception occurs.
				 */
				void send(const char* alias, double timeout) 
				throw(NullPointerException, InvalidArgumentException, MsgException);
				
				/*! \brief Task::sendBounded() - Sends the task on the mailbox identified by the default alias.
				 * (capping the emision rate to maxrate).
				 *
				 * \param maxRate			The maximum rate value.
				 *
				 * \exception				If this method failed, it throws one of the exceptions described below:
				 *
				 *							[BadAllocException]			if there is not enough memory to build the default alias.
				 *
				 *							[InvalidArgumentException]	if the parameter maxRate is negative and different
				 *														than -1.0.
				 *
				 *							[MsgException]				if an internal exception occurs.
				 */
				void sendBounded(double maxRate) 
				throw(BadAllocException, InvalidArgumentException, MsgException);
				
				/*! \brief Task::sendBounded() - Sends the task on the mailbox identified by the given alias.
				 * (capping the emision rate to maxrate).
				 *
				 *\ param alias				The alias of the mailbox where to send the task.
				 * \param maxRate			The maximum rate value.
				 *
				 * \exception				If this method failed, it throws one of the exceptions described below:
				 *
				 *							[NullPointerException]		if the alias parameter is NULL.
				 *
				 *							[InvalidArgumentException]	if the parameter maxRate is negative and different
				 *														than -1.0.
				 *
				 *							[MsgException]				if an internal exception occurs.
				 */
				void sendBounded(const char* alias, double maxRate) 
				throw(NullPointerException, InvalidArgumentException, MsgException);
				
				
				/*! \brief Task::receive() - Receives a task from the mailbox identified by the default alias (located
				 * on the local host).
				 *
				 * \return					A reference to the task.
				 *
				 * \exception				If this method failed, it throws one of the exception described below:
				 *
				 *							[BadAllocException]			if there is not enough memory to build the default alias.
				 *
				 *							[MsgException]				if an internal exception occurs.
				 */
				static Task& receive(void) 
				throw(BadAllocException, MsgException);
				
				/*! \brief Task::receive() - Receives a task from the mailbox identified by a given alias (located
				 * on the local host).
				 *
				 * \alias					The alias of the mailbox.
				 *
				 * \return					A reference to the task.
				 *
				 * \exception				If this method failed, it throws one of the exception described below:
				 *
				 *							[NullPointerException]		if the alias parameter is NULL.
				 *
				 *							[MsgException]				if an internal exception occurs.
				 */
				static Task& receive(const char* alias) 
				throw(NullPointerException, MsgException);
				
				/*! \brief Task::receive() - Receives a task from the mailbox identified by a given alias (located
				 * on the local host and waiting at most given time).
				 *
				 * \alias					The alias of the mailbox.
				 * \timeout					The timeout value.
				 *
				 * \return					A reference to the task.
				 *
				 * \exception				If this method failed, it throws one of the exception described below:
				 *
				 *							[NullPointerException]		if the alias parameter is NULL.
				 *
				 *							[InvalidArgumentException]	if the timeout value is negatif and different than
				 *														-1.0.
				 *
				 *							[MsgException]				if an internal exception occurs.
				 */
				static Task& receive(const char* alias, double timeout) 
				throw(NullPointerException, InvalidArgumentException, MsgException);
				
				/*! \brief Task::receive() - Receives a task from the mailbox identified by a given alias located
				 * on the given host.
				 *
				 * \alias					The alias of the mailbox.
				 * \rHost					The location of the mailbox.
				 *
				 * \return					A reference to the task.
				 *
				 * \exception				If this method failed, it throws one of the exception described below:
				 *
				 *							[NullPointerException]		if the alias parameter is NULL.
				 *
				 *							[MsgException]				if an internal exception occurs.
				 */
				static Task& receive(const char* alias, const Host& rHost) 
				throw(NullPointerException, MsgException);
				
				/*! \brief Task::receive() - Receives a task from the mailbox identified by a given alias located
				 * on the given host (and waiting at most given time).
				 *
				 * \alias					The alias of the mailbox.
				 * \timeout					The timeout value.
				 * \rHost					The location of the mailbox.
				 *
				 * \return					A reference to the task.
				 *
				 * \exception				If this method failed, it throws one of the exception described below:
				 *
				 *							[NullPointerException]		if the alias parameter is NULL.
				 *
				 *							[InvalidArgumentException]	if the timeout value is negatif and different than
				 *														-1.0.
				 *
				 *							[MsgException]				if an internal exception occurs.
				 */
				static Task& receive(const char* alias, double timeout, const Host& rHost) 
				throw(NullPointerException, InvalidArgumentException, MsgException);
				
				/*! \brief Task::listen() - Listen whether there is a waiting task on the mailbox 
				 * identified by the default alias of local host.
				 *
				 * \return					If there is a waiting task on the mailbox the method returns true.
				 *							Otherwise the method returns false.
				 *
				 * \exception				If this method fails, it throws one of the exceptions described below:
				 *
				 *							[BadAllocException]			if there is not enough memory to build the default alias.
				 */
				static bool listen(void) 
				throw(BadAllocException);
				
				/*! \brief Task::listen() - Listen whether there is a waiting task on the mailbox 
				 * identified by the given alias of local host.
				 *
				 * \param alias				The alias of the mailbox.
				 *
				 * \return					If there is a waiting task on the mailbox the method returns true.
				 *							Otherwise the method returns false.
				 *
				 * \exception				If this method fails, it throws one of the exceptions described below:
				 *
				 *							[NullPointerException]		if the parameter alias is NULL.
				 */
				static bool listen(const char* alias) 
				throw(NullPointerException);
				
				/*! \brief Task::listenFrom() - Tests whether there is a pending communication on the mailbox 
				 * identified by the default alias, and who sent it.
				 *
				 * \return					If there is a pending communication on the mailbox, the method returns
				 *							the PID of it sender. Otherwise the method returns -1.
				 *
				 * \exception				If this method fails, it throws the exception described below:
				 *
				 *							[BadAllocException]			if there is not enough memory to build the default
				 *														alias.
				 */
				static int listenFrom(void) 
				throw(BadAllocException);
				
				/*! \brief Task::listenFrom() - Tests whether there is a pending communication on the mailbox 
				 * identified by the specified alias, and who sent it.
				 *
				 * \alias					The alias of the mailbox.
				 *
				 * \return					If there is a pending communication on the mailbox, the method returns
				 *							the PID of it sender. Otherwise the method returns -1.
				 *
				 * \exception				If this method fails, it throws the exception described below:
				 *
				 *							[NullPointerException]		if the alias parameter is NULL.
				 */
				static int listenFrom(const char* alias) 
				throw(NullPointerException);
				
				/*! \brief Task::listenFromHost() - Tests whether there is a pending communication on the mailbox 
				 * identified by the default alias and located on the host of the current process, and who sent it.
				 *
				 * \param rHost				The location of the mailbox.
				 *
				 * \return					If there is a pending communication on the mailbox, the method returns
				 *							the PID of it sender. Otherwise the method returns -1.
				 *
				 * \exception				If this method fails, it throws the exception described below:
				 *
				 *							[BadAllocException]			if there is not enough memory to build the default
				 *														alias.
				 */
				static int listenFromHost(const Host& rHost) 
				throw(BadAllocException);
				
				/*! \brief Task::listenFromHost() - Tests whether there is a pending communication on the mailbox 
				 * identified by the default alias and located on the host of the current process, and who sent it.
				 *
				 * \param rHost				The location of the mailbox.
				 *
				 * \return					If there is a pending communication on the mailbox, the method returns
				 *							the PID of it sender. Otherwise the method returns -1.
				 *
				 * \exception				If this method fails, it throws the exception described below:
				 *
				 *							[BadAllocException]			if there is not enough memory to build the default
				 *														alias.
				 */
				static bool listenFromHost(const char* alias, const Host& rHost) 
				throw(NullPointerException, NativeException);
			
			private:
				
				// Attributes.
				
					m_task_t nativeTask;	// the native MSG task.
		};
	
	} // namespace Msg 
} // namespace SimGrid

#endif // §MSG_TASK_HPP