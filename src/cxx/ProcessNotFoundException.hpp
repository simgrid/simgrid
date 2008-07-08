/*
 * ProcessNotFoundException.hpp
 *
 * Copyright 2006,2007 Martin Quinson, Malek Cherier           
 * All right reserved. 
 *
 * This program is free software; you can redistribute 
 * it and/or modify it under the terms of the license 
 *(GNU LGPL) which comes with this package. 
 *
 */  
 
#ifndef MSG_PROCESSNOTFOUNDEXCEPTION_HPP
#define MSG_PROCESSNOTFOUNDEXCEPTION_HPP

#ifndef __cplusplus
	#error ProcessNotFoundException.hpp requires C++ compilation (use a .cxx suffix)
#endif

#include <Exception.hpp>

namespace SimGrid
{
	namespace Msg
	{
		
		class SIMGRIDX_EXPORT ProcessNotFoundException : public Exception
		{
			public:
			
			// Default constructor.
				ProcessNotFoundException();
			
			// Copy constructor.
				ProcessNotFoundException(const ProcessNotFoundException& rProcessNotFoundException);
			
			// This constructor takes PID of the process.
				ProcessNotFoundException(int PID);
			
			// Destructor.
				virtual ~ProcessNotFoundException();
				
			// Operations.
					
					// Returns the reason of the exception :
					// the message "Process not found `PID'"
					const char* toString(void) const;
			
			// Operators.
				
				// Assignement.
				const ProcessNotFoundException& operator = (const ProcessNotFoundException& rProcessNotFoundException);
				
			private :
			
			// Attributes.
				
				// A buffer used to build the message returned by the methode toString().
				char* reason;
		};
		
		
	} // namespace Msg	

}// namespace SimGrid


#endif // !MSG_PROCESSNOTFOUNDEXCEPTION_HPP
