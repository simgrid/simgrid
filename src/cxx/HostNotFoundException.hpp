/*
 * HostNotFoundException.hpp
 *
 * Copyright 2006,2007 Martin Quinson, Malek Cherier           
 * All right reserved. 
 *
 * This program is free software; you can redistribute 
 * it and/or modify it under the terms of the license 
 *(GNU LGPL) which comes with this package. 
 *
 */  
 
#ifndef MSG_HOSTNOTFOUNDEXCEPTION_HPP
#define MSG_HOSTNOTFOUNDEXCEPTION_HPP

#include "Exception.hpp"

namespace SimGrid
{
	namespace Msg
	{
		
		class HostNotFoundException : public Exception
		{
			public:
			
			// Default constructor.
				HostNotFoundException();
			
			// Copy constructor.
				HostNotFoundException(const HostNotFoundException& rHostNotFoundException);
			
			// This constructor takes the name of the host not found.
				HostNotFoundException(const char* name);
			
			// Destructor.
				virtual ~HostNotFoundException();
				
			// Operations.
					
					// Returns the reason of the exception :
					// the message "Host not found `host name'"
					const char* toString(void) const;
			
			// Operators.
				
				// Assignement.
				const HostNotFoundException& operator = (const HostNotFoundException& rHostNotFoundException);
				
			private :
			
			// Attributes.
				
				// A buffer used to build the message returned by the methode toString().
				char* reason;
		};
		
		
	} // namespace Msg	

}// namespace SimGrid


#endif // !MSG_HOSTNOTFOUNDEXCEPTION_HPP
