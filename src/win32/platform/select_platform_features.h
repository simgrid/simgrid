#ifndef __XBT_SELECT_PLATFORM_FEATURES_H__
#define __XBT_SELECT_PLATFORM_FEATURES_H__


/* select_platform_features.h - platform features selection    */

/* Copyright (c) 2006, 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/* Windows platforms. */


/* 
 * win32 or win64 (__XBT_WIN32 is defined for win32 and win64 applications, __TOS_WIN__ is defined by xlC).
*/

/* If the platform is not resolved _XBT_PLATFORM_ID is set to zero. */
#define _XBT_PLATFORM_ID 		0


#if defined(_WIN32) || defined(__WIN32__) || defined(WIN32) || defined(__TOS_WIN__)
#undef _XBT_PLATFORM_ID
#define _XBT_WIN32_PLATFORM	1
#define _XBT_PLATFORM_ID 	_XBT_WIN32_PLATFORM
#define _XBT_PLATFORM_NAME	"Windows 32 bits platform"
/* 
 * win64.
 */
#elif defined(_WIN64)
#undef _XBT_PLATFORM_ID
#ifdef _XBT_WIN32_PLATFORM
#undef _XBT_WIN32_PLATFORM
#endif

#define _XBT_PLATFORM_NAME	"Windows 64 bits platform"
#define _XBT_WIN64_PLATFORM 	2
#define _XBT_PLATFORM_ID 	_XBT_WIN64_PLATFORM
/* 
 * win16.
 */
#elif defined(_WIN16)
#undef _XBT_PLATFORM_ID
#define _XBT_WIN16_PLATFORM 	3
#define _XBT_PLATFORM_NAME	"Windows 16 bits platform"
#define _XBT_PLATFORM_ID 	_XBT_WIN16_PLATFORM
/*
 * wince.
 */
#elif defined(UNDER_CE)
#undef _XBT_PLATFORM_ID
#define __XBT_WINCE_PLATFORM 	4
#define _XBT_PLATFORM_ID 	_XBT_WINCE_PLATFORM
#define _XBT_PLATFORM_NAME	"Windows CE bits platform"

#elif defined(linux) || defined(__linux) || defined(__linux__)
/* linux. */
#undef _XBT_PLATFORM_ID
#define __XBT_LINUX_PLATFORM 	5
#define _XBT_PLATFORM_ID 	_XBT_LINUX_PLATFORM
#define _XBT_PLATFORM_NAME	"Linux platform"

#elif defined (_XBT_ASSERT_CONFIG)
        // This must come last - generate an error if we don't
        // resolve the platform:
#error "Unknown platform - please configure (simgrid.gforge.inria.fr/xbt/libs/config/config.htm#configuring) and report the results to the main simgrid mailing list (simgrid.gforge.inra.fr/more/mailing_lists.htm#main)"
#endif

#if defined(_XBT_WIN32_PLATFORM) || defined(_XBT_WIN64_PLATFORM) || defined(_XBT_WIN16_PLATFORM) || defined(_XBT_WINCE_PLATFORM)
#ifndef _XBT_WIN32
#define _XBT_WIN32
#endif
#endif


/* Returns true if the platform is resolved, false in the other case. */
#define _xbt_is_platform_resolved()	(_XBT_PLATFORM_ID != 0)

/* Returns the platform name. */
#define _xbt_get_platform_name()	_XBT_PLATFORM_NAME

/* Returns the platform id. */
#define _xbt_get_platform_id()		_XBT_PLATFORM_ID

/* Returns true if the platform is Windows 32 bits. */
#define _xbt_is_win32()			(_XBT_PLATFORM_ID == 1)

/* Returns true if the platform is Windows 64 bits. */
#define _xbt_is_win64()			(_XBT_PLATFORM_ID == 2)

/* Returns true if the platform is Windows 16 bits. */
#define _xbt_is_win16()			(_XBT_PLATFORM_ID == 3)

/* Returns true if the platform is Windows CE. */
#define _xbt_is_wince()			(_XBT_PLATFORM_ID == 4)

/* Returns true if the platform is linux. */
#define _xbt_is_linux()			(_XBT_PLATFORM_ID == 5)




#endif                          /* #define __XBT_SELECT_PLATFORM_FEATURES_H__ */
