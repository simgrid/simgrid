/* platf_interface.h - Internal interface to the SimGrid platforms          */

/* Copyright (c) 2004-2007, 2009-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SG_PLATF_INTERFACE_H
#define SG_PLATF_INTERFACE_H

#include <xbt/dict.h>

#include "simgrid/platf.h" /* public interface */
#include "xbt/RngStream.h"

#include <simgrid/forward.h>

SG_BEGIN_DECL()

/* Module management functions */
XBT_PUBLIC(void) sg_platf_init(void);
XBT_PUBLIC(void) sg_platf_exit(void);

/** \brief Pick the right models for CPU, net and host, and call their model_init_preparse
 *
 * Must be called within parsing/creating the environment (after the <config>s, if any, and before <AS> or friends such as <cluster>)
 */
XBT_PUBLIC(void) surf_config_models_setup(void);

SG_END_DECL()

#endif                          /* SG_PLATF_INTERFACE_H */
