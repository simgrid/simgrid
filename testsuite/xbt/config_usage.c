/* $Id$ */

/* test config - test code to the config set */

#include <stdio.h>
#include <gras.h>

/*====[ Prototypes ]=========================================================*/
xbt_cfg_t make_set(void); /* build a minimal set */

/*====[ Code ]===============================================================*/
xbt_cfg_t make_set(){
  xbt_cfg_t set=NULL; 
  xbt_error_t errcode;

  set = xbt_cfg_new();
  TRYFAIL(xbt_cfg_register_str(set,"speed:1_to_2_int"));
  TRYFAIL(xbt_cfg_register_str(set,"hostname:1_to_1_string"));
  TRYFAIL(xbt_cfg_register_str(set,"user:1_to_10_string"));

  TRYFAIL(xbt_cfg_set_parse(set, "hostname:veloce "
			     "user:mquinson\nuser:oaumage\tuser:alegrand"));
  return set;
}
 
int main(int argc, char **argv) {
  xbt_error_t errcode;
  xbt_cfg_t set;

  xbt_dynar_t dyn;
  char *str;
  int ival;
  
  xbt_init_defaultlog(&argc,argv,"config.thresh=debug root.thresh=info");

  fprintf(stderr,"==== Alloc and free a config set.\n");
  set=make_set();
  xbt_cfg_dump("test set","",set);
  xbt_cfg_free(&set);
  xbt_cfg_free(&set);


  fprintf(stderr,"==== Try to use an unregistered option. (ERROR EXPECTED: 'color' not registered)\n");
  set=make_set();
  TRYEXPECT(mismatch_error,xbt_cfg_set_parse(set,"color:blue"));
  xbt_cfg_free(&set);
  xbt_cfg_free(&set);


  fprintf(stderr,
	  "==== Validation test. (ERROR EXPECTED: not enough values of 'speed')\n");
  set=make_set();
  xbt_cfg_check(set);
  xbt_cfg_free(&set);
  xbt_cfg_free(&set);

  fprintf(stderr,"==== Validation test (ERROR EXPECTED: too many elements)\n");
  set=make_set();
  xbt_cfg_set_parse(set,"hostname:toto:42");
  xbt_cfg_set_parse(set,"speed:42 speed:24 speed:34");
  xbt_cfg_check(set);
  xbt_cfg_free(&set);
  xbt_cfg_free(&set);

  fprintf(stderr,"==== Get single value (Expected: 'speed value: 42')\n");
  set=make_set();
  xbt_cfg_set_parse(set,"hostname:toto:42 speed:42");
  xbt_cfg_get_int(set,"speed",&ival);
  fprintf(stderr,"speed value: %d\n",ival); 
  xbt_cfg_free(&set);
  xbt_cfg_free(&set);

  fprintf(stderr,"==== Get multiple values (Expected: 'Count: 3; Options: mquinson;ecaron;alegrand;')\n");
  set=make_set();
  xbt_cfg_set_parse(set,"speed:42");
  xbt_cfg_check(set);
  xbt_cfg_get_dynar(set,"user",&dyn);
  fprintf(stderr,"Count: %lu; Options: ",xbt_dynar_length(dyn));
  xbt_dynar_foreach(dyn,ival,str) {
    fprintf(stderr,"%s;",str);
  }
  fprintf(stderr,"\n");
  xbt_cfg_free(&set);
  xbt_cfg_free(&set);

  xbt_exit();
  return 0;
}

