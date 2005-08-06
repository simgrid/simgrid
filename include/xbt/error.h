/* $Id$ */

/* xbt/error.h - Error tracking support                                     */

/* Copyright (c) 2003, 2004 Martin Quinson. All rights reserved.            */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef XBT_ERROR_H
#define XBT_ERROR_H

#include "xbt/misc.h" /* BEGIN_DECL */
#include "xbt/log.h"

BEGIN_DECL()

/** @addtogroup XBT_error 
 *
 *  This is how the errors get reported in the SimGrid toolkit. This mechanism is not 
 *  as powerful as exceptions, but since we're using C, there is not much we can do.
 *
 *  @{*/

/** @name 1. Type definition and basic operations
 *
 *  @{
 */
/** \brief Error types */
typedef enum {
  no_error=0,       /**< succes */
  old_mismatch_error=1, /**< The provided ID does not match */
  old_system_error=2,   /**< a syscall did fail */
  old_network_error=3,  /**< error while sending/receiving data */
  old_timeout_error=4,  /**< not quick enough, dude */
  old_thread_error=5,   /**< error while [un]locking */
  old_unknown_error=6   /**< unknown error */
     
} xbt_error_t;

 const char *xbt_error_name(xbt_error_t errcode);
 void xbt_abort(void) _XBT_GNUC_NORETURN;
 void xbt_die(const char *msg) _XBT_GNUC_NORETURN;

/** @} */

/** @name 2. TRY macro family
 * 
 * Those functions are used to launch a function call and react automatically 
 * to its return value. They expect such a variable to be declared in the scope:
 * \verbatim xbt_error_t errcode;\endverbatim
 * @{
 */

/** @brief return the error code if != no_error
 *  @hideinitializer
 */
#define TRYOLD(action) do {                                       \
  if ((errcode=action) != no_error) {                          \
     ERROR1("'%s' error raising...", xbt_error_name(errcode)); \
     return errcode;                                           \
  } } while (0)
   
/** @brief return the error code if != no_error and != \a catched
 *  @hideinitializer
 */
#define TRYCATCH(action,catched) if ((errcode=action) != no_error && errcode !=catched) return errcode

/** @brief xbt_abort if the error code != no_error
 *  @hideinitializer
 */
#define TRYFAIL(action) do {                                   \
  if ((errcode=action) != no_error) {                          \
     ERROR1("Got '%s' error !", xbt_error_name(errcode));      \
     fflush(stdout);                                           \
     xbt_abort();                                              \
  } } while(0)


/** @brief return the error code if != \a expected_error (no_error not ok)
 *  @hideinitializer
 */
#define TRYEXPECT(action,expected_error)  do {                 \
  errcode=action;                                              \
  if (errcode != expected_error) {                             \
    ERROR2("Got error %s (instead of %s expected)\n",          \
	    xbt_error_name(errcode),                          \
	    xbt_error_name(expected_error));                  \
    xbt_abort();                                              \
  }                                                            \
} while(0)

/** @}*/

#define _XBT_ERR_PRE do {
#define _XBT_ERR_POST(code)                                    \
  return code;                                                 \
} while (0)
  

/** @name 3. RAISE macro family
 *
 *  Return a error code, doing some logs on stderr.
 *
 *  @todo This should use the logging features, not stderr
 * 
 *  @{
 */

/** @hideinitializer  */
#define OLDRAISE0(code,fmt)                   _XBT_ERR_PRE   ERROR0(fmt);                   _XBT_ERR_POST(code)
/** @hideinitializer  */
#define OLDRAISE1(code,fmt,a1)                _XBT_ERR_PRE   ERROR1(fmt,a1);                _XBT_ERR_POST(code)
/** @hideinitializer  */
#define OLDRAISE2(code,fmt,a1,a2)             _XBT_ERR_PRE   ERROR2(fmt,a1,a2);             _XBT_ERR_POST(code)
/** @hideinitializer  */
#define OLDRAISE3(code,fmt,a1,a2,a3)          _XBT_ERR_PRE   ERROR3(fmt,a1,a2,a3);          _XBT_ERR_POST(code)
/** @hideinitializer  */
#define OLDRAISE4(code,fmt,a1,a2,a3,a4)       _XBT_ERR_PRE   ERROR4(fmt,a1,a2,a3,a4);       _XBT_ERR_POST(code)
/** @hideinitializer  */
#define OLDRAISE5(code,fmt,a1,a2,a3,a4,a5)    _XBT_ERR_PRE   ERROR5(fmt,a1,a2,a3,a4,a5);    _XBT_ERR_POST(code)
/** @hideinitializer  */
#define OLDRAISE6(code,fmt,a1,a2,a3,a4,a5,a6) _XBT_ERR_PRE   ERROR6(fmt,a1,a2,a3,a4,a5,a6); _XBT_ERR_POST(code)

/** @} */
/** 
 * \name 4. assert macro familly
 *
 * Those are the GRAS version of the good ol' assert macro. You can pass them a format message and 
 * arguments, just as if it where a printf. It is converted to a CRITICALn logging request.
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

/** @} */

/** @name 5. Useful predefined errors 
 *
 *  @{ 
 */
#define RAISE_IMPOSSIBLE RAISE0(old_unknown_error,"The Impossible did happen (yet again)")
#define RAISE_UNIMPLEMENTED RAISE1(old_unknown_error,"Function %s unimplemented",__FUNCTION__)

#define OLDDIE_IMPOSSIBLE xbt_assert0(0,"The Impossible did happen (yet again)")
#define OLDxbt_assert_error(a) xbt_assert1(errcode == (a), "Error %s unexpected",xbt_error_name(errcode))

/** @} */
/** @} */

END_DECL()

#endif /* XBT_ERROR_H */
