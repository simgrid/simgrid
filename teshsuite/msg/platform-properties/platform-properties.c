/* Copyright (c) 2007-2019. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/msg.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(test, "Property test");

static void test_host(const char* hostname)
{
  msg_host_t thehost       = MSG_host_by_name(hostname);
  xbt_dict_t props         = MSG_host_get_properties(thehost);
  xbt_dict_cursor_t cursor = NULL;
  char* key;
  char* data;
  const char* noexist = "Unknown";
  const char* value;
  char exist[] = "Hdd";

  XBT_INFO("== Print the properties of the host '%s'", hostname);
  xbt_dict_foreach (props, cursor, key, data)
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
  MSG_host_set_property_value(thehost, exist, (char*)"250");

  /* Test if we have changed the value */
  value = MSG_host_get_property_value(thehost, exist);
  xbt_assert(value, "Property %s is undefined (where it should)", exist);
  xbt_assert(!strcmp(value, "250"), "Value of property %s is defined to %s (where it should be 250)", exist, value);
  XBT_INFO("   Property: %s old value: %s", exist, value);

  /* Restore the value for the next test */
  MSG_host_set_property_value(thehost, exist, (char*)"180");

  xbt_dict_free(&props);
}

static int alice(XBT_ATTRIB_UNUSED int argc, XBT_ATTRIB_UNUSED char* argv[])
{ /* Dump what we have on the current host */
  test_host("host1");
  return 0;
}

static int carole(XBT_ATTRIB_UNUSED int argc, XBT_ATTRIB_UNUSED char* argv[])
{                       /* Dump what we have on a remote host */
  MSG_process_sleep(1); // Wait for alice to be done with its experiment
  test_host("host1");
  return 0;
}

static int david(XBT_ATTRIB_UNUSED int argc, XBT_ATTRIB_UNUSED char* argv[])
{                       /* Dump what we have on a remote host */
  MSG_process_sleep(2); // Wait for alice and carole to be done with its experiment
  test_host("node-0.simgrid.org");
  return 0;
}

static int bob(XBT_ATTRIB_UNUSED int argc, XBT_ATTRIB_UNUSED char* argv[])
{
  /* this host also tests the properties of the AS*/
  msg_netzone_t root = MSG_zone_get_root();
  XBT_INFO("== Print the properties of the AS");
  XBT_INFO("   Process property: filename -> %s", MSG_zone_get_property_value(root, "filename"));
  XBT_INFO("   Process property: date -> %s", MSG_zone_get_property_value(root, "date"));
  XBT_INFO("   Process property: author -> %s", MSG_zone_get_property_value(root, "author"));

  /* Get the property list of current bob process */
  xbt_dict_t props         = MSG_process_get_properties(MSG_process_self());
  xbt_dict_cursor_t cursor = NULL;
  char* key;
  char* data;
  const char* noexist = "UnknownProcessProp";
  XBT_ATTRIB_UNUSED const char* value;

  XBT_INFO("== Print the properties of the process");
  xbt_dict_foreach (props, cursor, key, data)
    XBT_INFO("   Process property: %s -> %s", key, data);

  XBT_INFO("== Try to get a process property that does not exist");

  value = MSG_process_get_property_value(MSG_process_self(), noexist);
  xbt_assert(!value, "The property is defined (it shouldn't)");
  xbt_dict_free(&props);

  return 0;
}

int main(int argc, char* argv[])
{
  unsigned int i;
  msg_host_t host;

  MSG_init(&argc, argv);
  xbt_assert(argc > 2, "Usage: %s platform_file deployment_file\n"
                       "\tExample: %s msg_platform.xml msg_deployment.xml\n",
             argv[0], argv[0]);

  MSG_function_register("alice", alice);
  MSG_function_register("bob", bob);
  MSG_function_register("carole", carole);
  MSG_function_register("david", david);

  MSG_create_environment(argv[1]);

  XBT_INFO("There are %zu hosts in the environment", MSG_get_host_number());

  xbt_dynar_t hosts = MSG_hosts_as_dynar();
  xbt_dynar_foreach (hosts, i, host) {
    XBT_INFO("Host '%s' runs at %.0f flops/s", MSG_host_get_name(host), MSG_host_get_speed(host));
  }
  xbt_dynar_free(&hosts);

  MSG_launch_application(argv[2]);

  msg_error_t res = MSG_main();

  return res != MSG_OK;
}
