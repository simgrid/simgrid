#include <ruby.h>
#include <stdio.h>
#include <stdlib.h>





VALUE oProcess;

void r_init()
{
  
  ruby_init();
  ruby_init_loadpath();
  rb_require("RubyProcess.rb");
  oProcess = rb_funcall3(rb_const_get(rb_cObject, rb_intern("RbProcess")),  rb_intern("new"), 0, 0);
  
}


VALUE getID(VALUE current)
{
  
   
   
   return rb_funcall(current,rb_intern("getID"),0);
  
  
}

VALUE isAlive(VALUE current)
{
  
   
   
   return rb_funcall(current,rb_intern("alive?"),0);
  
  
}
  
void List(VALUE current)
{
  
  rb_funcall(current,rb_intern("processList"),0);
  
}


int main(int argc, char ** argv) 
{

  
  
  
  r_init();
  VALUE test = isAlive(oProcess);
  
  if (TYPE(test) == T_TRUE)
    printf("Aliiiive\n");
  
  getID(oProcess);
  List(oProcess);
   
  
  
  
  //keep...
  /*VALUE current = rb_thread_current(); // main One
  test = isAlive(current);
  
  if(TYPE(test) == T_TRUE)
    printf("The Current Thread is Alive\n");*/
    
  
  
 /*application_handler_on_start_document();
 application_handler_on_end_document();
 application_handler_on_begin_process();
 application_handler_property();
 application_handler_on_process_arg();

 application_handler_on_end_process();
 
 */
   
}

// compile command : gcc -I/usr/lib/ruby/1.8/i486-linux essai2.c  -o essai2 -lruby1.8 

// gcc -o libProcess.so  -I/usr/lib/ruby/1.8/i486-linux  -lruby1.8 -lsimgrid  -shared rb_msg_process.c

