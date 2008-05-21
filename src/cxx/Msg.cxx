
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

namespace SimGrid
{
	namespace Msg
	{
		
		void init(int argc, char** argv)
		{
			MSG_global_init(&argc,argv);
			MSG_set_channel_number(10); // FIXME: this should not be fixed statically 
		}	
		
		void finalize(void)
		throw (MsgException)
		{
			if(MSG_OK != MSG_clean())
				throw MsgException("MSG_clean() failed");
		}
		
		void info(const char* s)
		{
			 INFO1("%s",s);
		}

	} // namespace Msg

} // namespace SimGrid