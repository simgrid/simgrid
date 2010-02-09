#include "rb_msg.h"
#include "msg/msg.h"
#include "msg/datatypes.h"
#include "xbt/sysdep.h"        
#include "xbt/log.h"
#include "xbt/asserts.h"
#include "surf/surfxml_parse.h"


#include "rb_msg_task.c"
#include "rb_msg_host.c"
#include "rb_msg_process.c"
#include "rb_application_handler.c"



//Init Msg_Init From Ruby


static void msg_init(VALUE Class,VALUE args)
{
  
  char **argv=NULL;  
  const char *tmp;
  int argc,type,i;
  VALUE *ptr ;
  
  
  
  // Testing The Args Type
  type =  TYPE(args);
  
  if (type != T_ARRAY )
  {
    rb_raise(rb_eRuntimeError,"Argh!! Bad Arguments to msg_init");
    return;
  }
    
  ptr= RARRAY(args)->ptr;
  argc= RARRAY(args)->len;
  
//   Create C Array to Hold Data_Get_Struct 
  argv = xbt_new0(char *, argc);  // argc or argc +1
  
  argv[0] = strdup("ruby");
  
  
  for (i=0;i<argc;i++)
  {
   VALUE value = ptr[i];
   type = TYPE(value);
//   if (type == T_STRING)
   tmp = RSTRING(value)->ptr;
   argv[i+1] = strdup(tmp);
    
  }
  
  // Calling C Msg_Init Method
  MSG_global_init(&argc,argv);
  MSG_set_channel_number(10); // Okey !! Okey !! This Must Be Fixed Dynamiclly , But Later ;)
  SIMIX_context_select_factory("ruby"); 
  
  // Free Stuffs 
  for (i=0;i<argc;i++)
   free(argv[i]) ;
  
  free (argv);
 
  printf("Msg Init...Done\n");
  return;
}




//Init Msg_Run From Ruby
static void msg_run(VALUE class)
{
  
 xbt_fifo_item_t item = NULL;
 m_host_t host = NULL;
 VALUE rbHost;
 
 // Let's Run
 if (MSG_OK != MSG_main()){
   
   rb_raise(rb_eRuntimeError,"MSG_main() failed");
   
 }
  DEBUG
    ("MSG_main finished. Bail out before cleanup since there is a bug in this part.");
     /* Cleanup Ruby hosts */
   DEBUG("Clean Ruby World");
   xbt_fifo_foreach(msg_global->host, item, host, m_host_t) {
   
     //rbHost = (VALUE)host->data;// ??!!
    
      }
    
   if (MSG_OK != MSG_clean()){
    
     rb_raise(rb_eRuntimeError,"MSG_clean() failed");
   }
    
    return;
    
}




// Create Environment
static void msg_createEnvironment(VALUE class,VALUE plateformFile)
{
 
  
  int type = TYPE(plateformFile);
  
  if ( type != T_STRING )
    rb_raise(rb_eRuntimeError,"Bad Argument's Type");
   
 const char * platform =  RSTRING(plateformFile)->ptr;
 
 MSG_create_environment(platform);
 
 return; 
  
}


// deploy Application

static void msg_deployApplication(VALUE class,VALUE deploymentFile )
{
  
    int type = TYPE(deploymentFile);
  
    if ( type != T_STRING )
     rb_raise(rb_eRuntimeError,"Bad Argument's Type");
  
    const char *dep_file = RSTRING(deploymentFile)->ptr;
    
    
    surf_parse_reset_parser();
    
    surfxml_add_callback(STag_surfxml_process_cb_list,
			 application_handler_on_begin_process);
    
    surfxml_add_callback(STag_surfxml_argument_cb_list,
			 application_handler_on_process_arg);

    surfxml_add_callback(STag_surfxml_prop_cb_list,
			 application_handler_on_property);
			 
    surfxml_add_callback(STag_surfxml_process_cb_list,
			 application_handler_on_end_process);
			 
    surf_parse_open(dep_file);
			 
    application_handler_on_start_document();
    
    if(surf_parse())
      rb_raise(rb_eRuntimeError,"surf_parse() failed");
    
    surf_parse_close();
    
    application_handler_on_end_document();
    
  
}



