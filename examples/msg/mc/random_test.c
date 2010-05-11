#include <msg/msg.h>
#include <mc/modelchecker.h>

XBT_LOG_NEW_DEFAULT_CATEGORY(random_test,"Random Test");

int server(int argc,char *argv[]);

int server(int argc,char *argv[])
{
  int val;
  val = MC_random(3,6);
  INFO1("val=%d", val);
  INFO0("OK");
  return 0;
}

int main(int argc,char*argv[]) 
{
  MSG_global_init(&argc,argv);

  MSG_create_environment("platform.xml");

  MSG_function_register("server", server);

//  MSG_function_register("client", client);

  MSG_launch_application("deploy_random_test.xml");

  MSG_main();

  return 0;
}
