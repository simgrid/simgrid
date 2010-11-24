/* process_sg - GRAS process handling on simulator                          */

/* Copyright (c) 2004, 2005, 2006, 2007, 2008, 2009, 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "xbt/ex.h"
#include "xbt/dict.h"
#include "gras_modinter.h"      /* module initialization interface */
#include "gras/Virtu/virtu_sg.h"
#include "gras/Msg/msg_interface.h"     /* For some checks at simulation end */
#include "gras/Transport/transport_interface.h" /* For some checks at simulation end */
#if HAVE_LUA
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
#endif
XBT_LOG_EXTERNAL_DEFAULT_CATEGORY(gras_virtu_process);

static long int PID = 1;


void gras_agent_spawn(const char *name,
                      xbt_main_func_t code, int argc, char *argv[],
                      xbt_dict_t properties)
{

  SIMIX_process_create(name, code, NULL,
                       gras_os_myname(), argc, argv, properties);
}

/* **************************************************************************
 * Process constructor/destructor (semi-public interface)
 * **************************************************************************/

void gras_process_init()
{
  gras_hostdata_t *hd =
      (gras_hostdata_t *) SIMIX_host_get_data(SIMIX_host_self());
  gras_procdata_t *pd = xbt_new0(gras_procdata_t, 1);
  gras_trp_procdata_t trp_pd;

  SIMIX_process_set_data(SIMIX_process_self(), (void *) pd);


  gras_procdata_init();

  if (!hd) {
    /* First process on this host */
    hd = xbt_new(gras_hostdata_t, 1);
    hd->refcount = 1;
    hd->ports = xbt_dynar_new(sizeof(gras_sg_portrec_t), NULL);
    SIMIX_host_set_data(SIMIX_host_self(), (void *) hd);
  } else {
    hd->refcount++;
  }

  trp_pd = (gras_trp_procdata_t) gras_libdata_by_name("gras_trp");
  pd->pid = PID++;

  if (SIMIX_process_self() != NULL) {
    pd->ppid = gras_os_getpid();
  } else
    pd->ppid = -1;

  trp_pd->msg_selectable_sockets = xbt_queue_new(0, sizeof(gras_socket_t));

  trp_pd->meas_selectable_sockets =
      xbt_queue_new(0, sizeof(gras_socket_t));

  VERB2("Creating process '%s' (%d)",
        SIMIX_process_get_name(SIMIX_process_self()), gras_os_getpid());
}

void gras_process_exit()
{
  xbt_dynar_t sockets =
      ((gras_trp_procdata_t) gras_libdata_by_name("gras_trp"))->sockets;
  gras_socket_t sock_iter;
  unsigned int cursor;
  gras_hostdata_t *hd =
      (gras_hostdata_t *) SIMIX_host_get_data(SIMIX_host_self());
  gras_procdata_t *pd =
      (gras_procdata_t *) SIMIX_process_get_data(SIMIX_process_self());

  gras_msg_procdata_t msg_pd =
      (gras_msg_procdata_t) gras_libdata_by_name("gras_msg");
  gras_trp_procdata_t trp_pd =
      (gras_trp_procdata_t) gras_libdata_by_name("gras_trp");

  xbt_queue_free(&trp_pd->msg_selectable_sockets);

  xbt_queue_free(&trp_pd->meas_selectable_sockets);


  xbt_assert0(hd, "Run gras_process_init (ie, gras_init)!!");

  VERB2("GRAS: Finalizing process '%s' (%d)",
        SIMIX_process_get_name(SIMIX_process_self()), gras_os_getpid());

  if (xbt_dynar_length(msg_pd->msg_queue)) {
    unsigned int cpt;
    s_gras_msg_t msg;
    WARN2
        ("process %d terminated, but %ld messages are still queued. Message list:",
         gras_os_getpid(), xbt_dynar_length(msg_pd->msg_queue));
    xbt_dynar_foreach(msg_pd->msg_queue, cpt, msg) {
      WARN5("   Message %s (%s) from %s@%s:%d", msg.type->name,
            e_gras_msg_kind_names[msg.kind],
            gras_socket_peer_proc(msg.expe),
            gras_socket_peer_name(msg.expe),
            gras_socket_peer_port(msg.expe));
    }
  }

  /* if each process has its sockets list, we need to close them when the
     process finish */
  xbt_dynar_foreach(sockets, cursor, sock_iter) {
    VERB1("Closing the socket %p left open on exit. Maybe a socket leak?",
          sock_iter);
    gras_socket_close(sock_iter);
  }
  if (!--(hd->refcount)) {
    xbt_dynar_free(&hd->ports);
    free(hd);
  }
  gras_procdata_exit();
  free(pd);
}

