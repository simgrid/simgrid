/*
 * NullPointerException.hpp
 *
 * Copyright 2006,2007 Martin Quinson, Malek Cherier           
 * All right reserved. 
 *
 * This program is free software; you can redistribute 
 * it and/or modify it under the terms of the license 
 *(GNU LGPL) which comes with this package. 
 *
 */  
 
#ifndef MSG_NULLPOINTEREXCEPTION_HPP
#define MSG_NULLPOINTEREXCEPTION_HPP

#ifndef __cplusplus
	#error NullPointerException.hpp requires C++ compilation (use a .cxx suffix)
#endif

#include <Exception.hpp>

namespace SimGrid
{
	namespace Msg
	{
		
		class SIMGRIDX_EXPORT NullPointerException : public Exception
		{
			public:
			
			// Default constructor.
				NullPointerException();
			
			// Copy constructor.
				NullPointerException(const NullPointerException& rNullPointerException);
			
			// This constructor takes the name of the invalid argument.
				NullPointerException(const char* name);
			
			// Destructor.
				virtual ~NullPointerException();
				
			// Operations.
					
					// Returns the reason of the exception :
					// the message "Null pointer `pointer name'"
					const char* toString(void) const;
			
			// Operators.
				
				// Assignement.
				const NullPointerException& operator = (const NullPointerException& rNullPointerException);
				
			private :
			
			// Attributes.
				
				// A buffer used to build the message returned by the methode toString().
				char* reason;
		};
		
		
	} // namespace Msg	

}// namespace SimGrid


#endif // !MSG_NULLPOINTEREXCEPTION_HPP

