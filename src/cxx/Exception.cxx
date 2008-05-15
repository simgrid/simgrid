#include "Exception.hpp"

namespace SimGrid
{
	namespace Msg
	{
		
			Exception::Exception()
			{
				reason = "Unknown reason";
			}
		
		
			Exception::Exception(const Exception& rException)
			{
				this->reason = rException.toString();
			}
		
		
			Exception::Exception(const char* reason)
			{
				this->reason = reason;
			}
		
		
			Exception::~Exception()
			{
				// NOTHING TODO
			}
				
			const char* Exception::toString(void) const
			{
				return this->reason;	
			}
		
		
			const Exception& Exception::operator = (const Exception& rException)
			{
				this->reason = rException.toString();
				return *this;
			}
		
	} // namespace Msg	

}// namespace SimGrid



