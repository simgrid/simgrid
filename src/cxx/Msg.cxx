
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

#include <msg/msg.h>
#include <msg/private.h>
#include <stdio.h>




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
		
		void info(const char* s)
		{
			 //INFO1("%s",s);
			printf("[SimGridX/info] %s\n", s);
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

