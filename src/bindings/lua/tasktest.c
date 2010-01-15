#include <stdio.h>
#include "lauxlib.h"
#include "lualib.h"

// Msg Includes
#include <stdio.h>
#include "msg/msg.h"
#include "msg/datatypes.h"
#include "xbt/sysdep.h"
#include "xbt/log.h"
#include "xbt/asserts.h"



// *** Testing Stuff !!
XBT_LOG_NEW_DEFAULT_CATEGORY(msg_test,
    "Messages specific for this msg example");

int master_lua(int argc, char *argv[]); 
int slave_lua(int argc, char *argv[]); 
//int load_lua(char * file);
//int forwarder(int argc, char *argv[]); LUA
MSG_error_t test_all(const char *platform_file, const char *application_file);

typedef enum {

  PORT_22 = 0,
      MAX_CHANNEL

} channel_t;

char *lua_file;
//***************************** LOAD LUA *************************************************
static int load_lua(char * luaFile, lua_State *L) {
  luaL_openlibs(L);

  // Lua Stuff
  Task_register(L);
  Host_register(L);
  Process_register(L);

  if (luaL_loadfile(L, luaFile) || lua_pcall(L, 0, 0, 0)) {
    printf("error while parsing %s: %s", luaFile, lua_tostring(L, -1));
    return -1;
  }

  return 0;

}

int lua_wrapper(int argc, char *argv[]) {
  lua_State *L = lua_open();
  load_lua(lua_file, L);

  // Seek the right lua function
  lua_getglobal(L,argv[0]);
  if(!lua_isfunction(L,-1)) {
    lua_pop(L,1);
    ERROR1("The lua function %s does not seem to exist",argv[0]);
    return -1;
  }

  // push arguments onto the stack
  int i;
  for(i=1;i<argc;i++)
    lua_pushstring(L,argv[i]);

  // Call the function
  lua_call(L,argc-1,0); // takes argc-1 argument, returns nothing

  // User process terminated
  lua_pop(L, 1);
  lua_close(L);
  return 0;
}



//*****************************************************************************

int main(int argc,char * argv[])
{


  MSG_error_t res = MSG_OK;

  MSG_global_init(&argc, argv);

  if(argc < 4)
  {
    printf("Usage: %s platform_file deployment_file lua_script\n", argv[0]);
    printf("example: %s msg_platform.xml msg_deployment.xml script_lua.lua\n", argv[0]);
    exit(1);

  }

  lua_file=argv[3];
//  load_lua(argv[3]);

  /* MSG_config("surf_workstation_model","KCCFLN05"); */
  MSG_create_environment(argv[1]);
  MSG_function_register_default(&lua_wrapper);
  MSG_launch_application(argv[2]);
  res = MSG_main();

  fflush(stdout);
  INFO1("Simulation time %g", MSG_get_clock());

  MSG_clean();

  if (res == MSG_OK)
    return 0;
  else
    return 1;
}     






