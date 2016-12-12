/* Copyright (c) 2009-2016. The SimGrid Team.  All rights reserved.         */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/surf/surf_routing.hpp"
#include "simgrid/msg.h"

int MSG_FILE_LEVEL = -1;             //Msg file level

int SIMIX_STORAGE_LEVEL = -1;        //Simix storage level
int MSG_STORAGE_LEVEL = -1;          //Msg storage level

xbt_lib_t as_router_lib;
int ROUTING_ASR_LEVEL = -1;          //Routing level
int ROUTING_PROP_ASR_LEVEL = -1;     //Where the properties are stored
