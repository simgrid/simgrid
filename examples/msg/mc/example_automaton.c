#include "xbt/automaton.h"
#include "xbt/automatonparse_promela.h"
#include "example_automaton.h"
#include "msg/msg.h"
#include "mc/mc.h"

#include "y.tab.c"

#define N 3

XBT_LOG_NEW_DEFAULT_CATEGORY(example, "Example with automaton");

extern xbt_automaton_t automaton;


int p=1;
int r=0;
int q=1;
int e=0;
int d=1;


int predP(){
  return p;
}

int predR(){
  return r;
}

int predQ(){
  return q;
}


int predD(){
  return d;
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
    e=(e+1)%2;
    //d=(d+1)%2;
    //XBT_INFO("r (server) = %d", r);
     
  }
  MC_assert_pair(atoi(MSG_task_get_name(task)) == 3);

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
  
  //r=(r+1)%3;
  //XBT_INFO("r (client) = %d", r);
  
  return 0;
}



int main(int argc, char **argv){
  init();
  yyparse();
  automaton = get_automaton();
  xbt_propositional_symbol_t ps = xbt_new_propositional_symbol(automaton,"p", &predP); 
  ps = xbt_new_propositional_symbol(automaton,"q", &predQ); 
  ps = xbt_new_propositional_symbol(automaton,"r", &predR); 
  ps = xbt_new_propositional_symbol(automaton,"e", &predE);
  ps = xbt_new_propositional_symbol(automaton,"d", &predD);
      
  //display_automaton();

  MSG_global_init(&argc, argv);

  MSG_create_environment("platform.xml");

  MSG_function_register("server", server);

  MSG_function_register("client", client);

  MSG_launch_application("deploy_bugged1.xml"); 
  
  //XBT_INFO("r=%d", r);
  
  MSG_main_with_automaton(automaton);

  MSG_clean();

  return 0;

}
