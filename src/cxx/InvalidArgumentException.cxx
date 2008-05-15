#include "InvalidArgumentException.hpp"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

using namespace std;

namespace SimGrid
{
	namespace Msg
	{
		
			InvalidArgumentException::InvalidArgumentException()
			{
				this->reason = (char*) calloc(strlen("Invalid argument : unknown") + 1, sizeof(char));
				strcpy(this->reason, "Invalid argument : unknown");
			}
		
		
			InvalidArgumentException::InvalidArgumentException(const InvalidArgumentException& rInvalidArgumentException)
			{
				const char* reason = rInvalidArgumentException.toString();
				this->reason = (char*) calloc(strlen(reason) + 1, sizeof(char));
				strcpy(this->reason, reason);
				
			}
		
		
			InvalidArgumentException::InvalidArgumentException(const char* name)
			{
				this->reason = (char*) calloc(strlen("Invalid argument : ") + strlen(name) + 1, sizeof(char));
				sprintf(this->reason, "Invalid argument : %s", name);
			}
		
		
			InvalidArgumentException::~InvalidArgumentException()
			{
				if(this->reason)
					free(this->reason);
			}
				
			const char* InvalidArgumentException::toString(void) const
			{
				return (const char*)(this->reason);	
			}
		
		
			const InvalidArgumentException& InvalidArgumentException::operator = (const InvalidArgumentException& rInvalidArgumentException)
			{
				const char* reason = rInvalidArgumentException.toString();
				this->reason = (char*) calloc(strlen(reason) + 1, sizeof(char));
				
				return *this;
			}
		
	} // namespace Msg	

}// namespace SimGrid