static void msg_registerFunction(VALUE class,VALUE function_name,VALUE code)
{
  
 char * fct_name = RSTRING(function_name)->ptr;
// xbt_main_func_t fct_code
  
}
// INFO
static void msg_info(VALUE class,VALUE msg)
{
 const char *s = RSTRING(msg)->ptr;
 INFO("%s",s);
}


// Get Clock
static VALUE msg_get_clock(VALUE class)
{
 
  return DBL2NUM(MSG_get_clock());
  
}

//pajeOutput
static void msg_paje_out(VALUE class,VALUE pajeFile)
{
  const char *pfile = RSTRING(pajeFile)->ptr;
  MSG_paje_output(pfile);
  
}

/*****************************************************************************************************************

Wrapping MSG module and its Class ( Task,Host) & Methods ( Process's method...ect)
To Ruby 

 the part after "Init_" is the name of the C extension specified in extconf.rb , not the name of C source file
 
*****************************************************************************************************************/

void Init_msg()
{
  
  // Modules
   rb_msg = rb_define_module("MSG");
    
   //Associated Environment Methods!
   rb_define_method(rb_msg,"init",msg_init,1);
   rb_define_method(rb_msg,"run",msg_run,0);
   rb_define_method(rb_msg,"createEnvironment",msg_createEnvironment,1);
   rb_define_method(rb_msg,"deployApplication",msg_deployApplication,1);
   rb_define_method(rb_msg,"info",msg_info,1);
   rb_define_method(rb_msg,"getClock",msg_get_clock,0);
   rb_define_method(rb_msg,"pajeOutput",msg_paje_out,1);
   
   // Associated Process Methods
//    rb_define_method(rb_msg,"processCreate",processCreate,2);
   rb_define_method(rb_msg,"processSuspend",processSuspend,1);
   rb_define_method(rb_msg,"processResume",processResume,1);
   rb_define_method(rb_msg,"processIsSuspend",processIsSuspend,1);
   rb_define_method(rb_msg,"processKill",processKill,1);
   rb_define_method(rb_msg,"processGetHost",processGetHost,1);
     
   //Classes
   rb_task = rb_define_class_under(rb_msg,"Task",rb_cObject);
   rb_host = rb_define_class_under(rb_msg,"Host",rb_cObject);
   
   //Task Methods
   rb_define_module_function(rb_task,"new",task_new,3);
   rb_define_module_function(rb_task,"compSize",task_comp,1);
   rb_define_module_function(rb_task,"name",task_name,1);
   rb_define_module_function(rb_task,"execute",task_execute,1);
   rb_define_module_function(rb_task,"send",task_send,2);
   rb_define_module_function(rb_task,"receive",task_receive,1);
   rb_define_module_function(rb_task,"receive2",task_receive2,2);
   rb_define_module_function(rb_task,"sender",task_sender,1);
   rb_define_module_function(rb_task,"source",task_source,1);
   rb_define_module_function(rb_task,"listen",task_listen,2);
   rb_define_module_function(rb_task,"listenFromHost",task_listen_host,3);
    
   //Host Methods
   rb_define_module_function(rb_host,"getByName",host_get_by_name,1);
   rb_define_module_function(rb_host,"name",host_name,1);
   rb_define_module_function(rb_host,"speed",host_speed,1);
   rb_define_module_function(rb_host,"number",host_number,0);
   rb_define_module_function(rb_host,"setData",host_set_data,2);
   rb_define_module_function(rb_host,"getData",host_get_data,1);
//    rb_define_module_function(rb_host,"hasData",host_has_data,1);
   rb_define_module_function(rb_host,"isAvail",host_is_avail,1);
   
}
