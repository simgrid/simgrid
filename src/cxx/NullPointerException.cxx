/*
 * NullPointerException.cxx
 *
 * Copyright 2006,2007 Martin Quinson, Malek Cherier           
 * All right reserved. 
 *
 * This program is free software; you can redistribute 
 * it and/or modify it under the terms of the license 
 *(GNU LGPL) which comes with this package. 
 *
 */
 
 /* NullPointerException member functions implementation.
  */  

#include <NullPointerException.hpp>

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

namespace SimGrid
{
	namespace Msg
	{
		
			NullPointerException::NullPointerException()
			{
				this->reason = (char*) calloc(strlen("Null pointer : unknown") + 1, sizeof(char));
				strcpy(this->reason, "Null pointer : unknown");
			}
		
		
			NullPointerException::NullPointerException(const NullPointerException& rNullPointerException)
			{
				const char* reason = rNullPointerException.toString();
				this->reason = (char*) calloc(strlen(reason) + 1, sizeof(char));
				strcpy(this->reason, reason);
				
			}
		
		
			NullPointerException::NullPointerException(const char* name)
			{
				this->reason = (char*) calloc(strlen("Null pointer : ") + strlen(name) + 1, sizeof(char));
				sprintf(this->reason, "Null pointer : %s", name);
			}
		
		
			NullPointerException::~NullPointerException()
			{
				if(this->reason)
					free(this->reason);
			}
				
			const char* NullPointerException::toString(void) const
			{
				return (const char*)(this->reason);	
			}
		
		
			const NullPointerException& NullPointerException::operator = (const NullPointerException& rNullPointerException)
			{
				const char* reason = rNullPointerException.toString();
				this->reason = (char*) calloc(strlen(reason) + 1, sizeof(char));
				
				return *this;
			}
		
	} // namespace Msg	

}// namespace SimGrid



