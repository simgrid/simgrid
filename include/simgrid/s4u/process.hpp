/* Copyright (c) 2006-2015. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_S4U_PROCESS_HPP
#define SIMGRID_S4U_PROCESS_HPP

#include "simgrid/simix.h"

namespace simgrid {
namespace s4u {

class Host;

/** @brief Simulation Agent
 *
 * A process may be defined as a code executing in a location (host).
 *
 * All processes should be started from the XML deployment file (using the @link{s4u::Engine::loadDeployment()}),
 * even if you can also start new processes directly.
 * Separating the deployment in the XML from the logic in the code is a good habit as it makes your simulation easier
 * to adapt to new settings.
 *
 * The code that you define for a given process should be placed in the main method that is virtual.
 * For example, a Worker process should be declared as follows:
 *
 * \verbatim
 * #include "s4u/process.hpp"
 *
 * class Worker : simgrid::s4u::Process {
 *
 * 	  int main(int argc, char **argv) {
 *		printf("Hello s4u");
 *	  }
 * };
 * \endverbatim
 *
 */
class Process {
public:
	Process(const char*procname, s4u::Host *host, int argc, char **argv);
	Process(const char*procname, s4u::Host *host, int argc, char **argv, double killTime);
	virtual ~Process() {}

	/** The main method of your agent */
	virtual int main(int argc, char **argv)=0;

	/** The process that is currently running */
	static Process *current();
	/** Retrieves the process that have the given PID (or NULL if not existing) */
	static Process *byPid(int pid);

	/** Retrieves the name of that process */
	const char*getName();
	/** Retrieves the host on which that process is running */
	s4u::Host *getHost();
	/** Retrieves the PID of that process */
	int getPid();

	/** If set to true, the process will automatically restart when its host reboots */
	void setAutoRestart(bool autorestart);
	/** Sets the time at which that process should be killed */
	void setKillTime(double time);
	/** Retrieves the time at which that process will be killed (or -1 if not set) */
	double getKillTime();

	/** Ask kindly to all processes to die. Only the issuer will survive. */
	static void killAll();
	/** Ask the process to die.
	 *
	 * It will only notice your request when doing a simcall next time (a communication or similar).
	 * SimGrid sometimes have issues when you kill processes that are currently communicating and such. We are working on it to fix the issues.
	 */
	void kill();

	/** Block the process sleeping for that amount of seconds (may throws hostFailure) */
	void sleep(double duration);

	/** Block the process, computing the given amount of flops */
	void execute(double flop);

	/** Block the process until it gets a message from the given mailbox */
	//void* recv(const char *mailbox);

	/** Block the process until it gets a string message (to be freed after use) from the given mailbox */
	char *recvstr(const char* mailbox);

	/** Block the process until it delivers a string message (that will be copied) to the given mailbox */
	void sendstr(const char*mailbox, const char*msg);

protected:
	smx_process_t getInferior() {return p_smx_process;}
private:
	smx_process_t p_smx_process;
};
}} // namespace simgrid::s4u

#endif /* SIMGRID_S4U_PROCESS_HPP */

#if 0

public abstract class Process implements Runnable {
	/** Suspends the process. See {@link #resume()} to resume it afterward */
	public native void suspend();
	/** Resume a process that was suspended by {@link #suspend()}. */
	public native void resume();	
	/** Tests if a process is suspended. */
	public native boolean isSuspended();
	
	/**
	 * Returns the value of a given process property. 
	 */
	public native String getProperty(String name);


	/**
	 * Migrates a process to another host.
	 *
	 * @param host			The host where to migrate the process.
	 *
	 */
	public native void migrate(Host host);	

	public native void exit();    
	/**
	 * This static method returns the current amount of processes running
	 *
	 * @return			The count of the running processes
	 */ 
	public native static int getCount();

}
#endif
