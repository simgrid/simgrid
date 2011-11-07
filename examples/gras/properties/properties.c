/* Copyright (c) 2007, 2008, 2009, 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "gras.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(test, "Simple Property example");

int alice(int argc, char *argv[]);
int bob(int argc, char *argv[]);

int alice(int argc, char *argv[])
{
  gras_init(&argc, argv);

  /* Get the properties */
  xbt_dict_t process_props = gras_process_properties();
  xbt_dict_t host_props = gras_os_host_properties();

  xbt_dict_cursor_t cursor = NULL;
  char *key, *data;
  _XBT_GNUC_UNUSED const char *value;

  /* Let the other process change the host props */
  gras_os_sleep(1);

  XBT_INFO("== Dump all the properties of current host");
  xbt_dict_foreach(host_props, cursor, key, data)
      XBT_INFO("  Host property: '%s' has value: '%s'", key, data);

  XBT_INFO("== Dump all the properties of alice");
  xbt_dict_foreach(process_props, cursor, key, data)
      if (!strncmp(key, "SG_TEST_", 8))
    XBT_INFO("  Process property: '%s' has value: '%s'", key, data);

  XBT_INFO("== Try to get a process property that does not exist");
  value = gras_process_property_value("Nonexisting");
  xbt_assert(!value, "nonexisting property exists!!");

  /* Modify an existing property. First check it exists */
  XBT_INFO("== Trying to modify a process property");
  value = gras_process_property_value("new prop");
  xbt_assert(!value, "Property 'new prop' exists before I add it!");
  xbt_dict_set(process_props, "new prop", xbt_strdup("new value"),
               xbt_free_f);

  /* Test if we have changed the value */
  value = gras_process_property_value("new prop");
  xbt_assert(!strcmp(value, "new value"),
              "New property does have the value I've set ('%s' != 'new value')",
              value);

  gras_exit();
  return 0;
}

int bob(int argc, char *argv[])
{
  gras_init(&argc, argv);

  /* Get the properties */
  xbt_dict_t host_props = gras_os_host_properties();
  xbt_dict_cursor_t cursor = NULL;
  char *key, *data;
  _XBT_GNUC_UNUSED const char *value;

  XBT_INFO("== Dump all the properties of host1");
  xbt_dict_foreach(host_props, cursor, key, data)
      XBT_INFO("  Host property: '%s' has value: '%s'", key, data);

  XBT_INFO("== Try to get a property that does not exist");
  value = gras_os_host_property_value("non existing key");
  xbt_assert(value == NULL,
              "The key 'non existing key' exists (with value '%s')!!",
              value);

  XBT_INFO
      ("== Set a host property that alice will try to retrieve in SG (from bob->hello)");
  xbt_dict_set(host_props, "from bob", xbt_strdup("hello"), xbt_free_f);

  XBT_INFO("== Dump all the properties of host1 again to check the addition");
  xbt_dict_foreach(host_props, cursor, key, data)
      XBT_INFO("  Host property: '%s' has value: '%s'", key, data);

  gras_os_sleep(1);             /* KILLME once bug on empty main is solved */
  gras_exit();
  return 0;
}
