#include "MsgException.hpp"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

using namespace std;

namespace SimGrid
{
	namespace Msg
	{
		
			MsgException::MsgException()
			{
				this->reason = (char*) calloc(strlen("Internal exception : unknown reason") + 1, sizeof(char));
				strcpy(this->reason, "Internal exception : unknown reason");
			}
		
		
			MsgException::MsgException(const MsgException& rMsgException)
			{
				const char* reason = rMsgException.toString();
				this->reason = (char*) calloc(strlen(reason) + 1, sizeof(char));
				strcpy(this->reason, reason);
				
			}
		
			MsgException::MsgException(const char* reason)
			{
				this->reason = (char*) calloc(strlen("Internal exception : ") + strlen(reason) + 1, sizeof(char));
				sprintf(this->reason, "Invalid exception : %s", reason);
			}
		
		
			MsgException::~MsgException()
			{
				if(this->reason)
					free(this->reason);
			}
				
			const char* MsgException::toString(void) const
			{
				return (const char*)(this->reason);	
			}
		
		
			const MsgException& MsgException::operator = (const MsgException& rMsgException)
			{
				const char* reason = rMsgException.toString();
				this->reason = (char*) calloc(strlen(reason) + 1, sizeof(char));
				
				return *this;
			}
		
	} // namespace Msg	

}// namespace SimGrid



