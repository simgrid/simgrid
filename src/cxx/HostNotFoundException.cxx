/*
 * HostNotFoundException.cxx
 *
 * Copyright 2006,2007 Martin Quinson, Malek Cherier           
 * All right reserved. 
 *
 * This program is free software; you can redistribute 
 * it and/or modify it under the terms of the license 
 *(GNU LGPL) which comes with this package. 
 *
 */
 
 /* HostNotFoundException member functions implementation.
  */  

#include <HostNotFoundException.hpp>

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

namespace SimGrid
{
	namespace Msg
	{
		
			HostNotFoundException::HostNotFoundException()
			{
				this->reason = (char*) calloc(strlen("Host not found : unknown") + 1, sizeof(char));
				strcpy(this->reason, "Host not found : unknown");
			}
		
		
			HostNotFoundException::HostNotFoundException(const HostNotFoundException& rHostNotFoundException)
			{
				const char* reason = rHostNotFoundException.toString();
				this->reason = (char*) calloc(strlen(reason) + 1, sizeof(char));
				strcpy(this->reason, reason);
				
			}
		
		
			HostNotFoundException::HostNotFoundException(const char* name)
			{
				this->reason = (char*) calloc(strlen("Host not found : ") + strlen(name) + 1, sizeof(char));
				sprintf(this->reason, "Host not found : %s", name);
			}
		
		
			HostNotFoundException::~HostNotFoundException()
			{
				if(this->reason)
					free(this->reason);
			}
				
			const char* HostNotFoundException::toString(void) const
			{
				return (const char*)(this->reason);	
			}
		
		
			const HostNotFoundException& HostNotFoundException::operator = (const HostNotFoundException& rHostNotFoundException)
			{
				const char* reason = rHostNotFoundException.toString();
				this->reason = (char*) calloc(strlen(reason) + 1, sizeof(char));
				
				return *this;
			}
		
	} // namespace Msg	

}// namespace SimGrid



