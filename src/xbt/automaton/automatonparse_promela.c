/* methods for implementation of automaton from promela description */

/* Copyright (c) 2011-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/internal_config.h"
#include "xbt/automaton.h"
#include <errno.h>
#include <string.h>   /* strerror */
#if HAVE_UNISTD_H
# include <unistd.h>   /* isatty */
#endif

static xbt_automaton_t parsed_automaton;
char* state_id_src;

static void new_state(char* id, int src){

  char* id_state = xbt_strdup(id);
  char* first_part = strtok(id,"_");
  int type = 0 ; // -1=initial state; 0=intermediate state; 1=final state

  if(strcmp(first_part,"accept")==0){
    type = 1;
  }else{
    char* second_part = strtok(NULL,"_");
    if(strcmp(second_part,"init")==0){
      type = -1;
    }
  }

  xbt_automaton_state_t state = NULL;
  state = xbt_automaton_state_exists(parsed_automaton, id_state);
  if(state == NULL){
    state = xbt_automaton_state_new(parsed_automaton, type, id_state);
  }

  if(type==-1)
    parsed_automaton->current_state = state;

  if(src)
    state_id_src = xbt_strdup(id_state);
    
}

static void new_transition(char* id, xbt_automaton_exp_label_t label){

  char* id_state = xbt_strdup(id);
  xbt_automaton_state_t state_dst = NULL;
  new_state(id, 0);
  state_dst = xbt_automaton_state_exists(parsed_automaton, id_state);
  xbt_automaton_state_t state_src = xbt_automaton_state_exists(parsed_automaton, state_id_src);
  
  //xbt_transition_t trans = NULL;
  xbt_automaton_transition_new(parsed_automaton, state_src, state_dst, label);

}

static xbt_automaton_exp_label_t new_label(int type, ...){
  xbt_automaton_exp_label_t label = NULL;
  va_list ap;
  va_start(ap,type);
  switch(type){
  case 0 : {
    xbt_automaton_exp_label_t left = va_arg(ap, xbt_automaton_exp_label_t);
    xbt_automaton_exp_label_t right = va_arg(ap, xbt_automaton_exp_label_t);
    label = xbt_automaton_exp_label_new(type, left, right);
    break;
  }
  case 1 : {
    xbt_automaton_exp_label_t left = va_arg(ap, xbt_automaton_exp_label_t);
    xbt_automaton_exp_label_t right = va_arg(ap, xbt_automaton_exp_label_t);
    label = xbt_automaton_exp_label_new(type, left, right);
    break;
  }
  case 2 : {
    xbt_automaton_exp_label_t exp_not = va_arg(ap, xbt_automaton_exp_label_t);
    label = xbt_automaton_exp_label_new(type, exp_not);
    break;
  }
  case 3 : {
    char* p = va_arg(ap, char*);
    label = xbt_automaton_exp_label_new(type, p);
    break;
  }
  case 4 : {
    label = xbt_automaton_exp_label_new(type);
    break;
  }
  }
  va_end(ap);
  return label;
}


#include "parserPromela.tab.cacc"

void xbt_automaton_load(xbt_automaton_t a, const char *file)
{
  parsed_automaton = a;
  yyin = fopen(file, "r");
  if (yyin == NULL)
    xbt_die("Failed to open automaton file `%s': %s", file, strerror(errno));
  yyparse();
}
