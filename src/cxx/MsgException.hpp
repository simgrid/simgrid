/*
 * MsgException.hpp
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

#include <Exception.hpp>

namespace SimGrid
{
	namespace Msg
	{
		
		class SIMGRIDX_EXPORT MsgException : public Exception
		{
			public:
			
			// Default constructor.
				MsgException();
			
			// Copy constructor.
				MsgException(const MsgException& rMsgException);
			
			// This constructor takes the reason of the exception.
				MsgException(const char* reason);
			
			// Destructor.
				virtual ~MsgException();
				
			// Operations.
					
					// Returns the reason of the exception :
					// the message "Internal exception `reason'"
					const char* toString(void) const;
			
			// Operators.
				
				// Assignement.
				const MsgException& operator = (const MsgException& rMsgException);
				
			private :
			
			// Attributes.
				
				// A buffer used to build the message returned by the methode toString().
				char* reason;
		};
		
		
	} // namespace Msg	

}// namespace SimGrid


#endif // !MSG_MSGEXCEPTION_HPP

