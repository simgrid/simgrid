/* $Id$ */

/* test config - test code to the config set */

#include <stdio.h>
#include <gras.h>

/*====[ Prototypes ]=========================================================*/
gras_cfg_t *make_set(void); /* build a minimal set */
int test3(void); /* validate=>not enought */
int test4(void); /* validate=> too many */
int test5(void); /* get users list */


/*====[ Code ]===============================================================*/
gras_cfg_t *make_set(){
  gras_cfg_t *set=NULL; 
  gras_error_t errcode;

  TRYFAIL(gras_cfg_new(&set));
  gras_cfg_register_str(set,"hostname:1_to_1_string");
  gras_cfg_register_str(set,"user:1_to_10_string");
  gras_cfg_register_str(set,"speed:1_to_1_int");

  gras_cfg_set_parse(set,
		   "hostname:veloce "
		   "user:mquinson\nuser:ecaron\tuser:fsuter");
  return set;
}


/*----[ get users list ]-----------------------------------------------------*/
int test5()
{
  gras_dynar_t *dyn;
  char *str;
  int i;

  gras_cfg_t *set=make_set();
  gras_cfg_set_parse(set,"speed:42");
  gras_cfg_check(set);
  gras_cfg_get_dynar(set,"user",&dyn);
  printf("Count: %d; Options: \n",gras_dynar_length(dyn));
  gras_dynar_foreach(dyn,i,str) {
    printf("%s\n",str);
  }
  gras_cfg_free(&set);
  return 1;
}
 
int main(int argc, char **argv) {
  gras_error_t errcode;
  gras_cfg_t *set;
  int ival;
  
  gras_init_defaultlog(&argc,argv,"config.thresh=debug root.thresh=info");

  fprintf(stderr,"==== Alloc and free a config set.\n");
  set=make_set();
  gras_cfg_dump("test set","",set);
  gras_cfg_free(&set);


  fprintf(stderr,"==== Try to use an unregistered option (err msg expected).\n");
  set=make_set();
  TRYEXPECT(mismatch_error,gras_cfg_set_parse(set,"color:blue"));
  gras_cfg_free(&set);


  fprintf(stderr,
	  "\n==== Validation test (err msg about not enough values expected)\n");
  set=make_set();
  gras_cfg_check(set);
  gras_cfg_free(&set);

  fprintf(stderr,"\n==== Validation test (too many elements)\n");
  set=make_set();
  gras_cfg_set_parse(set,"hostname:toto:42");
  gras_cfg_set_parse(set,"speed:42 speed:24");
  gras_cfg_check(set);
  gras_cfg_get_int(set,"speed",&ival);
  printf("speed value: %d\n",ival);
  gras_cfg_free(&set);

  fprintf(stderr,"\n§§§§§§§§§ %s §§§§§§§§§\n§§§ Expected: %s\n",
	 "TEST5",
	 "Count: 3; Options:\\nmquinson\\necaron\\nfsuter");
  test5();

  gras_exit();
  return 0;
}

