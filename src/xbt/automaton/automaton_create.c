#include "xbt/automatonparse_promela.h"
#include "y.tab.c"
#include "automaton_create.h"

xbt_automaton_t xbt_create_automaton(const char *file){

  init();
  yyin = fopen(file, "r");
  yyparse();
  return automaton;

}
