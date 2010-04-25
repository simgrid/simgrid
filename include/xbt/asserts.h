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
#define xbt_assert(cond)                  if (!(cond)) THROW1(0,0,"Assertion %s failed", #cond)
     /** @hideinitializer  */
#define xbt_assert0(cond,msg)             if (!(cond)) THROW0(0,0,msg)
     /** @hideinitializer  */
#define xbt_assert1(cond,msg,a)           if (!(cond)) THROW1(0,0,msg,a)
     /** @hideinitializer  */
#define xbt_assert2(cond,msg,a,b)         if (!(cond)) THROW2(0,0,msg,a,b)
     /** @hideinitializer  */
#define xbt_assert3(cond,msg,a,b,c)       if (!(cond)) THROW3(0,0,msg,a,b,c)
     /** @hideinitializer  */
#define xbt_assert4(cond,msg,a,b,c,d)     if (!(cond)) THROW4(0,0,msg,a,b,c,d)
     /** @hideinitializer  */
#define xbt_assert5(cond,msg,a,b,c,d,e)   if (!(cond)) THROW5(0,0,msg,a,b,c,d,e)
     /** @hideinitializer  */
#define xbt_assert6(cond,msg,a,b,c,d,e,f) if (!(cond)) THROW6(0,0,msg,a,b,c,d,e,f)
#endif
/** @} */
  SG_END_DECL()
#endif /* _XBT_ASSERTS_H */
