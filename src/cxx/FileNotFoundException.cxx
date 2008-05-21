#include <FileNotFoundException.hpp>

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

using namespace std;

namespace SimGrid
{
	namespace Msg
	{
		
			FileNotFoundException::FileNotFoundException()
			{
				this->reason = (char*) calloc(strlen("File not found") + 1, sizeof(char));
				strcpy(this->reason, "File not found");
			}
		
		
			FileNotFoundException::FileNotFoundException(const FileNotFoundException& rFileNotFoundException)
			{
				const char* reason = rFileNotFoundException.toString();
				this->reason = (char*) calloc(strlen(reason) + 1, sizeof(char));
				strcpy(this->reason, reason);
				
			}
		
			FileNotFoundException::FileNotFoundException(const char* file)
			{
				this->reason = (char*) calloc(strlen("File not found ") + strlen(file) + 1, sizeof(char));
				sprintf(this->reason, "File not found %s", file);
			}
		
		
			FileNotFoundException::~FileNotFoundException()
			{
				if(this->reason)
					free(this->reason);
			}
				
			const char* FileNotFoundException::toString(void) const
			{
				return (const char*)(this->reason);	
			}
		
		
			const FileNotFoundException& FileNotFoundException::operator = (const FileNotFoundException& rFileNotFoundException)
			{
				const char* reason = rFileNotFoundException.toString();
				this->reason = (char*) calloc(strlen(reason) + 1, sizeof(char));
				
				return *this;
			}
		
	} // namespace Msg	

}// namespace SimGrid



