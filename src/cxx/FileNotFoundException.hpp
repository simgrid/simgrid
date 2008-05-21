/*
 * FileNotFoundException.hpp
 *
 * Copyright 2006,2007 Martin Quinson, Malek Cherier           
 * All right reserved. 
 *
 * This program is free software; you can redistribute 
 * it and/or modify it under the terms of the license 
 *(GNU LGPL) which comes with this package. 
 *
 */  
 
#ifndef MSG_FILENOTFOUND_HPP
#define MSG_FILENOTFOUND_HPP

#include <Exception.hp>

namespace SimGrid
{
	namespace Msg
	{
		
		class FileNotFoundException : public Exception
		{
			public:
			
			// Default constructor.
				FileNotFoundException();
			
			// Copy constructor.
				FileNotFoundException(const FileNotFoundException& rFileNotFoundException);
			
			// This constructor takes the name of the file.
				FileNotFoundException(const char* file);
			
			// Destructor.
				virtual ~FileNotFoundException();
				
			// Operations.
					
					// Returns the reason of the exception :
					// the message "File not found : `object name'"
					const char* toString(void) const;
			
			// Operators.
				
				// Assignement.
				const FileNotFoundException& operator = (const FileNotFoundException& rFileNotFoundException);
				
			private :
			
			// Attributes.
				
				// A buffer used to build the message returned by the methode toString().
				char* reason;
		};
		
		
	} // namespace Msg	

}// namespace SimGrid


#endif // !MSG_MSGEXCEPTION_HPP
