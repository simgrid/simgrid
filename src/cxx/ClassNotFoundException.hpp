/*
 * ClassNotFoundException.hpp
 *
 * Copyright 2006,2007 Martin Quinson, Malek Cherier           
 * All right reserved. 
 *
 * This program is free software; you can redistribute 
 * it and/or modify it under the terms of the license 
 *(GNU LGPL) which comes with this package. 
 *
 */  
 
#ifndef MSG_CLASSNOTFOUNDEXCEPTION_HPP
#define MSG_CLASSNOTFOUNDEXCEPTION_HPP

#include <Exception.hpp>

namespace SimGrid
{
	namespace Msg
	{
		
		class SIMGRIDX_EXPORT ClassNotFoundException : public Exception
		{
			public:
			
			// Default constructor.
				ClassNotFoundException();
			
			// Copy constructor.
				ClassNotFoundException(const ClassNotFoundException& rClassNotFoundException);
			
			// This constructor takes the name of the class not found.
				ClassNotFoundException(const char* name);
			
			// Destructor.
				virtual ~ClassNotFoundException();
				
			// Operations.
					
					// Returns the reason of the exception :
					// the message "Host not found `host name'"
					const char* toString(void) const;
			
			// Operators.
				
				// Assignement.
				const ClassNotFoundException& operator = (const ClassNotFoundException& rClassNotFoundException);
				
			private :
			
			// Attributes.
				
				// A buffer used to build the message returned by the methode toString().
				char* reason;
		};
		
		
	} // namespace Msg	

}// namespace SimGrid


#endif // !MSG_CLASSNOTFOUNDEXCEPTION_HPP
