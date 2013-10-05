#include "msg/msg.h"
#include "xbt/log.h"
#include "inttypes.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(storage,"Messages specific for this simulation");

void storage_info(msg_host_t host);
void display_storage_properties(msg_storage_t storage);
int hsm_put(const char *remote_host, const char *src, const char *dest);
sg_storage_size_t write_local_file(char *dest, sg_storage_size_t file_size);
sg_storage_size_t read_local_file(const char *src);
void display_storage_info(msg_host_t host);
void dump_storage_by_name(char *name);
void display_storage_content(msg_storage_t storage);
void get_set_storage_data(const char *storage_name);
int client(int argc, char *argv[]);
int server(int argc, char *argv[]);

void storage_info(msg_host_t host)
{
  const char* host_name = MSG_host_get_name(host);
  XBT_INFO("*** Storage info on %s ***:", host_name);

  xbt_dict_cursor_t cursor = NULL;
  char* mount_name;
  char* storage_name;
  msg_storage_t storage;

  xbt_dict_t storage_list = MSG_host_get_storage_list(MSG_host_self());

  xbt_dict_foreach(storage_list,cursor,mount_name,storage_name)
  {
	XBT_INFO("Storage name: %s, mount name: %s", storage_name, mount_name);

    sg_storage_size_t free_size = MSG_storage_get_free_size(mount_name);
    sg_storage_size_t used_size = MSG_storage_get_used_size(mount_name);

    XBT_INFO("Free size: %zu bytes", free_size);
    XBT_INFO("Used size: %zu bytes", used_size);

    storage = MSG_storage_get_by_name(storage_name);
    display_storage_properties(storage);
  }
}

void display_storage_properties(msg_storage_t storage){
  xbt_dict_cursor_t cursor = NULL;
  char *key, *data;
  xbt_dict_t props = MSG_storage_get_properties(storage);
  if (props){
    XBT_INFO("Properties of mounted storage: %s", MSG_storage_get_name(storage));
    xbt_dict_foreach(props, cursor, key, data)
	  XBT_INFO("'%s' -> '%s'", key, data);
  }else{
	XBT_INFO("No property attached.");
  }
}

// Read src file on local disk and send a put message to remote host (size of message = size of src file)
int hsm_put(const char *remote_host, const char *src, const char *dest){

  // Read local src file, and return the size that was actually read
  sg_storage_size_t read_size = read_local_file(src);

  // Send file
  XBT_INFO("%s sends %zu to %s",MSG_host_get_name(MSG_host_self()),read_size,remote_host);
  msg_task_t to_execute = MSG_task_create((const char*)"hsm_put", 0, (double) read_size, (void*)dest);
  MSG_task_send(to_execute, remote_host);

  return 1;
}

sg_storage_size_t write_local_file(char *dest, sg_storage_size_t file_size)
{
  sg_storage_size_t write;
  msg_file_t file = MSG_file_open("/sd1",dest, NULL);
  write = MSG_file_write(file, file_size);
  MSG_file_close(file);
  return write;
}

sg_storage_size_t read_local_file(const char *src)
{
  sg_storage_size_t read, file_size;
  msg_file_t file = MSG_file_open("/sd1",src, NULL);
  file_size = MSG_file_get_size(file);

  read = MSG_file_read(file, file_size);
  XBT_INFO("%s has read %zu on %s",MSG_host_get_name(MSG_host_self()),read,src);
  MSG_file_close(file);

  return read;
}

void display_storage_info(msg_host_t host)
{
  const char* host_name = MSG_host_get_name(host);
  XBT_INFO("*** Storage info of: %s ***", host_name);

  xbt_dict_cursor_t cursor = NULL;
  char* mount_name;
  char* storage_name;

  xbt_dict_t storage_list = MSG_host_get_storage_list(host);

  xbt_dict_foreach(storage_list,cursor,mount_name,storage_name)
  {
    dump_storage_by_name(storage_name);
  }
}

void dump_storage_by_name(char *name){
  XBT_INFO("*** Dump a storage element ***");
  msg_storage_t storage = MSG_storage_get_by_name(name);

  if(storage){
    display_storage_content(storage);
  }
  else{
    XBT_INFO("Unable to retrieve storage element by its name: %s.", name);
  }
}

void display_storage_content(msg_storage_t storage){
  XBT_INFO("Print the content of the storage element: %s",MSG_storage_get_name(storage));
  xbt_dict_cursor_t cursor = NULL;
  char *file;
  sg_storage_size_t size;
  xbt_dict_t content = MSG_storage_get_content(storage);
  if (content){
    xbt_dict_foreach(content, cursor, file, size)
    XBT_INFO("%s size: %" PRIu64 "bytes", file, size);
  } else {
    XBT_INFO("No content.");
  }
}

void get_set_storage_data(const char *storage_name){
	XBT_INFO("*** GET/SET DATA for storage element: %s ***",storage_name);
	msg_storage_t storage = MSG_storage_get_by_name(storage_name);
	char *data = MSG_storage_get_data(storage);
	XBT_INFO("Get data: '%s'", data);
	MSG_storage_set_data(storage,strdup("Some data"));
	data = MSG_storage_get_data(storage);
	XBT_INFO("Set and get data: '%s'", data);
}

int client(int argc, char *argv[])
{
  hsm_put("server","./doc/simgrid/examples/cxx/autoDestination/FinalizeTask.cxx","./scratch/toto.xml");
  hsm_put("server","./doc/simgrid/examples/cxx/autoDestination/autoDestination_deployment.xml","./scratch/titi.cxx");
  hsm_put("server","./doc/simgrid/examples/cxx/autoDestination/Slave.cxx","./scratch/tata.cxx");

  msg_task_t finalize = MSG_task_create("finalize", 0, 0, NULL);
  MSG_task_send(finalize, "server");

  get_set_storage_data("cdisk");
  display_storage_info(MSG_host_self());

  return 1;
}

int server(int argc, char *argv[])
{
  msg_task_t to_execute = NULL;
  _XBT_GNUC_UNUSED int res;

  display_storage_info(MSG_host_self());

  XBT_INFO("Server waiting for transfers");
  while(1){
    res = MSG_task_receive(&(to_execute), MSG_host_get_name(MSG_host_self()));
    xbt_assert(res == MSG_OK, "MSG_task_get failed");

    const char *task_name;
    task_name = MSG_task_get_name(to_execute);

    if (!strcmp(task_name, "finalize")) { // Shutdown ...
      MSG_task_destroy(to_execute);
      break;
    }
    else if(!strcmp(task_name,"hsm_put")){// Receive file to save
      // Write file on local disk
      char *dest = MSG_task_get_data(to_execute);
      sg_storage_size_t size_to_write = (sg_storage_size_t)MSG_task_get_data_size(to_execute);
      write_local_file(dest, size_to_write);
	}

    MSG_task_destroy(to_execute);
    to_execute = NULL;
  }

  display_storage_info(MSG_host_self());
  return 1;
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
  XBT_INFO("Simulated time: %g", MSG_get_clock());

  if (res == MSG_OK)
    return 0;
  else
    return 1;
}
