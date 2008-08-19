
/*
 * Config.hpp
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

#ifndef MSG_CONFIG_HPP
#define MSG_CONFIG_HPP

#ifndef __cplusplus
	#error Config.hpp requires C++ compilation (use a .cxx suffix)
#endif

namespace SimGrid
{
	namespace Msg
	{
		#if defined(WIN32) && !defined(__MINGW32__)
			#if defined(SIMGRIDX_EXPORTS)
				#define SIMGRIDX_EXPORT __declspec(dllexport)
			#else
				#define SIMGRIDX_EXPORT __declspec(dllimport)
			#endif
		#else
			#define SIMGRIDX_EXPORT
		#endif 
	} // namespace Msg
} // namespace SimGrid

#ifndef WIN32
#define _strdup strdup
#endif

#endif // MSG_CONFIG_HPP

