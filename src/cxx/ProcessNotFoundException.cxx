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
				_itoa(PID, buff, 10);
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



