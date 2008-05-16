/*
 * LogicException.hpp
 *
 * Copyright 2006,2007 Martin Quinson, Malek Cherier           
 * All right reserved. 
 *
 * This program is free software; you can redistribute 
 * it and/or modify it under the terms of the license 
 *(GNU LGPL) which comes with this package. 
 *
 */  
 
#ifndef MSG_LOGICEXCEPTION_HPP
#define MSG_LOGICEXCEPTION_HPP

#include "Exception.hpp"

namespace SimGrid
{
	namespace Msg
	{
		
		class LogicException : public Exception
		{
			public:
			
			// Default constructor.
				LogicException();
			
			// Copy constructor.
				LogicException(const LogicException& rLogicException);
			
			// This constructor takes the detail of the logic exception.
				LogicException(const char* detail);
			
			// Destructor.
				virtual ~LogicException();
				
			// Operations.
					
					// Returns the reason of the exception :
					// the message "Logic exception `detail'"
					const char* toString(void) const;
			
			// Operators.
				
				// Assignement.
				const LogicException& operator = (const LogicException& rLogicException);
				
			private :
			
			// Attributes.
				
				// A buffer used to build the message returned by the methode toString().
				char* reason;
		};
		
		
	} // namespace Msg	

}// namespace SimGrid


#endif // !MSG_INVALIDARGUMENTEXCEPTION_HPP
