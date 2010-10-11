/* Copyright (c) 2007, 2008, 2009, 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "msg/msg.h"            /* Yeah! If you want to use msg, you need to include msg/msg.h */
#include "xbt/sysdep.h"         /* calloc, printf */

/* Create a log channel to have nice outputs. */
#include "xbt/log.h"
#include "xbt/asserts.h"

#include <stdio.h>
#include <stdlib.h>

XBT_LOG_NEW_DEFAULT_CATEGORY(test, "Property test");

int alice(int argc, char *argv[]);
int bob(int argc, char *argv[]);
int forwarder(int argc, char *argv[]);
MSG_error_t test_all(const char *platform_file,
                     const char *application_file);

int alice(int argc, char *argv[])
{
  m_host_t host1 = MSG_get_host_by_name("host1");
  xbt_dict_t props = MSG_host_get_properties(host1);
  xbt_dict_cursor_t cursor = NULL;
  char *key, *data;
  const char *noexist = "Unknown";
  const char *value;
  char exist[] = "SG_TEST_Hdd";

  INFO0("== Print the properties of the host");
  xbt_dict_foreach(props, cursor, key, data)
      INFO2("  Host property: '%s' -> '%s'", key, data);

  INFO0("== Try to get a host property that does not exist");
  value = MSG_host_get_property_value(host1, noexist);
  xbt_assert0(!value, "The key exists (it's not supposed to)");

  INFO0("== Try to get a host property that does exist");
  value = MSG_host_get_property_value(host1, exist);
  xbt_assert1(value, "\tProperty %s is undefined (where it should)",
              exist);
  xbt_assert2(!strcmp(value, "180"),
              "\tValue of property %s is defined to %s (where it should be 180)",
              exist, value);
  INFO2("   Property: %s old value: %s", exist, value);

  INFO0("== Trying to modify a host property");
  xbt_dict_set(props, exist, xbt_strdup("250"), xbt_free_f);

  /* Test if we have changed the value */
  value = MSG_host_get_property_value(host1, exist);
  xbt_assert1(value, "Property %s is undefined (where it should)", exist);
  xbt_assert2(!strcmp(value, "250"),
              "Value of property %s is defined to %s (where it should be 250)",
              exist, value);
  INFO2("   Property: %s old value: %s", exist, value);

  return 0;
}

int bob(int argc, char *argv[])
{
  /* Get the property list of current bob process */
  xbt_dict_t props = MSG_process_get_properties(MSG_process_self());
  xbt_dict_cursor_t cursor = NULL;
  char *key, *data;
  const char *noexist = "UnknownProcessProp";
  const char *value;

  INFO0("== Print the properties of the process");
  xbt_dict_foreach(props, cursor, key, data)
      INFO2("   Process property: %s -> %s", key, data);

  INFO0("== Try to get a process property that does not exist");

  value = MSG_process_get_property_value(MSG_process_self(), noexist);
  xbt_assert0(!value, "The property is defined (it shouldnt)");

  return 0;
}

/** Test function */
MSG_error_t test_all(const char *platform_file,
                     const char *application_file)
{
  MSG_function_register("alice", alice);
  MSG_function_register("bob", bob);

  MSG_create_environment(platform_file);
  MSG_launch_application(application_file);

  return MSG_main();
}                               /* end_of_test_all */


/** Main function */
int main(int argc, char *argv[])
{
  MSG_error_t res = MSG_OK;

  MSG_global_init(&argc, argv);
  if (argc < 3) {
    printf("Usage: %s platform_file deployment_file\n", argv[0]);
    printf("example: %s msg_platform.xml msg_deployment.xml\n", argv[0]);
    exit(1);
  }
  res = test_all(argv[1], argv[2]);
  MSG_clean();

  if (res == MSG_OK)
    return 0;
  else
    return 1;
}                               /* end_of_main */
