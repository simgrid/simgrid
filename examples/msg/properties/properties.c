/* Copyright (c) 2007-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/msg.h"

/** @addtogroup MSG_examples
 * 
 * - <b>properties/msg_prop.c</b> Attaching arbitrary informations to host, processes and such, and retrieving them
 *   with @ref MSG_host_get_properties, @ref MSG_host_get_property_value, @ref MSG_process_get_properties and
 *   @ref MSG_process_get_property_value. Also make sure to read the platform and deployment XML files to see how to
 *   declare these data.
 */

XBT_LOG_NEW_DEFAULT_CATEGORY(test, "Property test");

static void test_host(const char*hostname) 
{
  msg_host_t thehost = MSG_host_by_name(hostname);
  xbt_dict_t props = MSG_host_get_properties(thehost);
  xbt_dict_cursor_t cursor = NULL;
  char *key, *data;
  const char *noexist = "Unknown";
  const char *value;
  char exist[] = "Hdd";

  XBT_INFO("== Print the properties of the host '%s'", hostname);
  xbt_dict_foreach(props, cursor, key, data)
    XBT_INFO("  Host property: '%s' -> '%s'", key, data);

  XBT_INFO("== Try to get a host property that does not exist");
  value = MSG_host_get_property_value(thehost, noexist);
  xbt_assert(!value, "The key exists (it's not supposed to)");

  XBT_INFO("== Try to get a host property that does exist");
  value = MSG_host_get_property_value(thehost, exist);
  xbt_assert(value, "\tProperty %s is undefined (where it should)", exist);
  xbt_assert(!strcmp(value, "180"), "\tValue of property %s is defined to %s (where it should be 180)", exist, value);
  XBT_INFO("   Property: %s old value: %s", exist, value);

  XBT_INFO("== Trying to modify a host property");
  MSG_host_set_property_value(thehost, exist, xbt_strdup("250"), xbt_free_f);

  /* Test if we have changed the value */
  value = MSG_host_get_property_value(thehost, exist);
  xbt_assert(value, "Property %s is undefined (where it should)", exist);
  xbt_assert(!strcmp(value, "250"), "Value of property %s is defined to %s (where it should be 250)", exist, value);
  XBT_INFO("   Property: %s old value: %s", exist, value);

  /* Restore the value for the next test */
  MSG_host_set_property_value(thehost, exist, xbt_strdup("180"), xbt_free_f);
}

static int alice(int argc, char *argv[]) { /* Dump what we have on the current host */
  test_host("host1");
  return 0;
}

static int carole(int argc, char *argv[]) {/* Dump what we have on a remote host */
  MSG_process_sleep(1); // Wait for alice to be done with its experiment
  test_host("host1");
  return 0;
}

static int david(int argc, char *argv[]) {/* Dump what we have on a remote host */
  MSG_process_sleep(2); // Wait for alice and carole to be done with its experiment
  test_host("node-0.acme.org");
  return 0;
}

static int bob(int argc, char *argv[])
{
  /* Get the property list of current bob process */
  xbt_dict_t props = MSG_process_get_properties(MSG_process_self());
  xbt_dict_cursor_t cursor = NULL;
  char *key, *data;
  const char *noexist = "UnknownProcessProp";
  XBT_ATTRIB_UNUSED const char *value;

  XBT_INFO("== Print the properties of the process");
  xbt_dict_foreach(props, cursor, key, data)
    XBT_INFO("   Process property: %s -> %s", key, data);

  XBT_INFO("== Try to get a process property that does not exist");

  value = MSG_process_get_property_value(MSG_process_self(), noexist);
  xbt_assert(!value, "The property is defined (it shouldnt)");

  return 0;
}

int main(int argc, char *argv[])
{
  msg_error_t res = MSG_OK;
  unsigned int i;
  xbt_dynar_t hosts;
  msg_host_t host;

  MSG_init(&argc, argv);
  xbt_assert(argc > 2, "Usage: %s platform_file deployment_file\n"
             "\tExample: %s msg_platform.xml msg_deployment.xml\n", argv[0], argv[0]);

  MSG_function_register("alice", alice);
  MSG_function_register("bob", bob);
  MSG_function_register("carole", carole);
  MSG_function_register("david", david);

  MSG_create_environment(argv[1]);

  XBT_INFO("There are %d hosts in the environment", MSG_get_host_number());

  hosts = MSG_hosts_as_dynar();

  xbt_dynar_foreach(hosts, i, host){
    XBT_INFO("Host '%s' runs at %.0f flops/s",MSG_host_get_name(host), MSG_host_get_speed(host));
  }

  MSG_launch_application(argv[2]);

  res = MSG_main();

  xbt_dynar_free(&hosts);

  return res!=MSG_OK;
}
