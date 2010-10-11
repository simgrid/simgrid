#ifndef __DEFS_H__
#define __DEFS_H__

#include <assert.h>

/* NULL definition*/
#ifndef NULL
#ifdef __cplusplus
#define NULL 0
#else
#define NULL ((void *)0)
#endif
#endif                          /* #ifndef NULL */

#if !defined(__cplusplus) && !defined(__BOOL_TYPE_DEFINED)
typedef int bool;
#define __BOOL_TYPE_DEFINED
#endif                          /* #ifndef __cplusplus */

#ifndef __SSIZE_TYPE_DEFINED
typedef int ssize_t;
#define __SSIZE_TYPE_DEFINED
#endif                          /* #ifndef __SSIZE_TYPE_DEFINED */

#ifndef true
#define true 1
#endif                          /* #ifndef true */

#ifndef false
#define false 0
#endif                          /* #ifndef false */

/* Asserts that a condition is true.*/
#define ASSERT(c)						assert(c)
/* Asserts that a pointer is not NULL.*/
#define ASSERT_NOT_NULL(p)				assert(NULL != p)

/* Error number type (int) */
#ifndef __ERRNO_TYPE_DEFINED
typedef int errno_t;
#define __ERRNO_TYPE_DEFINED
#endif                          /* #ifndef __ERRNO_TYPE_DEFINED */

/* comment this line if you don't want activate the verbose mode. */
#define __VERBOSE

#endif                          /*  #ifndef __DEFS_H__ */
