/*  xbt/asserts.h -- assertion mecanism                                     */

/* Copyright (c) 2005, 2006, 2007, 2009, 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef _XBT_ASSERTS_H
#define _XBT_ASSERTS_H

#include "xbt/misc.h"
#include "xbt/log.h"
#include "xbt/ex.h"

SG_BEGIN_DECL()

/**
 * @addtogroup XBT_error
 * @brief Those are the SimGrid version of the good ol' assert macro.
 *
 * You can pass them a format message and arguments, just as if it where a printf.
 * It is converted to a CRITICALn logging request.
 *
 * @{
 */
#ifdef NDEBUG
#define xbt_assert(cond)
#define xbt_assert0(cond,msg)
#define xbt_assert1(cond,msg,a)
#define xbt_assert2(cond,msg,a,b)
#define xbt_assert3(cond,msg,a,b,c)
#define xbt_assert4(cond,msg,a,b,c,d)
#define xbt_assert5(cond,msg,a,b,c,d,e)
#define xbt_assert6(cond,msg,a,b,c,d,e,f)
#else
     /** @brief The condition which failed will be displayed.
	 @hideinitializer  */
#define xbt_assert(cond)                  do { if (!(cond)) THROW1(0,0,"Assertion %s failed", #cond); } while (0)
     /** @hideinitializer  */
#define xbt_assert0(cond,msg)             do { if (!(cond)) THROW0(0,0,msg); } while (0)
     /** @hideinitializer  */
#define xbt_assert1(cond,msg,a)           do { if (!(cond)) THROW1(0,0,msg,a); } while (0)
     /** @hideinitializer  */
#define xbt_assert2(cond,msg,a,b)         do { if (!(cond)) THROW2(0,0,msg,a,b); } while (0)
     /** @hideinitializer  */
#define xbt_assert3(cond,msg,a,b,c)       do { if (!(cond)) THROW3(0,0,msg,a,b,c); } while (0)
     /** @hideinitializer  */
#define xbt_assert4(cond,msg,a,b,c,d)     do { if (!(cond)) THROW4(0,0,msg,a,b,c,d); } while (0)
     /** @hideinitializer  */
#define xbt_assert5(cond,msg,a,b,c,d,e)   do { if (!(cond)) THROW5(0,0,msg,a,b,c,d,e); } while (0)
     /** @hideinitializer  */
#define xbt_assert6(cond,msg,a,b,c,d,e,f) do { if (!(cond)) THROW6(0,0,msg,a,b,c,d,e,f); } while (0)
#endif
/** @} */
    SG_END_DECL()
#endif                          /* _XBT_ASSERTS_H */
