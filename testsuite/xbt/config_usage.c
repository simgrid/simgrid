/* $Id$ */

/* test config - test code to the config set */

#include <stdio.h>
#include <gras.h>

/*====[ Prototypes ]=========================================================*/
gras_cfg_t make_set(void); /* build a minimal set */

/*====[ Code ]===============================================================*/
gras_cfg_t make_set(){
  gras_cfg_t set=NULL; 
  gras_error_t errcode;

  set = gras_cfg_new();
  TRYFAIL(gras_cfg_register_str(set,"speed:1_to_2_int"));
  TRYFAIL(gras_cfg_register_str(set,"hostname:1_to_1_string"));
  TRYFAIL(gras_cfg_register_str(set,"user:1_to_10_string"));

  TRYFAIL(gras_cfg_set_parse(set, "hostname:veloce "
			     "user:mquinson\nuser:oaumage\tuser:alegrand"));
  return set;
}
 
int main(int argc, char **argv) {
  gras_error_t errcode;
  gras_cfg_t set;

  gras_dynar_t dyn;
  char *str;
  int ival;
  
  gras_init_defaultlog(&argc,argv,"config.thresh=debug root.thresh=info");

  fprintf(stderr,"==== Alloc and free a config set.\n");
  set=make_set();
  gras_cfg_dump("test set","",set);
  gras_cfg_free(&set);
  gras_cfg_free(&set);


  fprintf(stderr,"==== Try to use an unregistered option. (ERROR EXPECTED: 'color' not registered)\n");
  set=make_set();
  TRYEXPECT(mismatch_error,gras_cfg_set_parse(set,"color:blue"));
  gras_cfg_free(&set);
  gras_cfg_free(&set);


  fprintf(stderr,
	  "==== Validation test. (ERROR EXPECTED: not enough values of 'speed')\n");
  set=make_set();
  gras_cfg_check(set);
  gras_cfg_free(&set);
  gras_cfg_free(&set);

  fprintf(stderr,"==== Validation test (ERROR EXPECTED: too many elements)\n");
  set=make_set();
  gras_cfg_set_parse(set,"hostname:toto:42");
  gras_cfg_set_parse(set,"speed:42 speed:24 speed:34");
  gras_cfg_check(set);
  gras_cfg_free(&set);
  gras_cfg_free(&set);

  fprintf(stderr,"==== Get single value (Expected: 'speed value: 42')\n");
  set=make_set();
  gras_cfg_set_parse(set,"hostname:toto:42 speed:42");
  gras_cfg_get_int(set,"speed",&ival);
  fprintf(stderr,"speed value: %d\n",ival); 
  gras_cfg_free(&set);
  gras_cfg_free(&set);

  fprintf(stderr,"==== Get multiple values (Expected: 'Count: 3; Options: mquinson;ecaron;alegrand;')\n");
  set=make_set();
  gras_cfg_set_parse(set,"speed:42");
  gras_cfg_check(set);
  gras_cfg_get_dynar(set,"user",&dyn);
  fprintf(stderr,"Count: %lu; Options: ",gras_dynar_length(dyn));
  gras_dynar_foreach(dyn,ival,str) {
    fprintf(stderr,"%s;",str);
  }
  fprintf(stderr,"\n");
  gras_cfg_free(&set);
  gras_cfg_free(&set);

  gras_exit();
  return 0;
}

