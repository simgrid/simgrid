/* $Id$ */

/* rl_dns - name resolution (real life)                                     */

/* Copyright (c) 2003, 2004 Martin Quinson. All rights reserved.            */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "gras/Virtu/virtu_rl.h"

/* A portable DNS resolver is a nightmare to do in a portable manner */

/* the ADNS library is a good candidate for inclusion in the source tree, but
 * it would be a bit too much. We need a stripped down version of it, and it's
 * a bit too late for this tonight.
 * 
 * Next time maybe ;)
 */
const char *gras_os_myname(void) {
   return "(unknown host)";
}

