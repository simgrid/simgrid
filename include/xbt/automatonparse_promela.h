/*  xbt/automatonparse_promela.h -- implementation automaton from promela file    */

/* Copyright (c) 2011. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef _XBT_AUTOMATONPARSE_PROMELA_H
#define _XBT_AUTOMATONPARSE_PROMELA_H

#include <stdlib.h>
#include <string.h>
#include "xbt/automaton.h"

SG_BEGIN_DECL()

XBT_PUBLIC_DATA(xbt_automaton_t) automaton;

XBT_PUBLIC(void) init(void);

XBT_PUBLIC(void) new_state(char* id, int src);

XBT_PUBLIC(void) new_transition(char* id, xbt_exp_label_t label);

XBT_PUBLIC(xbt_exp_label_t) new_label(int type, ...);

XBT_PUBLIC(xbt_automaton_t) get_automaton(void);
			    
XBT_PUBLIC(void) display_automaton(void);

SG_END_DECL()

#endif
