/*
 * InvalidArgumentException.hpp
 *
 * Copyright 2006,2007 Martin Quinson, Malek Cherier           
 * All right reserved. 
 *
 * This program is free software; you can redistribute 
 * it and/or modify it under the terms of the license 
 *(GNU LGPL) which comes with this package. 
 *
 */  
 
#ifndef MSG_INVALIDARGUMENTEXCEPTION_HPP
#define MSG_INVALIDARGUMENTEXCEPTION_HPP

#include "Exception.hpp"

namespace SimGrid
{
	namespace Msg
	{
		
		class InvalidArgumentException : public Exception
		{
			public:
			
			// Default constructor.
				InvalidArgumentException();
			
			// Copy constructor.
				InvalidArgumentException(const InvalidArgumentException& rInvalidArgumentException);
			
			// This constructor takes the name of the invalid argument.
				InvalidArgumentException(const char* name);
			
			// Destructor.
				virtual ~InvalidArgumentException();
				
			// Operations.
					
					// Returns the reason of the exception :
					// the message "Invalid argument `argument name'"
					const char* toString(void) const;
			
			// Operators.
				
				// Assignement.
				const InvalidArgumentException& operator = (const InvalidArgumentException& rInvalidArgumentException);
				
			private :
			
			// Attributes.
				
				// A buffer used to build the message returned by the methode toString().
				char* reason;
		};
		
		
	} // namespace Msg	

}// namespace SimGrid


#endif // !MSG_INVALIDARGUMENTEXCEPTION_HPP
