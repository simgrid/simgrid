/* $Id$ */
/*  xbt/asserts.h -- assertion mecanism                                     */

/* Copyright (c) 2004,2005 Martin Quinson. All rights reserved.             */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef _XBT_ASSERTS_H
#define _XBT_ASSERTS_H

#include "xbt/misc.h"
  
SG_BEGIN_DECL()

/**
 * @addtogroup XBT_error
 * @brief Those are the SimGrid version of the good ol' assert macro.
 *
 * <center><table><tr><td><b>Top</b>    <td> [\ref index]::[\ref XBT_API]
 *                <tr><td><b>Prev</b>   <td> [\ref XBT_log]
 *                <tr><td><b>Next</b>   <td> [\ref XBT_config]     </table></center>
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
#define xbt_assert(cond)                  if (!(cond)) { CRITICAL1("Assertion %s failed", #cond); xbt_abort(); }
     /** @hideinitializer  */
#define xbt_assert0(cond,msg)             if (!(cond)) { CRITICAL0(msg); xbt_abort(); }
     /** @hideinitializer  */
#define xbt_assert1(cond,msg,a)           if (!(cond)) { CRITICAL1(msg,a); xbt_abort(); }
     /** @hideinitializer  */
#define xbt_assert2(cond,msg,a,b)         if (!(cond)) { CRITICAL2(msg,a,b); xbt_abort(); }
     /** @hideinitializer  */
#define xbt_assert3(cond,msg,a,b,c)       if (!(cond)) { CRITICAL3(msg,a,b,c); xbt_abort(); }
     /** @hideinitializer  */
#define xbt_assert4(cond,msg,a,b,c,d)     if (!(cond)) { CRITICAL4(msg,a,b,c,d); xbt_abort(); }
     /** @hideinitializer  */
#define xbt_assert5(cond,msg,a,b,c,d,e)   if (!(cond)) { CRITICAL5(msg,a,b,c,d,e); xbt_abort(); }
     /** @hideinitializer  */
#define xbt_assert6(cond,msg,a,b,c,d,e,f) if (!(cond)) { CRITICAL6(msg,a,b,c,d,e,f); xbt_abort(); }
#endif
     
void xbt_abort(void) _XBT_GNUC_NORETURN;
void xbt_die(const char *msg) _XBT_GNUC_NORETURN;

/** @} */     
  
SG_END_DECL()

#endif /* _XBT_ASSERTS_H */
