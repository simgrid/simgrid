#include "xbt/automaton.h"
#include "xbt/automatonparse_promela.h"
#include "example_automaton.h"
#include "msg/msg.h"
#include "mc/mc.h"

#include "y.tab.c"

#define N 5

XBT_LOG_NEW_DEFAULT_CATEGORY(example, "Example with automaton");

extern xbt_automaton_t automaton;


int d=1;
int r=1;
int e=1;

int predD(){
  return d;
}

int predR(){
  return r;
}

int predE(){
  return e;
}

int server(int argc, char *argv[])
{
  m_task_t task = NULL;
  int count = 0;
  while (count < N) {
    if (task) {
      MSG_task_destroy(task);
      task = NULL;
    }
    MSG_task_receive(&task, "mymailbox");
    count++;
    r=(r+1)%4;
     
  }
  MC_assert(atoi(MSG_task_get_name(task)) == 3);

  XBT_INFO("OK");
  return 0;
}

int client(int argc, char *argv[])
{

  m_task_t task =
      MSG_task_create(argv[1], 0 /*comp cost */ , 10000 /*comm size */ ,
                      NULL /*arbitrary data */ );

  MSG_task_send(task, "mymailbox");
  
  XBT_INFO("Sent!");
   r=(r+1)%4;
  return 0;
}



int main(int argc, char **argv){
  init();
  yyparse();
  automaton = get_automaton();
  xbt_propositional_symbol_t ps = xbt_new_propositional_symbol(automaton,"d", &predD); 
  ps = xbt_new_propositional_symbol(automaton,"e", &predE); 
  ps = xbt_new_propositional_symbol(automaton,"r", &predR); 
  
  //display_automaton();

  MSG_global_init(&argc, argv);

  MSG_create_environment("platform.xml");

  MSG_function_register("server", server);

  MSG_function_register("client", client);

  MSG_launch_application("deploy_bugged1.xml");

  MSG_main_with_automaton(automaton);

  MSG_clean();

  return 0;

}
