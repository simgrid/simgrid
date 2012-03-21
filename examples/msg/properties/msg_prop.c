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

/** @addtogroup MSG_examples
 * 
 * - <b>properties/msg_prop.c</b> Attaching arbitrary informations to
 *   host, processes and such, and retrieving them with @ref
 *   MSG_host_get_properties, @ref MSG_host_get_property_value, @ref
 *   MSG_process_get_properties and @ref
 *   MSG_process_get_property_value. Also make sure to read the
 *   platform and deployment XML files to see how to declare these
 *   data.
 * 
 */

XBT_LOG_NEW_DEFAULT_CATEGORY(test, "Property test");

int alice(int argc, char *argv[]);
int bob(int argc, char *argv[]);
int carole(int argc, char *argv[]);
int forwarder(int argc, char *argv[]);
MSG_error_t test_all(const char *platform_file,
                     const char *application_file);

static void test_host(const char*hostname) 
{
  m_host_t thehost = MSG_get_host_by_name(hostname);
  xbt_dict_t props = MSG_host_get_properties(thehost);
  xbt_dict_cursor_t cursor = NULL;
  char *key, *data;
  const char *noexist = "Unknown";
  const char *value;
  char exist[] = "SG_TEST_Hdd";

  XBT_INFO("== Print the properties of the host");
  xbt_dict_foreach(props, cursor, key, data)
      XBT_INFO("  Host property: '%s' -> '%s'", key, data);

  XBT_INFO("== Try to get a host property that does not exist");
  value = MSG_host_get_property_value(thehost, noexist);
  xbt_assert(!value, "The key exists (it's not supposed to)");

  XBT_INFO("== Try to get a host property that does exist");
  value = MSG_host_get_property_value(thehost, exist);
  xbt_assert(value, "\tProperty %s is undefined (where it should)",
              exist);
  xbt_assert(!strcmp(value, "180"),
              "\tValue of property %s is defined to %s (where it should be 180)",
              exist, value);
  XBT_INFO("   Property: %s old value: %s", exist, value);

  XBT_INFO("== Trying to modify a host property");
  xbt_dict_set(props, exist, xbt_strdup("250"), NULL);

  /* Test if we have changed the value */
  value = MSG_host_get_property_value(thehost, exist);
  xbt_assert(value, "Property %s is undefined (where it should)", exist);
  xbt_assert(!strcmp(value, "250"),
              "Value of property %s is defined to %s (where it should be 250)",
              exist, value);
  XBT_INFO("   Property: %s old value: %s", exist, value);
   
  /* Restore the value for the next test */
  xbt_dict_set(props, exist, xbt_strdup("180"), NULL);
}

int alice(int argc, char *argv[]) { /* Dump what we have on the current host */
  test_host("host1");
  return 0;
}
int carole(int argc, char *argv[]) {/* Dump what we have on a remote host */
  MSG_process_sleep(1); // Wait for alice to be done with its experiment
  test_host("host1");
  return 0;
}

int bob(int argc, char *argv[])
{
  /* Get the property list of current bob process */
  xbt_dict_t props = MSG_process_get_properties(MSG_process_self());
  xbt_dict_cursor_t cursor = NULL;
  char *key, *data;
  const char *noexist = "UnknownProcessProp";
  _XBT_GNUC_UNUSED const char *value;

  XBT_INFO("== Print the properties of the process");
  xbt_dict_foreach(props, cursor, key, data)
      XBT_INFO("   Process property: %s -> %s", key, data);

  XBT_INFO("== Try to get a process property that does not exist");

  value = MSG_process_get_property_value(MSG_process_self(), noexist);
  xbt_assert(!value, "The property is defined (it shouldnt)");

  return 0;
}

/** Test function */
MSG_error_t test_all(const char *platform_file,
                     const char *application_file)
{
  MSG_function_register("alice", alice);
  MSG_function_register("bob", bob);
  MSG_function_register("carole", carole);

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