/* **************************************************************************
 * Process data (public interface)
 * **************************************************************************/

gras_procdata_t *gras_procdata_get(void)
{
  gras_procdata_t *pd =
      (gras_procdata_t *) SIMIX_process_get_data(SIMIX_process_self());

  xbt_assert0(pd, "Run gras_process_init! (ie, gras_init)");

  return pd;
}

void *gras_libdata_by_name_from_remote(const char *name, smx_process_t p)
{
  gras_procdata_t *pd = (gras_procdata_t *) SIMIX_process_get_data(p);

  xbt_assert2(pd,
              "process '%s' on '%s' didn't run gras_process_init! (ie, gras_init)",
              SIMIX_process_get_name(p),
              SIMIX_host_get_name(SIMIX_process_get_host(p)));

  return gras_libdata_by_name_from_procdata(name, pd);
}

/** @brief retrieve the value of a given process property (or NULL if not defined) */
const char *gras_process_property_value(const char *name)
{
  return xbt_dict_get_or_null(
               SIMIX_process_get_properties(SIMIX_process_self()), 
                                            name);
}

/** @brief retrieve the process properties dictionnary
 *  @warning it's the original one, not a copy. Don't mess with it
 */
xbt_dict_t gras_process_properties(void)
{
  return SIMIX_process_get_properties(SIMIX_process_self());
}

/* **************************************************************************
 * OS virtualization function
 * **************************************************************************/

const char *xbt_procname(void)
{
  smx_process_t process = SIMIX_process_self();
  /*FIXME: maestro used not have a simix process, now it does so 
     SIMIX_process_self will return something different to NULL. This breaks
     the old xbt_log logic that assumed that NULL was equivalent to maestro,
     thus when printing it searches for maestro host name (which doesn't exists)
     and breaks the logging.
     As a hack we check for maestro by looking to the assigned host, if it is
     NULL then we are sure is maestro
   */
  if (process != NULL && SIMIX_process_get_host(process))
    return SIMIX_process_get_name(process);

  return "";
}

int gras_os_getpid(void)
{
  gras_procdata_t *data;
  smx_process_t process = SIMIX_process_self();

  if (process != NULL) {
    data = (gras_procdata_t *) SIMIX_process_get_data(process);
    if (data != NULL)
      return data->pid;
  }

  return 0;
}

/** @brief retrieve the value of a given host property (or NULL if not defined) */
const char *gras_os_host_property_value(const char *name)
{
  return
      xbt_dict_get_or_null(SIMIX_host_get_properties
                           (SIMIX_process_get_host(SIMIX_process_self())),
                           name);
}

/** @brief retrieve the host properties dictionnary
 *  @warning it's the original one, not a copy. Don't mess with it
 */
xbt_dict_t gras_os_host_properties(void)
{
  return
      SIMIX_host_get_properties(SIMIX_process_get_host
                                (SIMIX_process_self()));
}

/* **************************************************************************
 * Interface with SIMIX
 * (these functions are called by the stuff generated by gras_stub_generator)
 * **************************************************************************/

XBT_LOG_EXTERNAL_CATEGORY(gras_trp);
XBT_LOG_EXTERNAL_CATEGORY(gras_trp_sg);

void gras_global_init(int *argc, char **argv)
{
  XBT_LOG_CONNECT(gras_trp_sg, gras_trp);
  SIMIX_global_init(argc, argv);
}

void gras_create_environment(const char *file)
{
  SIMIX_create_environment(file);
}

void gras_function_register(const char *name, xbt_main_func_t code)
{
  SIMIX_function_register(name, code);
}

void gras_function_register_default(xbt_main_func_t code)
{
  SIMIX_function_register_default(code);
}

void gras_main()
{
  /* Clean IO before the run */
  fflush(stdout);
  fflush(stderr);
  SIMIX_init();

  while (SIMIX_solve(NULL, NULL) != -1.0);

  return;
}

void gras_launch_application(const char *file)
{
  SIMIX_launch_application(file);
}

void gras_load_environment_script(const char *script_file)
{
#ifdef HAVE_LUA
  lua_State *L = lua_open();
  luaL_openlibs(L);

  if (luaL_loadfile(L, script_file) || lua_pcall(L, 0, 0, 0)) {
    printf("error: %s\n", lua_tostring(L, -1));
    return;
  }
#else
  xbt_die
      ("Lua is not available!! to call gras_load_environment_script, lua should be available...");
#endif
  return;
}

void gras_clean()
{
  SIMIX_clean();
}
