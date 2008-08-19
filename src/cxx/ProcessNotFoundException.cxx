/*
 * ProcessNotFoundException.cxx
 *
 * Copyright 2006,2007 Martin Quinson, Malek Cherier           
 * All right reserved. 
 *
 * This program is free software; you can redistribute 
 * it and/or modify it under the terms of the license 
 *(GNU LGPL) which comes with this package. 
 *
 */
 
 /* ProcessNotFoundException member functions implementation.
  */  

#include <ProcessNotFoundException.hpp>

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

namespace SimGrid
{
	namespace Msg
	{
		
			ProcessNotFoundException::ProcessNotFoundException()
			{
				this->reason = (char*) calloc(strlen("Host not found : unknown") + 1, sizeof(char));
				strcpy(this->reason, "Host not found : unknown");
			}
		
		
			ProcessNotFoundException::ProcessNotFoundException(const ProcessNotFoundException& rProcessNotFoundException)
			{
				const char* reason = rProcessNotFoundException.toString();
				this->reason = (char*) calloc(strlen(reason) + 1, sizeof(char));
				strcpy(this->reason, reason);
				
			}
		
		
			ProcessNotFoundException::ProcessNotFoundException(int PID)
			{
				char buff[7] = {0};
				#ifdef WIN32
				_itoa(PID, buff, 10);
				#else
				sprintf(buff,"%d",PID);
				#endif
				this->reason = (char*) calloc(strlen("Process not found : ") + strlen(buff) + 1, sizeof(char));
				sprintf(this->reason, "Host not found : %s", buff);
			}
		
		
			ProcessNotFoundException::~ProcessNotFoundException()
			{
				if(this->reason)
					free(this->reason);
			}
				
			const char* ProcessNotFoundException::toString(void) const
			{
				return (const char*)(this->reason);	
			}
		
		
			const ProcessNotFoundException& ProcessNotFoundException::operator = (const ProcessNotFoundException& rProcessNotFoundException)
			{
				const char* reason = rProcessNotFoundException.toString();
				this->reason = (char*) calloc(strlen(reason) + 1, sizeof(char));
				
				return *this;
			}
		
	} // namespace Msg	

}// namespace SimGrid



