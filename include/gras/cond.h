/* $Id$                     */

/* gras/cond.h - public interface to conditional execution                  */
/*                (specific parts for SG or RL)                             */
 
/* Copyright (c) 2003, 2004 Martin Quinson. All rights reserved.            */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef GRAS_COND_H
#define GRAS_COND_H

#include "xbt/misc.h" /* BEGIN_DECL */

BEGIN_DECL()

/** @addtogroup GRAS_cond
 *  @brief Handling code specific to the simulation or to the reality (Virtualization).
 * 
 *  Please note that those are real functions and not pre-processor defines. This is to ensure
 *  that the same object code can be linked against the SG library or the RL one without recompilation.
 * 
 *  @{
 */
  
/** \brief Returns true only if the program runs on real life */
int gras_if_RL(void);

/** \brief Returns true only if the program runs within the simulator */
int gras_if_SG(void);

/** @} */

END_DECL()

#endif /* GRAS_COND_H */

