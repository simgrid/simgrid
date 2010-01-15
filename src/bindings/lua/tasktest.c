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

XBT_LOG_NEW_DEFAULT_CATEGORY(lua,"Lua bindings");

char *lua_file;
//***************************** LOAD LUA *************************************************
static int load_lua(char * luaFile, lua_State *L) {
  luaL_openlibs(L);


  if (luaL_loadfile(L, luaFile) || lua_pcall(L, 0, 0, 0)) {
    printf("error while parsing %s: %s", luaFile, lua_tostring(L, -1));
    exit(1);
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

extern const char*xbt_ctx_factory_to_use; /*Hack: let msg load directly the right factory */

int main(int argc,char * argv[]) {
  MSG_error_t res = MSG_OK;
  void *lua_factory;

  xbt_ctx_factory_to_use = "lua";
  MSG_global_init(&argc, argv);


  if(argc < 4) {
    printf("Usage: %s platform_file deployment_file lua_script\n", argv[0]);
    printf("example: %s msg_platform.xml msg_deployment.xml script_lua.lua\n", argv[0]);
    exit(1);

  }

  /* MSG_config("surf_workstation_model","KCCFLN05"); */
  SIMIX_ctx_lua_factory_loadfile(argv[3]);

  MSG_create_environment(argv[1]);
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






