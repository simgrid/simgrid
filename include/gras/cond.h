/* $Id$                     */

/* gras/cond.h - public interface to conditional execution                  */
/*                (specific parts for SG or RL)                             */
 
/* Copyright (c) 2004 Martin Quinson. All rights reserved.                  */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef GRAS_COND_H
#define GRAS_COND_H

#include "xbt/misc.h" /* BEGIN_DECL */

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

