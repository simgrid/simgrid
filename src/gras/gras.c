/* gras.c -- generic functions not fitting anywhere else                    */

/* Copyright (c) 2004, 2005, 2006, 2007, 2008, 2009, 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "xbt/log.h"
#include "xbt/virtu.h"          /* set the XBT virtualization to use GRAS */
#include "xbt/module.h"         /* xbt_init/exit */
#include "xbt/xbt_os_time.h"    /* xbt_os_time */
#include "xbt/synchro.h"
#include "xbt/socket.h"

#include "Virtu/virtu_interface.h"      /* Module mechanism FIXME: deplace&rename */
#include "Virtu/virtu_private.h"
#include "gras_modinter.h"      /* module init/exit */
#include "amok/amok_modinter.h" /* module init/exit */
#include "xbt_modinter.h"       /* module init/exit */

#include "gras.h"
#include "gras/process.h"       /* FIXME: killme and put process_init in modinter */
#include "gras/transport.h"
#include "gras/Msg/msg_private.h"
#include "portable.h"           /* hexa_*(); signalling stuff */

XBT_LOG_NEW_DEFAULT_CATEGORY(gras,
                             "All GRAS categories (cf. section \\ref GRAS_API)");
static int gras_running_process = 0;
#if defined(HAVE_SIGNAL) && defined(HAVE_SIGNAL_H)
static void gras_sigusr_handler(int sig)
{
  XBT_INFO("SIGUSR1 received. Display the backtrace");
  xbt_backtrace_display_current();
}

static void gras_sigint_handler(int sig)
{
  static double lastone = 0;
  if (lastone == 0 || xbt_os_time() - lastone > 5) {
    if (gras_if_RL())
      xbt_backtrace_display_current();
    else
      SIMIX_display_process_status();
    fprintf(stderr,
            "\nBacktrace displayed because Ctrl-C was pressed. Press again (within 5 sec) to abort the process.\n");
    lastone = xbt_os_time();
  } else {
    exit(1);
  }
}
#endif

/**
 * @ingroup GRAS_API
 * \brief Initialize the gras mechanisms.
 */
void gras_init(int *argc, char **argv)
{
  int first = 0;
  gras_procdata_t *pd;
  gras_msg_procdata_t msg_pd;

  xbt_getpid = gras_os_getpid;
  /* First initialize the XBT */
  xbt_init(argc, argv);

  XBT_VERB("Initialize GRAS");

  /* module registrations:
   *    - declare process specific data we need (without creating them)
   */
  if (gras_running_process == 0) {
    first = 1;
    /* Connect our log channels: that must be done manually under windows */

    XBT_LOG_CONNECT(gras_modules, gras);

    XBT_LOG_CONNECT(gras_msg, gras);
    XBT_LOG_CONNECT(gras_msg_read, gras_msg);
    XBT_LOG_CONNECT(gras_msg_rpc, gras_msg);

    XBT_LOG_CONNECT(gras_timer, gras);

    XBT_LOG_CONNECT(gras_virtu, gras);
    XBT_LOG_CONNECT(gras_virtu_emul, gras_virtu);
    XBT_LOG_CONNECT(gras_virtu_process, gras_virtu);

    if (!getenv("GRAS_NO_WARN_EXPERIMENTAL"))
      XBT_WARN("GRAS is not well maintained anymore. We consider it to be experimental (and not stable anymore) since SimGrid 3.6. Sorry about it, please consider contributing to improve this situation");

    gras_trp_register();
    gras_msg_register();

    xbt_trp_plugin_new("file", gras_trp_file_setup);
    if (gras_if_SG()) {
      xbt_trp_plugin_new("sg", gras_trp_sg_setup);
    }
    /* the TCP plugin (used in RL mode) is automatically loaded by XBT */
  }
  gras_running_process++;

  /*
   * Initialize the process specific stuff
   */
  gras_process_init();          /* calls procdata_init, which creates process specific data for each module */

  /*
   * Initialize the global stuff if it's not the first process created
   */
  if (first) {
    gras_emul_init();
    gras_msg_init();
#if defined(HAVE_SIGNAL) && defined(HAVE_SIGNAL_H)
# ifdef SIGUSR1
    signal(SIGUSR1, gras_sigusr_handler);
# endif
    signal(SIGINT, gras_sigint_handler);
#endif
  }

  /* and then init amok */
  amok_init();

  /* And finally, launch the listener thread */
  pd = gras_procdata_get();
  msg_pd = gras_libdata_by_name("gras_msg");
  pd->listener = gras_msg_listener_launch(msg_pd->msg_received);
}

/**
 * @ingroup GRAS_API
 * @brief Finalize the gras mechanisms.
 * */
void gras_exit(void)
{
  XBT_INFO("Exiting GRAS");
  amok_exit();
  gras_moddata_leave();
  gras_msg_listener_shutdown();
  gras_process_exit();
  if (--gras_running_process == 0) {
    gras_msg_exit();
    gras_emul_exit();
    gras_moddata_exit();
  }
}

const char *hexa_str(unsigned char *data, int size, int downside)
{
  static char *buff = NULL;
  static int buffsize = 0;
  int i, pos = 0;
  int begin, increment;

  if (buffsize < 5 * (size + 1)) {
    free(buff);
    buffsize = 5 * (size + 1);
    buff = xbt_malloc(buffsize);
  }


  if (downside) {
    begin = size - 1;
    increment = -1;
  } else {
    begin = 0;
    increment = 1;
  }

  for (i = begin; 0 <= i && i < size; i += increment) {
    if (data[i] < 32 || data[i] > 126)
      sprintf(buff + pos, ".");
    else
      sprintf(buff + pos, "%c", data[i]);
    while (buff[++pos]);
  }
  sprintf(buff + pos, "(");
  while (buff[++pos]);
  for (i = begin; 0 <= i && i < size; i += increment) {
    sprintf(buff + pos, "%02x", data[i]);
    while (buff[++pos]);
  }
  sprintf(buff + pos, ")");
  while (buff[++pos]);
  buff[pos] = '\0';
  return buff;
}

void hexa_print(const char *name, unsigned char *data, int size)
{
  printf("%s: %s\n", name, hexa_str(data, size, 0));
}
