/* $Id$                     */

/* gras/core.h - Unsorted part of the GRAS public interface                 */

/* Copyright (c) 2003, 2004 Martin Quinson. All rights reserved.            */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef GRAS_CORE_H
#define GRAS_CORE_H

#include <stddef.h>    /* offsetof() */
#include <sys/types.h>  /* size_t */
#include <stdarg.h>


/*! C++ users need love */
#ifndef BEGIN_DECL
# ifdef __cplusplus
#  define BEGIN_DECL extern "C" {
# else
#  define BEGIN_DECL 
# endif
#endif

/*! C++ users need love */
#ifndef END_DECL
# ifdef __cplusplus
#  define END_DECL }
# else
#  define END_DECL 
# endif
#endif
/* End of cruft for C++ */

BEGIN_DECL

/* **************************************************************************
 * Garbage collection support
 * **************************************************************************/
typedef enum { free_after_use, free_never } e_xbt_free_directive_t;


/* **************************************************************************
 * Wrappers over OS functions
 * **************************************************************************/

/**
 * gras_get_my_fqdn:
 *
 * Returns the fully-qualified name of the host machine, or NULL if the name
 * cannot be determined.  Always returns the same value, so multiple calls
 * cause no problems.
 */
const char *
gras_get_my_fqdn(void);


END_DECL

#endif /* GRAS_CORE_H */

