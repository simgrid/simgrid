/*
 * Msg.hpp
 *
 * This file contains the declaration of the wrapper class of the native MSG task type.
 *
 * Copyright 2006,2007 Martin Quinson, Malek Cherier           
 * All right reserved. 
 *
 * This program is free software; you can redistribute 
 * it and/or modify it under the terms of the license 
 *(GNU LGPL) which comes with this package. 
 *
 */  
 
#ifndef MSG_HPP
#define MSG_HPP

// Compilation C++ recquise
#ifndef __cplusplus
	#error Msg.hpp requires C++ compilation (use a .cxx suffix)
#endif

#include <MsgException.hpp>

namespace SimGrid
{
	namespace Msg
	{
		
		/*! \brief init() - Initialize MSG (This function must be called at the begining of each simulation).
		 *
		 * \param argv			A list of arguments.
		 * \param argc			The number of arguments of the list.
		 */
		static void init(int argc, char** argv);
		
		/* \brief finalize() - Finalize MSG (This function must be called at the end of each simulation).
		 *
		 * \exception			If this function fails, it throws a exception describing the cause of the failure.
		 */
		static void finalize(void)
		throw (MsgException);
		
		/*! \brief info() - Display information (using xbt log format).
		 *
		 * \param s				The information to display.
		 */
		static void info(const char* s);
		
		
		
		
	} // namespace Msg
} // namespace SimGrid

#endif // !MSG_HPP