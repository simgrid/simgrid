/*
 * ClassNotFoundException.cxx
 *
 * Copyright 2006,2007 Martin Quinson, Malek Cherier           
 * All right reserved. 
 *
 * This program is free software; you can redistribute 
 * it and/or modify it under the terms of the license 
 *(GNU LGPL) which comes with this package. 
 *
 */
 
 /* ClassNotFoundException member functions implementation.
  */  

#include <ClassNotFoundException.hpp>

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

namespace SimGrid
{
	namespace Msg
	{
		
			ClassNotFoundException::ClassNotFoundException()
			{
				this->reason = (char*) calloc(strlen("Class not found : unknown") + 1, sizeof(char));
				strcpy(this->reason, "Class not found : unknown");
			}
		
		
			ClassNotFoundException::ClassNotFoundException(const ClassNotFoundException& rClassNotFoundException)
			{
				const char* reason = rClassNotFoundException.toString();
				this->reason = (char*) calloc(strlen(reason) + 1, sizeof(char));
				strcpy(this->reason, reason);
				
			}
		
		
			ClassNotFoundException::ClassNotFoundException(const char* name)
			{
				this->reason = (char*) calloc(strlen("Class not found : ") + strlen(name) + 1, sizeof(char));
				sprintf(this->reason, "Class not found : %s", name);
			}
		
		
			ClassNotFoundException::~ClassNotFoundException()
			{
				if(this->reason)
					free(this->reason);
			}
				
			const char* ClassNotFoundException::toString(void) const
			{
				return (const char*)(this->reason);	
			}
		
		
			const ClassNotFoundException& ClassNotFoundException::operator = (const ClassNotFoundException& rClassNotFoundException)
			{
				const char* reason = rClassNotFoundException.toString();
				this->reason = (char*) calloc(strlen(reason) + 1, sizeof(char));
				
				return *this;
			}
		
	} // namespace Msg	

}// namespace SimGrid



