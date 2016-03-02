/* Copyright (c) 2007-2016. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/simdag.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(test, "Property test");

int main(int argc, char **argv)
{
  xbt_dict_cursor_t cursor = NULL;
  char *key, *data;
  char noexist[] = "NoProp";
  const char *value;
  char exist[] = "Hdd";

  /* SD initialization */
  SD_init(&argc, argv);
  xbt_assert(argc > 1, "Usage: %s platform_file\n\tExample: %s ../../platforms/prop.xml", argv[0], argv[0]);

  SD_create_environment(argv[1]);

  /* init of platform elements */
  sg_host_t h1 = sg_host_by_name("host1");
  sg_host_t h2 = sg_host_by_name("host2");
  const char *name1 = sg_host_get_name(h1);
  const char *name2 = sg_host_get_name(h2);

  /* Get the property list of 'host1' */
  XBT_INFO("Property list for host %s", name1);
  xbt_dict_t props = sg_host_get_properties(h1);

  /* Trying to set a new property */
  xbt_dict_set(props, "NewProp", strdup("newValue"), xbt_free_f);

  /* Print the properties of 'host1' */
  xbt_dict_foreach(props, cursor, key, data) {
    XBT_INFO("\tProperty: %s has value: %s", key, data);
  }

  /* Try to get a property that does not exist */
  value = sg_host_get_property_value(h1, noexist);
  XBT_INFO("\tProperty: %s has value: %s", noexist, value?value:"Undefined (NULL)");

  /* Get the property list of 'host2' */
  XBT_INFO("Property list for host %s", name2);
  props = sg_host_get_properties(h2);

  /* Print the properties of 'host2' */
  xbt_dict_foreach(props, cursor, key, data) {
    XBT_INFO("\tProperty: %s on host: %s", key, data);
  }

  /* Modify an existing property test. First check it exists */
  XBT_INFO("Modify an existing property");

  value = sg_host_get_property_value(h2, exist);
  if (value == NULL)
    XBT_INFO("\tProperty: %s is undefined", exist);
  else {
    XBT_INFO("\tProperty: %s old value: %s", exist, value);
    xbt_dict_set(props, exist, strdup("250"), xbt_free_f);
  }

  /* Test if we have changed the value */
  value = sg_host_get_property_value(h2, exist);
  XBT_INFO("\tProperty: %s new value: %s", exist, value?value:"Undefined (NULL)");

  /* Test if properties are displayed by sg_host_dump */
  sg_host_dump(h2);

  SD_exit();
  return 0;
}
