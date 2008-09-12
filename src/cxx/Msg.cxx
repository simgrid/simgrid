
/*
 * Msg.cxx
 *
 * Copyright 2006,2007 Martin Quinson, Malek Cherier           
 * All right reserved. 
 *
 * This program is free software; you can redistribute 
 * it and/or modify it under the terms of the license 
 *(GNU LGPL) which comes with this package. 
 *
 */
 
 /* Msg functions implementation.
  */  


#include <Msg.hpp>
#include <MsgHost.hpp>
#include <MsgProcess.hpp>


#include <msg/msg.h>
#include <msg/private.h>

#include <iostream>
using std::cout;
using std::endl;
using std::streamsize;

#include <iomanip>
using std::setprecision;
using std::setw;



namespace SimGrid
{
	namespace Msg
	{
		#define SIMGRIDX_DEFAULT_CHANNEL_NUMBER	((int)10)
		
		void init(int argc, char** argv)
		{
			MSG_global_init(&argc,argv);

			if(getMaxChannelNumber() == 0)
				setMaxChannelNumber(SIMGRIDX_DEFAULT_CHANNEL_NUMBER);
		}	
		
		void finalize(void)
		throw (MsgException)
		{
			if(MSG_OK != MSG_clean())
				throw MsgException("MSG_clean() failed");
			
		}
		
		void info(const StringHelper& s)
		{
			StringHelper clock;

			cout << "[";
			cout << Host::currentHost().getName();
			cout << ":";
			cout << Process::currentProcess().getName();
			cout << ":(";
			cout << Process::currentProcess().getPID();
			cout << ") " ;
			cout << clock.append(getClock(), "%07lf");
			cout << "] [cxx4msg/INFO] ";
			cout << s;
			cout << endl;
		}

		void info(const char* s)
		{
			StringHelper clock;
			
			cout << "[";
			cout << Host::currentHost().getName();
			cout << ":";
			cout << Process::currentProcess().getName();
			cout << ":(";
			cout << Process::currentProcess().getPID();
			cout << ") " ;
			cout << clock.append(getClock(), "%07lf");
			cout << "] [cxx4msg/INFO] ";
			cout << s;
			cout << endl;
		}

		void error(const StringHelper& s)
		{
			StringHelper clock;

			cout << "[";
			cout << Host::currentHost().getName();
			cout << ":";
			cout << Process::currentProcess().getName();
			cout << ":(";
			cout << Process::currentProcess().getPID();
			cout << ") " ;
			cout << clock.append(getClock(), "%07lf");
			cout << "] [cxx4msg/ERROR] ";
			cout << s;
			cout << endl;
		}

		void error(const char* s)
		{
			StringHelper clock;
			
			cout << "[";
			cout << Host::currentHost().getName();
			cout << ":";
			cout << Process::currentProcess().getName();
			cout << ":(";
			cout << Process::currentProcess().getPID();
			cout << ") " ;
			cout << clock.append(getClock(), "%07lf");
			cout << "] [cxx4msg/ERROR] ";
			cout << s;
			cout << endl;
		}

		double getClock(void)
		{
			return MSG_get_clock();
		}

		void setMaxChannelNumber(int number)
		throw(InvalidArgumentException, LogicException)
		{
			if(msg_global->max_channel > 0)
				throw LogicException("Max channel number already setted");

			if(number < 0)
				throw InvalidArgumentException("number");

			msg_global->max_channel = number;
		}

		int getMaxChannelNumber(void)
		{
			return msg_global->max_channel;
		}

	} // namespace Msg

} // namespace SimGrid

