/*
 * BadAllocationException.cxx
 *
 * Copyright 2006,2007 Martin Quinson, Malek Cherier           
 * All right reserved. 
 *
 * This program is free software; you can redistribute 
 * it and/or modify it under the terms of the license 
 *(GNU LGPL) which comes with this package. 
 *
 */
 
 /* BadAllocationException member functions implementation.
  */  

#include <BadAllocException.hpp>

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

namespace SimGrid
{
	namespace Msg
	{
		
			BadAllocException::BadAllocException()
			{
				this->reason = (char*) calloc(strlen("Not enough memory") + 1, sizeof(char));
				strcpy(this->reason, "Not enough memory");
			}
		
		
			BadAllocException::BadAllocException(const BadAllocException& rBadAllocException)
			{
				const char* reason = rBadAllocException.toString();
				this->reason = (char*) calloc(strlen(reason) + 1, sizeof(char));
				strcpy(this->reason, reason);
				
			}
		
			BadAllocException::BadAllocException(const char* objectName)
			{
				this->reason = (char*) calloc(strlen("Not enough memory to allocate ") + strlen(objectName) + 1, sizeof(char));
				sprintf(this->reason, "Not enough memory to allocate %s", objectName);
			}
		
		
			BadAllocException::~BadAllocException()
			{
				if(this->reason)
					free(this->reason);
			}
				
			const char* BadAllocException::toString(void) const
			{
				return (const char*)(this->reason);	
			}
		
		
			const BadAllocException& BadAllocException::operator = (const BadAllocException& rBadAllocException)
			{
				const char* reason = rBadAllocException.toString();
				this->reason = (char*) calloc(strlen(reason) + 1, sizeof(char));
				
				return *this;
			}
		
	} // namespace Msg	

}// namespace SimGrid



