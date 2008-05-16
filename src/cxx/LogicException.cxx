#include "LogicException.hpp"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

using namespace std;

namespace SimGrid
{
	namespace Msg
	{
		
			LogicException::LogicException()
			{
				this->reason = (char*) calloc(strlen("Logic exception : no detail") + 1, sizeof(char));
				strcpy(this->reason, "Logic exception : no detail");
			}
		
		
			LogicException::LogicException(const LogicException& rLogicException)
			{
				const char* reason = rLogicException.toString();
				this->reason = (char*) calloc(strlen(reason) + 1, sizeof(char));
				strcpy(this->reason, reason);
				
			}
		
		
			LogicException::LogicException(const char* detail)
			{
				this->reason = (char*) calloc(strlen("Logic exception : ") + strlen(detail) + 1, sizeof(char));
				sprintf(this->reason, "Logic exception : %s", detail);
			}
		
		
			LogicException::~LogicException()
			{
				if(this->reason)
					free(this->reason);
			}
				
			const char* LogicException::toString(void) const
			{
				return (const char*)(this->reason);	
			}
		
		
			const LogicException& LogicException::operator = (const LogicException& rLogicException)
			{
				const char* reason = rLogicException.toString();
				this->reason = (char*) calloc(strlen(reason) + 1, sizeof(char));
				
				return *this;
			}
		
	} // namespace Msg	

}// namespace SimGrid



