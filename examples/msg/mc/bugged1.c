#include <msg/msg.h>
#include <mc/modelchecker.h>
#define N 3

XBT_LOG_NEW_DEFAULT_CATEGORY(example,"this example");

int server(int argc,char *argv[]);
int client(int argc,char *argv[]);

int server(int argc,char *argv[]) {
 m_task_t task = NULL;
 int count = 0;
 while(count < N){
   if(task){
     MSG_task_destroy(task);
     task = NULL;
   }
   MSG_task_receive(&task,"mymailbox");
   count++;
 }
 MC_assert(atoi(MSG_task_get_name(task)) == 3);

  INFO0("OK");
 return 0;
}

int client(int argc,char *argv[]) {

 m_task_t task = MSG_task_create(argv[1], 0/*comp cost*/, 10000/*comm size*/, NULL /*arbitrary data*/);

 MSG_task_send(task,"mymailbox");

 INFO0("Sent!");
 return 0;
}

int main(int argc,char*argv[]) {

 MSG_global_init(&argc,argv);

 MSG_create_environment("platform.xml");

 MSG_function_register("server", server);

 MSG_function_register("client", client);

 MSG_launch_application("deploy.xml");

 MSG_main();

 return 0;
  
}


