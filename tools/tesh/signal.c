/* signal -- what TESH needs to know about signals                          */

/* Copyright (c) 2007, 2008, 2009, 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "tesh.h"
#include <signal.h>

typedef struct s_signal_entry {
  const char *name;
  int number;
} s_signal_entry_t, *signal_entry_t;

static const s_signal_entry_t signals[] = {
  {"SIGHUP", SIGHUP},
  {"SIGINT", SIGINT},
  {"SIGQUIT", SIGQUIT},
  {"SIGILL", SIGILL},
  {"SIGTRAP", SIGTRAP},
  {"SIGABRT", SIGABRT},
  {"SIGFPE", SIGFPE},
  {"SIGKILL", SIGKILL},
  {"SIGBUS", SIGBUS},
  {"SIGSEGV", SIGSEGV},
  {"SIGSYS", SIGSYS},
  {"SIGPIPE", SIGPIPE},
  {"SIGALRM", SIGALRM},
  {"SIGTERM", SIGTERM},
  {"SIGURG", SIGURG},
  {"SIGSTOP", SIGSTOP},
  {"SIGTSTP", SIGTSTP},
  {"SIGCONT", SIGCONT},
  {"SIGCHLD", SIGCHLD},
  {"SIGTTIN", SIGTTIN},
  {"SIGTTOU", SIGTTOU},
  {"SIGIO", SIGIO},
  {"SIGXCPU", SIGXCPU},
  {"SIGXFSZ", SIGXFSZ},
  {"SIGVTALRM", SIGVTALRM},
  {"SIGPROF", SIGPROF},
  {"SIGWINCH", SIGWINCH},
  {"SIGUSR1", SIGUSR1},
  {"SIGUSR2", SIGUSR2},
  {"SIG UNKNOWN", -1}
};


const char *signal_name(unsigned int got, char *expected)
{
  int i;

  /* Make SIGBUS a synonym for SIGSEGV
     (segfault leads to any of them depending on the system) */
  if ((got == SIGBUS) && !strcmp("SIGSEGV", expected))
    got = SIGSEGV;

  for (i = 0; signals[i].number != -1; i++)
    if (signals[i].number == got)
      return (signals[i].name);

  return bprintf("SIG UNKNOWN (%d)", got);
}
