/* 	$Id$	 */

#include "gras.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(test,"Simple Property example");

int master(int argc, char *argv[]);
int slave(int argc, char *argv[]);


int slave(int argc, char *argv[]) {
  gras_init(&argc,argv);

  /* Get the properties */
  xbt_dict_t props = gras_process_properties();
  xbt_dict_cursor_t cursor = NULL;
  char *key,*data;

  /* Print the properties of the workstation 1 */
  xbt_dict_foreach(props,cursor,key,data) {
    INFO2("Process property: %s has value: %s",key,data);
  }
 
  /* Try to get a property that does not exist */
  const char *noexist="Nonexisting";
  const char *value = gras_process_property_value(noexist);
  if ( value == NULL) 
    INFO1("Process property: %s is undefined", noexist);
  else
    INFO2("Process property: %s has value: %s", noexist, value);
 
   /* Modify an existing property. First check it exists */
    INFO0("Trying to modify a process property");
    const char *exist="otherprop";
    value = gras_process_property_value(exist);
    if (value == NULL) 
      INFO1("\tProperty: %s is undefined", exist);
    else {
      INFO2("\tProperty: %s old value: %s", exist, value);
      xbt_dict_set(props, exist, xbt_strdup("newValue"), free);  
    }
 
    /* Test if we have changed the value */
    value = gras_process_property_value(exist);
    if (value == NULL) 
      INFO1("\tProperty: %s is undefined", exist);
    else
      INFO2("\tProperty: %s new value: %s", exist, value);

  gras_os_sleep(1);
  gras_exit();
  return 0;
}

int master(int argc, char *argv[]) {
  gras_init(&argc,argv);

  /* Get the properties */
  xbt_dict_t props = gras_os_host_properties();
  xbt_dict_cursor_t cursor = NULL;
  char *key,*data;

  /* Print the properties of the workstation 1 */
  xbt_dict_foreach(props,cursor,key,data) {
    INFO2("Host property: %s has value: %s",key,data);
  }
 
  /* Try to get a property that does not exist */
  const char *noexist="Nonexisting";
  const char *value = gras_os_host_property_value(noexist);
  xbt_assert2(value == NULL, "Host property: %s has value: %s", noexist, value);
  
  gras_os_sleep(1);
  gras_exit();
  return 0;
}
