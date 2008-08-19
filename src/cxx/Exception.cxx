/*
 * Exception.cxx
 *
 * Copyright 2006,2007 Martin Quinson, Malek Cherier           
 * All right reserved. 
 *
 * This program is free software; you can redistribute 
 * it and/or modify it under the terms of the license 
 *(GNU LGPL) which comes with this package. 
 *
 */
 
 /* Exception member functions implementation.
  * The base class of all the types of exceptions of SimGrid::Msg.
  */  

#include <Exception.hpp>

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



