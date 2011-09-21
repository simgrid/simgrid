#ifndef _XBT_AUTOMATONPARSE_PROMELA_H
#define _XBT_AUTOMATONPARSE_PROMELA_H

#include <stdlib.h>
#include <string.h>
#include "xbt/automaton.h"

xbt_automaton_t automaton;

SG_BEGIN_DECL()

XBT_PUBLIC(void) init(void);

XBT_PUBLIC(void) new_state(char* id, int src);

XBT_PUBLIC(void) new_transition(char* id, xbt_exp_label_t label);

XBT_PUBLIC(xbt_exp_label_t) new_label(int type, ...);

XBT_PUBLIC(xbt_automaton_t) get_automaton(void);
			    
XBT_PUBLIC(void) display_automaton(void);

SG_END_DECL()

#endif
