/* $Id$                     */

/* gras/cond.h - public interface to conditional execution                  */
/*                (specific parts for SG or RL)                             */
 
/* Authors: Martin Quinson                                                  */
/* Copyright (C) 2003,2004 da GRAS posse.                                   */

/* This program is free software; you can redistribute it and/or modify it
   under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef GRAS_COND_H
#define GRAS_COND_H

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

/**
 * gras_if_RL:
 * 
 * Returns true only if the program runs on real life
 */
int gras_if_RL(void);

/**
 * gras_if_SG:
 * 
 * Returns true only if the program runs within the simulator
 */
int gras_if_SG(void);

END_DECL

#endif /* GRAS_COND_H */

