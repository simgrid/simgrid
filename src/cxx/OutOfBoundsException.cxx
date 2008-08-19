/*
 * OutOfBoundsException.cxx
 *
 * Copyright 2006,2007 Martin Quinson, Malek Cherier           
 * All right reserved. 
 *
 * This program is free software; you can redistribute 
 * it and/or modify it under the terms of the license 
 *(GNU LGPL) which comes with this package. 
 *
 */
 
 /* OutOfBoundsException member functions implementation.
  */  

#include <OutOfBoundsException.hpp>

#include <string.h>
#include <stdlib.h>
#include <stdio.h>


namespace SimGrid
{
	namespace Msg
	{
		
			OutOfBoundsException::OutOfBoundsException()
			{
				this->reason = (char*) calloc(strlen("Out of bounds") + 1, sizeof(char));
				strcpy(this->reason, "Out of bounds");
			}
		
		
			OutOfBoundsException::OutOfBoundsException(const OutOfBoundsException& rOutOfBoundsException)
			{
				const char* reason = rOutOfBoundsException.toString();
				this->reason = (char*) calloc(strlen(reason) + 1, sizeof(char));
				strcpy(this->reason, reason);
				
			}
		
			OutOfBoundsException::OutOfBoundsException(int pos)
			{
				this->reason = (char*) calloc(strlen("Out of bounds ") + 21 + 1, sizeof(char));
				sprintf(this->reason, "Out of bounds %d", pos);
			}

			OutOfBoundsException::OutOfBoundsException(int pos1, int pos2)
			{
				this->reason = (char*) calloc(strlen("Out of bounds ") + (2*21) + 1, sizeof(char));
				sprintf(this->reason, "Out of bounds %d : %d", pos1, pos2);
			}
		
		
			OutOfBoundsException::~OutOfBoundsException()
			{
				if(this->reason)
					free(this->reason);
			}
				
			const char* OutOfBoundsException::toString(void) const
			{
				return (const char*)(this->reason);	
			}
		
		
			const OutOfBoundsException& OutOfBoundsException::operator = (const OutOfBoundsException& rOutOfBoundsException)
			{
				const char* reason = rOutOfBoundsException.toString();
				this->reason = (char*) calloc(strlen(reason) + 1, sizeof(char));
				
				return *this;
			}
		
	} // namespace Msg	

}// namespace SimGrid



