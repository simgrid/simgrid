#include "msg/msg.h"
#include "xbt/log.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(storage,"Messages specific for this simulation");

void storage_info(msg_host_t host);
void display_storage_properties(msg_storage_t storage);
int client(int argc, char *argv[]);
int server(int argc, char *argv[]);


int client(int argc, char *argv[])
{
  storage_info(MSG_host_self());
  return 1;
}

int server(int argc, char *argv[])
{
  //display_storage_info();
  return 1;
}

void storage_info(msg_host_t host){

  const char* host_name = MSG_host_get_name(host);
  XBT_INFO("** Storage info on %s:", host_name);

  xbt_dict_cursor_t cursor = NULL;
  char* mount_name;
  char* storage_name;
  msg_storage_t storage;

  xbt_dict_t storage_list = MSG_host_get_storage_list(MSG_host_self());

  xbt_dict_foreach(storage_list,cursor,mount_name,storage_name)
  {
    XBT_INFO("\tStorage mount name: %s", mount_name);

    size_t free_size = MSG_storage_get_free_size(mount_name);
    size_t used_size = MSG_storage_get_used_size(mount_name);

    XBT_INFO("\t\tFree size: %zu octets", free_size);
    XBT_INFO("\t\tUsed size: %zu octets", used_size);

    storage = MSG_storage_get_by_name(storage_name);
    display_storage_properties(storage);
  }
}

void display_storage_properties(msg_storage_t storage){
  xbt_dict_cursor_t cursor = NULL;
  char *key, *data;
  xbt_dict_t props = MSG_storage_get_properties(storage);
  if (props){
    XBT_INFO("\tProperties of mounted storage: %s", MSG_storage_get_name(storage));
    xbt_dict_foreach(props, cursor, key, data)
	  XBT_INFO("\t\t'%s' -> '%s'", key, data);
  }else{
	XBT_INFO("\t\tNo property attached.");
  }
}

int main(int argc, char *argv[])
{
  MSG_init(&argc, argv);

  /* Check the arguments */
  if (argc < 3) {
    printf("Usage: %s platform_file deployment_file \n", argv[0]);
    return -1;
  }

  const char *platform_file = argv[1];
  const char *deployment_file = argv[2];

  MSG_create_environment(platform_file);

  MSG_function_register("client", client);
  MSG_function_register("server", server);
  MSG_launch_application(deployment_file);

  msg_error_t res = MSG_main();

  if (res == MSG_OK)
    return 0;
  else
    return 1;
}
