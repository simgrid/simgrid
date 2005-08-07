/* $Id$ */

/* test config - test code to the config set */

/* Copyright (c) 2004 Martin Quinson. All rights reserved.                  */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <stdio.h>
#include "gras.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(test,"Logging for this test");

/*====[ Code ]===============================================================*/
static xbt_cfg_t make_set(){
  xbt_cfg_t set=NULL; 
  xbt_error_t errcode;

  set = xbt_cfg_new();
  TRYFAIL(xbt_cfg_register_str(set,"speed:1_to_2_int"));
  TRYFAIL(xbt_cfg_register_str(set,"hostname:1_to_1_string"));
  TRYFAIL(xbt_cfg_register_str(set,"user:1_to_10_string"));

  return set;
} /* end_of_make_set */
 
int main(int argc, char **argv) {
  xbt_ex_t e;
  xbt_cfg_t set;

  char *str;
  
  xbt_init(&argc,argv);

  fprintf(stderr,"==== Alloc and free a config set.\n");
  set=make_set();
  xbt_cfg_set_parse(set, "hostname:veloce user:mquinson\nuser:oaumage\tuser:alegrand");
  xbt_cfg_dump("test set","",set);
  xbt_cfg_free(&set);
  xbt_cfg_free(&set);

  fprintf(stderr,
	  "==== Validation test. (ERROR EXPECTED: not enough values of 'speed')\n");
  set=make_set();
  xbt_cfg_set_parse(set, "hostname:veloce user:mquinson\nuser:oaumage\tuser:alegrand");
  xbt_cfg_check(set);
  xbt_cfg_free(&set);
  xbt_cfg_free(&set);

  fprintf(stderr,"==== Validation test\n");
  set=make_set();
  TRY {
    xbt_cfg_set_parse(set,"hostname:toto:42");
    xbt_cfg_set_parse(set,"speed:42 speed:24 speed:34");
  } CATCH(e) {
    if (e.category != mismatch_error)
      RETHROW;
    xbt_ex_free(e);
  }
  xbt_cfg_check(set);
  xbt_cfg_free(&set);
  xbt_cfg_free(&set);

  fprintf(stderr,"==== Get single value (Expected: 'speed value: 42')\n");
  {	
  /* get_single_value */
  int ival;
  xbt_cfg_t myset=make_set();
     
  xbt_cfg_set_parse(myset,"hostname:toto:42 speed:42");
  ival = xbt_cfg_get_int(myset,"speed"); 
  fprintf(stderr,"speed value: %d\n",ival); /* Prints: "speed value: 42" */
  xbt_cfg_free(&myset);
  }
   
  fprintf(stderr,"==== Get multiple values (Expected: 'Count: 3; Options: mquinson;ecaron;alegrand;')\n");
  {	
  /* get_multiple_value */
  xbt_dynar_t dyn; 
  int ival;
  xbt_cfg_t myset=make_set();
     
  xbt_cfg_set_parse(myset, "hostname:veloce user:mquinson\nuser:oaumage\tuser:alegrand");
  xbt_cfg_set_parse(myset,"speed:42");
  xbt_cfg_check(myset); 
  dyn = xbt_cfg_get_dynar(myset,"user");
  fprintf(stderr,"Count: %lu; Options: ",xbt_dynar_length(dyn));
  xbt_dynar_foreach(dyn,ival,str) {
    fprintf(stderr,"%s;",str); 
  }
  fprintf(stderr,"\n");
  /* This prints: "Count: 3; Options: mquinson;ecaron;alegrand;" */
  xbt_cfg_free(&myset);
  }
   
  fprintf(stderr,"==== Try to use an unregistered option. (ERROR EXPECTED: 'color' not registered)\n");
  {
  xbt_cfg_t myset=make_set();
  TRY {
    xbt_cfg_set_parse(myset,"color:blue");
    THROW1(mismatch_error,0,"Found an option which shouldn't be there (%s)","color:blue");
  } CATCH(e) {
    if (e.category != not_found_error)
      RETHROW;
    xbt_ex_free(e);
  }
  /* This spits an error: 'color' not registered */
  xbt_cfg_free(&myset);
  }

  fprintf(stderr,"==== Success\n");
  xbt_exit();
  return 0;
}

