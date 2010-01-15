#include "Msglua.h"



// *** Testing Stuff !!
XBT_LOG_NEW_DEFAULT_CATEGORY(msg_test,
    "Messages specific for this msg example");

int master_lua(int argc, char *argv[]); 
int slave_lua(int argc, char *argv[]); 
int load_lua(char * file);
//int forwarder(int argc, char *argv[]); LUA
MSG_error_t test_all(const char *platform_file, const char *application_file);

typedef enum {

  PORT_22 = 0,
      MAX_CHANNEL

} channel_t;

lua_State *L;

//***************************** LOAD LUA *************************************************
int load_lua(char * luaFile) {
  L = lua_open();

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
  // Table that Lua will read to read Arguments
  lua_newtable(L);

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

  load_lua(argv[3]);

  /* MSG_config("surf_workstation_model","KCCFLN05"); */
  MSG_create_environment(argv[1]);
  MSG_function_register_default(&lua_wrapper);
  MSG_launch_application(argv[2]);
  MSG_set_channel_number(10);
  res = MSG_main();

  INFO1("Simulation time %g", MSG_get_clock());

  MSG_clean();
  lua_close(L);

  if (res == MSG_OK)
    return 0;
  else
    return 1;
}     






