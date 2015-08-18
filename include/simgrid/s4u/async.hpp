/* Copyright (c) 2006-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_S4U_ASYNC_HPP
#define SIMGRID_S4U_ASYNC_HPP

#include <stdlib.h>
#include <xbt/misc.h>

SG_BEGIN_DECL();
typedef enum {
	inited, started, finished
} e_s4u_async_state_t;
SG_END_DECL();

namespace simgrid {
namespace s4u {

/* Forward declaration */
class Comm;


/** @brief Asynchronous Actions
 *
 * This class is the ancestor of every asynchronous actions, that is, of actions that do take time in the simulated world.
 */
class Async {
	friend Comm;
protected:
	Async();
	virtual ~Async();
	
private:
	struct s_smx_synchro *p_inferior = NULL;

private:
	e_s4u_async_state_t p_state = inited;
public:
	/** Starts a previously created async.
	 *
	 * This function is optional: you can call wait() even if you didn't call start()
	 */
	virtual void start()=0;
	/** Tests whether the given async is terminated yet */
	//virtual bool test()=0;
	/** Blocks until the async is terminated */
	virtual void wait()=0;
	/** Blocks until the async is terminated, or until the timeout is elapsed
	 *  Raises: timeout exception.*/
	virtual void wait(double timeout)=0;
	/** Cancel that async */
	//virtual void cancel();
	/** Retrieve the current state of the async */
	e_s4u_async_state_t getState() {return p_state;}

private:
	double p_remains = 0;
public:
	/** Get the remaining amount of work that this Async entails. When it's 0, it's done. */
	double getRemains();
	/** Set the [remaining] amount of work that this Async will entail
	 *
	 * It is forbidden to change the amount of work once the Async is started */
	void setRemains(double remains);

private:
	void *p_userData = NULL;
public:
	/** Put some user data onto the Async */
	void setUserData(void *data) {p_userData=data;}
	/** Retrieve the user data of the Async */
	void *getUserData() { return p_userData; }
}; // class

}}; // Namespace simgrid::s4u

#endif /* SIMGRID_S4U_ASYNC_HPP */
