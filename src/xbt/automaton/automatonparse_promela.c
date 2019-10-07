/* methods for implementation of automaton from promela description */

/* Copyright (c) 2011-2019. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/internal_config.h"
#include "xbt/automaton.h"
#include <errno.h>
#include <string.h>   /* strerror */
#if HAVE_UNISTD_H
# include <unistd.h>   /* isatty */
#endif
#include <xbt/log.h>
#include <xbt/sysdep.h>

#include "parserPromela.tab.cacc"

XBT_LOG_EXTERNAL_DEFAULT_CATEGORY(xbt_automaton);

static xbt_automaton_t parsed_automaton;
char* state_id_src;

static void new_state(char* id, int src){
  char* saveptr = NULL; // for strtok_r()
  char* id_copy = xbt_strdup(id);
  char* first_part = strtok_r(id_copy, "_", &saveptr);
  int type = 0 ; // -1=initial state; 0=intermediate state; 1=final state

  if(strcmp(first_part,"accept")==0){
    type = 1;
  }else{
    char* second_part = strtok_r(NULL, "_", &saveptr);
    if(strcmp(second_part,"init")==0){
      type = -1;
    }
  }
  xbt_free(id_copy);

  xbt_automaton_state_t state = xbt_automaton_state_exists(parsed_automaton, id);
  if(state == NULL){
    state = xbt_automaton_state_new(parsed_automaton, type, id);
  }

  if(type==-1)
    parsed_automaton->current_state = state;

  if(src) {
    xbt_free(state_id_src);
    state_id_src = xbt_strdup(id);
  }
}

static void new_transition(char* id, xbt_automaton_exp_label_t label)
{
  new_state(id, 0);
  xbt_automaton_state_t state_dst = xbt_automaton_state_exists(parsed_automaton, id);
  xbt_automaton_state_t state_src = xbt_automaton_state_exists(parsed_automaton, state_id_src);

  xbt_automaton_transition_new(parsed_automaton, state_src, state_dst, label);

}

void xbt_automaton_load(xbt_automaton_t a, const char *file)
{
  parsed_automaton = a;
  yyin = fopen(file, "r");
  if (yyin == NULL)
    xbt_die("Failed to open automaton file `%s': %s", file, strerror(errno));
  yyparse();
}
