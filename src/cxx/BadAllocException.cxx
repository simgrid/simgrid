#include "BadAllocException.hpp"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

using namespace std;

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



