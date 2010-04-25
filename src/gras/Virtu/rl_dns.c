/* rl_dns - name resolution (real life)                                     */

/* Copyright (c) 2003, 2004 Martin Quinson. All rights reserved.            */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "gras/Virtu/virtu_rl.h"
#include "portable.h"

/* A portable DNS resolver is a nightmare to do in a portable manner.
   keep it simple/stupid for now. */

const char *gras_os_myname(void)
{
  static char *myname = NULL;

  if (myname)
    return (const char *) myname;

  myname = xbt_new(char, 255);

  if (gethostname(myname, 255) == -1) {
#ifdef HAVE_SYS_SOCKET_H
    /* gethostname() failed! Trying with localhost instead. 
       We first need to query the DNS to make sure localhost is resolved 
       See the note in nws/Portability/dnsutil.c about {end,set}hostent() */
    struct hostent *tmp;
    sethostent(0);
    tmp = gethostbyname("localhost");
    endhostent();

    if (tmp) {
      strncat(myname, tmp->h_name, 255);
    } else {
      /* Erm. localhost cannot be resolved. There's something wrong in the user DNS setting */
      sprintf(myname, "(misconfigured host)");
    }
#else
    sprintf(myname, "(misconfigured windows host)");
#endif
  }

  myname[254] = '\0';

  return myname;
}
