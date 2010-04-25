#ifndef __XBT_SELECT_COMPILER_FEATURES_H__
#define __XBT_SELECT_COMPILER_FEATURES_H__


/* select_compiler_features.h - compiler features selection    */

/* Copyright (c) 2006, 2007, 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#define _XBT_COMPILER_RESOLVED         0


/* Borland compilers */
#if defined (__BORLANDC__)
#define _XBT_COMPILER_NAME 	"Borland C/C++"
        #define _XBT_COMPILER_VENDOR    "Inprise Corporation"
#define _XBT_BORLAND_COMPILER 	2
/* version macro : __BORLANDC__ */
/* version format : ?           */
#define _XBT_COMPILER_VERSION 		__BORLANDC__
#define _XBT_COMPILER_VERSION_FORMAT	"?"

        #undef _XBT_COMPILER_RESOLVED
        #define _XBT_COMPILER_RESOLVED 1
#endif

/* Microsoft Visual C++ compiler */
#if defined(_MSC_VER)
#define _XBT_VISUALC_COMPILER
#endif


/* GGG compilers */

#if defined(__GNUC__)
#if defined(__GNUC_PATCHLEVEL__)
#define __GNUC_VERSION__ (__GNUC__ * 10000 + __GNUC_MINOR__ * 100 + __GNUC_PATCHLEVEL__)
#else
#define __GNUC_VERSION__ (__GNUC__ * 10000 + __GNUC_MINOR__ * 100)
#endif

#define _XBT_COMPILER_NAME 	"GCC"
        #define _XBT_COMPILER_VENDOR    "GNU"
#define _XBT_GCC_COMPILER 	2
/* version macro : __GNUC_VERSION__ */
/* version format : VVRRPP (VV = Version, RR = Revision, PP = Patch) */
#define _XBT_COMPILER_VERSION 		__GNUC_VERSION__
#define _XBT_COMPILER_VERSION_FORMAT	"VVRRPP (VV = Version, RR = Revision, PP = Patch)"

        #undef _XBT_COMPILER_RESOLVED
        #define _XBT_COMPILER_RESOLVED 1
#endif




/* Returns the compiler name. */
#define _xbt_get_compiler_name()		_XBT_COMPILER_NAME

/* Returns the compiler vendor. */
#define _xbt_get_compiler_vendor()		_XBT_COMPILER_VENDOR

/* Returns 1 if the compiler is resolved (0 in the other case). */
#define _xbt_is_compiler_resolved()		_XBT_COMPILER_RESOLVED

/* Returns the compiler version. */
#define _xbt_get_compiler_version()		 _XBT_STRINGIZE(_XBT_COMPILER_VERSION)

/* Returns the compiler version format. */
#define _xbt_get_compiler_version_format()	_XBT_COMPILER_VERSION_FORMAT



#endif /* #ifndef __XBT_SELECT_FEATURES_COMPILER_H__ */
