/* $Id$ */

/* spawn - demo of the gras_agent_spawn function                            */

/* Copyright (c) 2007 Martin Quinson. All rights reserved.                  */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "spawn.h"
XBT_LOG_NEW_DEFAULT_CATEGORY(Spawn, "Messages specific to this example");

/* register messages which may be sent (common to client and server) */
void spawn_register_messages(void)
{
  gras_msgtype_declare("ping", gras_datadesc_by_name("int"));
  gras_msgtype_declare("pong", gras_datadesc_by_name("int"));
}
