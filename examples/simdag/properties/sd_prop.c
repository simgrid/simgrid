/* 	$Id$	 */

#include <stdio.h>
#include <stdlib.h>
#include "simdag/simdag.h"
#include "xbt/ex.h"
#include "xbt/log.h"
#include "xbt/dynar.h"
#include "xbt/dict.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(test, "Property test");

int main(int argc, char **argv)
{
  int i;
  const SD_workstation_t *workstations;
  SD_workstation_t w1;
  SD_workstation_t w2;
  const char *name1;
  const char *name2;
  xbt_dict_t props;
  xbt_dict_cursor_t cursor = NULL;
  char *key, *data;
  char noexist[] = "NoProp";
  const char *value;
  char exist[] = "SG_TEST_Hdd";

  const SD_link_t *route;
  int route_size;

  /* initialisation of SD */
  SD_init(&argc, argv);
  if (argc < 2) {
    INFO1("Usage: %s platform_file", argv[0]);
    INFO1("example: %s sd_platform.xml", argv[0]);
    exit(1);
  }
  SD_create_environment(argv[1]);

  /* init of platform elements */
  workstations = SD_workstation_get_list();
  w1 = workstations[0];
  w2 = workstations[1];
  SD_workstation_set_access_mode(w2, SD_WORKSTATION_SEQUENTIAL_ACCESS);
  name1 = SD_workstation_get_name(w1);
  name2 = SD_workstation_get_name(w2);


  /* The host properties can be retrived from all interfaces */

  INFO1("Property list for workstation %s", name1);
  /* Get the property list of the workstation 1 */
  props = SD_workstation_get_properties(w1);


  /* Trying to set a new property */
  xbt_dict_set(props, xbt_strdup("NewProp"), strdup("newValue"), free);

  /* Print the properties of the workstation 1 */
  xbt_dict_foreach(props, cursor, key, data) {
    INFO2("\tProperty: %s has value: %s", key, data);
  }

  /* Try to get a property that does not exist */

  value = SD_workstation_get_property_value(w1, noexist);
  if (value == NULL)
    INFO1("\tProperty: %s is undefined", noexist);
  else
    INFO2("\tProperty: %s has value: %s", noexist, value);


  INFO1("Property list for workstation %s", name2);
  /* Get the property list of the workstation 2 */
  props = SD_workstation_get_properties(w2);
  cursor = NULL;

  /* Print the properties of the workstation 2 */
  xbt_dict_foreach(props, cursor, key, data) {
    INFO2("\tProperty: %s on host: %s", key, data);
  }

  /* Modify an existing property test. First check it exists */
  INFO0("Modify an existing property");

  value = SD_workstation_get_property_value(w2, exist);
  if (value == NULL)
    INFO1("\tProperty: %s is undefined", exist);
  else {
    INFO2("\tProperty: %s old value: %s", exist, value);
    xbt_dict_set(props, exist, strdup("250"), free);
  }

  /* Test if we have changed the value */
  value = SD_workstation_get_property_value(w2, exist);
  if (value == NULL)
    INFO1("\tProperty: %s is undefined", exist);
  else
    INFO2("\tProperty: %s new value: %s", exist, value);

  /* NOTE: The link properties can be retrieved only from the SimDag interface */
  route = SD_route_get_list(w1, w2);
  route_size = SD_route_get_size(w1, w2);
  for (i = 0; i < route_size; i++) {
    xbt_dict_cursor_t cursor = NULL;
    char *key, *data;
    char noexist1[] = "Other";
    props = SD_link_get_properties(route[i]);


    /* Print the properties of the current link */
    xbt_dict_foreach(props, cursor, key, data) {
      INFO3("\tLink %s property: %s has value: %s",
            SD_link_get_name(route[i]), key, data);

      /* Try to get a property that does not exist */

      value = SD_link_get_property_value(route[i], noexist1);
      if (value == NULL)
        INFO2("\tProperty: %s for link %s is undefined", noexist,
              SD_link_get_name(route[i]));
      else
        INFO3("\tLink %s property: %s has value: %s",
              SD_link_get_name(route[i]), noexist, value);
    }

  }

  SD_exit();
  return 0;
}
