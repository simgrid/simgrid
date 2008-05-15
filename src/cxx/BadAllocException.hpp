/*
 * BadAllocException.hpp
 *
 * Copyright 2006,2007 Martin Quinson, Malek Cherier           
 * All right reserved. 
 *
 * This program is free software; you can redistribute 
 * it and/or modify it under the terms of the license 
 *(GNU LGPL) which comes with this package. 
 *
 */  
 
#ifndef MSG_MSGEXCEPTION_HPP
#define MSG_MSGEXCEPTION_HPP

#include "Exception.hpp"

namespace SimGrid
{
	namespace Msg
	{
		
		class BadAllocException : public Exception
		{
			public:
			
			// Default constructor.
				BadAllocException();
			
			// Copy constructor.
				BadAllocException(const BadAllocException& rBadAllocException);
			
			// This constructor takes the name of the object of the allocation failure.
				BadAllocException(const char* objectName);
			
			// Destructor.
				virtual ~BadAllocException();
				
			// Operations.
					
					// Returns the reason of the exception :
					// the message "Not enough memory to allocate : `object name'"
					const char* toString(void) const;
			
			// Operators.
				
				// Assignement.
				const BadAllocException& operator = (const BadAllocException& rBadAllocException);
				
			private :
			
			// Attributes.
				
				// A buffer used to build the message returned by the methode toString().
				char* reason;
		};
		
		
	} // namespace Msg	

}// namespace SimGrid


#endif // !MSG_MSGEXCEPTION_HPP
